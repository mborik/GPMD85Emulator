/*	ChipPIO8255.cpp: Class for emulation of PIO 8255 chip
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
//---------------------------------------------------------------------------
#include "ChipPIO8255.h"
/**
 * Constructor creates object for PIO 8255 chip.
 * Corresponds to Power-up state, default state after connecting the power.
 * Difference between this implementation and real chip is the possibility to
 * immediately reset the virtual chip while real hardware always remains
 * uninitialized. All signals will be cleared.
 *
* @param reset if true, perform reset o chip
 */
ChipPIO8255::ChipPIO8255(bool reset)
{
	if (reset)
		ChipReset(true);
	else
		ClearAllNotifyFunctions();
}
//---------------------------------------------------------------------------
/**
 * Private method to disable all signals.
 */
void ChipPIO8255::ClearAllNotifyFunctions()
{
	OnCpuWriteA.disconnect_all();
	OnCpuWriteB.disconnect_all();
	OnCpuWriteC.disconnect_all();
	OnCpuWriteCH.disconnect_all();
	OnCpuWriteCL.disconnect_all();
	OnCpuWriteCWR.disconnect_all();
	OnCpuReadA.disconnect_all();
	OnCpuReadB.disconnect_all();
	OnCpuReadC.disconnect_all();
	OnCpuReadCH.disconnect_all();
	OnCpuReadCL.disconnect_all();
	OnCpuReadCWR.disconnect_all();
}
//---------------------------------------------------------------------------
/**
 * Method ChipReset performs reset of a chip. It is an equivalent to
 * providing H logical state to RESET input pin (35)
 * Optionally signals can be disabled.
 *
 * @param clearNotifyFunc if true, disable signals
 */
void ChipPIO8255::ChipReset(bool clearNotifyFunc)
{
	if (clearNotifyFunc)
		ClearAllNotifyFunctions();

	// set all of ports of mode 0
	// reset all internal registers
	CpuWrite(PP_CWR, BASIC_CWR); // b10011011
	InLatchA = 0; // mode setting don't reset input latch,
	InLatchB = 0; // we do it manually
}
//---------------------------------------------------------------------------
/**
 * The GetChipState method is used when creating a Snapshot and stores some
 * internal chip registers into a buffer.
 * If the buffer is null, it returns the required size of the buffer in bytes.
 * Data is stored in the buffer in the order: CWR, PC, PB, PA, interrupts
 *
 * @param buffer buffer where the chip state is stored
 * @return number of bytes stored in the buffer
 */
int ChipPIO8255::GetChipState(BYTE *buffer)
{
	if (buffer != NULL) {
		*buffer = CWR;
		*(buffer + 1) = OutLatchC;
		*(buffer + 2) = OutLatchB;
		*(buffer + 3) = OutLatchA;
		*(buffer + 4) = (BYTE) (
			(InteAin ? 1 : 0) |
			(InteAout ? 2 : 0) |
			(InteB ? 4 : 0)
		);
	}

	return 5;
}
//---------------------------------------------------------------------------
/**
 * The SetChipState method is used after opening a Snapshot to preset
 * some internal chip registers.
 * Data in the buffer must be in the order: CWR, PC, PB, PA, interrupts
 *
 * @param buffer buffer where the chip state is stored
 * @return number of bytes stored in the buffer
 */
void ChipPIO8255::SetChipState(BYTE *buffer)
{
	if (buffer != NULL && *buffer != 0) {
		ChipReset(false);
		CpuWrite(PP_CWR, *buffer);
		CpuWrite(PP_PortC, *(buffer + 1));
		CpuWrite(PP_PortB, *(buffer + 2));
		CpuWrite(PP_PortA, *(buffer + 3));
		if (*(buffer + 4) & 1)
			CpuWrite(PP_CWR, 9);  // InteAin - PC4 <- 1
		if (*(buffer + 4) & 2)
			CpuWrite(PP_CWR, 13); // InteAout - PC6 <- 1
		if (*(buffer + 4) & 4)
			CpuWrite(PP_CWR, 5);  // InteB - PC2 <- 1
	}
}
//---------------------------------------------------------------------------
/**
 * Private method NotifyOnWritePortC called when writing to port C by the CPU.
 * Notification occurs only if the output value on port C has changed.
 * WARNING: Notification is performed either by calling the OnCpuWriteC function
 * or a pair of OnCpuWriteCH and OnCpuWriteCL. OnCpuWriteC has higher priority.
 * Therefore, if the address of OnCpuWriteC function is set, signals
 * for port half are ignored.
 *
 * @param oldVal original value on the output of port C
 * @param newVal new value sent to the output of port C
 */
void ChipPIO8255::NotifyOnWritePortC(BYTE oldVal, BYTE newVal)
{
	BYTE val = oldVal ^ newVal;
	if (val && OnCpuWriteC.isset())
		OnCpuWriteC();
	else {
		BYTE maskCH = 0xF0;
		BYTE maskCL = 0x0F;
		if (((CWR & GA_MODE) != GA_MODE0) || ((CWR & GB_MODE) != GB_MODE0)) {
			maskCH = 0xF8;
			maskCL = 0x07;
		}

		if ((val & maskCH) && OnCpuWriteCH.isset())
			OnCpuWriteCH();
		if ((val & maskCL) && OnCpuWriteCL.isset())
			OnCpuWriteCL();
	}
}
//---------------------------------------------------------------------------
/**
 * CpuWrite method is called by the CPU during the execution of an OUT instruction
 * - write to a port (CPU -> PIO).
 * It corresponds to bringing logic L level to inputs /CS (6) and /WR (36).
 * AFTER writing the value 'val' to the destination port 'dest', it calls the appropriate
 * signal function (if it was set by the specific peripheral) so that the
 * peripheral can process the data.
 * For ports A and B, this applies only if they are in Mode 0.
 * In Modes 1 and 2, the peripheral is notified by the signal functions of port C
 * through handshake signals.
 *
 * @param dest specifies the port (TPIOPort) to which the value 'val' is sent
 * @param val value sent to the given port
 */
void ChipPIO8255::CpuWrite(TPIOPort dest, BYTE val)
{
	BYTE oldVal;
	BYTE mode;

	switch (dest) {
		case PP_PortA :
			// mode 0
			if ((CWR & GA_MODE) == GA_MODE0) {
				// value change
				OutLatchA = val;
				OnCpuWriteA();
			}
			// mode 1, 2
			else {
				OutLatchA = val;
				oldVal = OutLatchC;
				OutLatchC &= (BYTE) (~(INTRA_MASK | _OBFA_MASK));
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortB :
			// mode 0
			if ((CWR & GB_MODE) == GB_MODE0) {
				OutLatchB = val;
				OnCpuWriteB();
			}
			// mode 1
			else {
				OutLatchB = val;
				oldVal = OutLatchC;
				OutLatchC &= ~(INTRB_MASK | _OBFB_MASK);
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortC :
			// in mode 0 are affected only output bits
			oldVal = OutLatchC;

			if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)       // PB Mode 0, PCL Out
			 && (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))      // PA Mode 0, PCH Out
				OutLatchC = val;
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)  // PB Mode 0, PCL Out
			      && (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP)) // PA Mode 0, PCH In
				OutLatchC = (BYTE) ((OutLatchC & 0xF0) | (val & 0x0F));
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)) // PB Mode 0, PA Mode 1/2
				OutLatchC = (BYTE) ((OutLatchC & 0xF8) | (val & 0x07));         // PC3 is INTRA
			else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)) // PA Mode 0, PB Mode 1
				OutLatchC = (BYTE) ((OutLatchC & 0x0F) | (val & 0xF0));         // PC3 only via bit-set

			NotifyOnWritePortC(oldVal, OutLatchC);
			break;

		case PP_CWR :
			if (val & CWR_MASK) {
				// setting of PIO mode
				CWR = val;
				if ((CWR & GA_MODE) == GA_MODE)
					CWR &= ~GA_MODE1;

				// reset all internal registers
				InBufferA = 0xFF; // pull-up
				OutLatchA = 0;

				InBufferB = 0xFF; // pull-up
				if ((CWR & GB_MODE) != GB_MODE0)
					InLatchB = 0;
				OutLatchB = 0;

				InBufferC = 0;
				if ((CWR & GB_MODE) == GB_MODE0)
					InBufferC |= 0x07; // pull-up
				if ((CWR & GA_MODE) == GA_MODE0)
					InBufferC |= 0xF8;

//				oldVal = OutLatchC;
				OutLatchC = 0;
				if ((CWR & GB_MODE) != GB_MODE0)
					OutLatchC |= _OBFB_MASK;
				if ((CWR & GA_MODE) != GA_MODE0)
					OutLatchC |= _OBFA_MASK;

				// disable interupt
				InteAin = false;
				InteAout = false;
				InteB = false;

				OnCpuWriteCWR(CWR);
/*
				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)
						&& (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
					oldVal = ~OutLatchC;
				else {
					if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))
						oldVal = OutLatchC ^ 0x07;
					if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
						oldVal = OutLatchC ^ 0xF8;
				}
				NotifyOnWritePortC(oldVal, OutLatchC);
*/
				NotifyOnWritePortC((BYTE) (~OutLatchC), OutLatchC);
			}
			else {
				// setting of bits of port C
				bool inte = false; // local flag whether are INTE bits changed
				val &= 0x0F; // only lower nibble is used

				mode = (BYTE) (CWR & (GA_MODE | PORTA_DIR));
				oldVal = OutLatchC;

				if ((mode == (GA_MODE1 | PORTA_INP) || (CWR & GA_MODE) == GA_MODE2) && (val & 0x0E) == 8) {
					InteAin = (val & 1);
					inte = true;
//					debug("ChipPIO8255", "InteAin=%d", InteAin);
				}
				else if ((mode == (GA_MODE1 | PORTA_OUT) || (CWR & GA_MODE) == GA_MODE2) && (val & 0x0E) == 12) {
					InteAout = (val & 1);
					inte = true;
//					debug("ChipPIO8255", "InteAout=%d", InteAout);
				}

				if (inte) {
					OutLatchC &= ~INTRA_MASK; // INTRA=0
					if ((InteAin
						&& (InBufferC & _STBA_MASK) == _STBA_MASK
						&& (OutLatchC & IBFA_MASK) == IBFA_MASK)
						|| (InteAout
							&& (InBufferC & _ACKA_MASK) == _ACKA_MASK
							&& (OutLatchC & _OBFA_MASK) == _OBFA_MASK)) {
						OutLatchC |= INTRA_MASK; // INTRA=1
					}
//					debug("ChipPIO8255", "OutLatchC=%u, InBufferC=%u", OutLatchC, InBufferC);
				}

				if ((CWR & GB_MODE) == GB_MODE1 && (val & 0x0E) == 4) {
					inte = true;
					InteB = (val & 1);
					OutLatchC &= ~INTRB_MASK; // INTRB=0
					if (InteB) {
						if (((CWR & PORTB_INP) == PORTB_INP
							&& (InBufferC & _STBB_MASK) == _STBB_MASK
							&& (OutLatchC & IBFB_MASK) == IBFB_MASK)
							|| ((CWR & PORTB_OUT) == PORTB_OUT
								&& (InBufferC & _ACKB_MASK) == _ACKB_MASK
								&& (OutLatchC & _OBFB_MASK) == _OBFB_MASK)) {
							OutLatchC |= INTRB_MASK;
						}
					}
				}

				if (!inte) {
					if (  ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)  // PB Mode 0, PCL output
					    && (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)) // PA Mode 0, PCH output
					|| ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)     // PB Mode 0, PCL output
					    && (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP)  // PA Mode 0, PCH input
					    && (val >> 1) <= 3) // PCL bits only
					|| ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP)     // PB Mode 0, PCL input
					    && (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)  // PA Mode 0, PCH output
					    && (val >> 1) >= 4) // PCH bits only
					|| ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT)     // PB Mode 0, PCL output, PA Mode 1,2
					    && (val >> 1) <= 2) // PC3 is INTRA
					|| ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT)     // PA Mode 0, PCH output, PB Mode 1
					    && (val >> 1) >= 4) // PCH bits only
					|| ((CWR & (GA_MODE | GB_MODE | PORTCL_DIR)) == (GA_MODE0 | GB_MODE1 | PORTCL_OUT) // PA Mode 0, PB Mode 1, PCL output
					    && (val >> 1) == 3) // PC3 only
					|| ((CWR & (GA_MODE | PORTA_DIR | PORTCH_DIR)) == (GA_MODE1 | PORTA_INP | PORTCH_OUT)
					    && (val >> 1) >= 6) // PC7, PC6 only
					|| ((CWR & (GA_MODE | PORTA_DIR | PORTCH_DIR)) == (GA_MODE1 | PORTA_OUT | PORTCH_OUT)
					    && ((val >> 1) == 5 || (val >> 1) == 4))) // PC5, PC4 only
					{
						if (val & 1)
							OutLatchC |= (BYTE)((1 << (val >> 1)));
						else
							OutLatchC &= (BYTE)(~(1 << (val >> 1)));
					}
				}

				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		default :
			warning("ChipPIO8255", "CpuWrite > invalid PIO port: %d", dest);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * CpuRead method is called by the CPU during the execution of an IN instruction
 * - read from a port (CPU <- PIO).
 * It corresponds to bringing logic L level to inputs /CS (6) and /RD (5).
 * BEFORE reading the value from the source port 'src', it calls the appropriate
 * signal function (if it was set by the specific peripheral) so that the peripheral
 * can prepare the necessary data. For ports A and B, this applies only if they are
 * in Mode 0. In Modes 1 and 2, the peripheral is notified by the signal functions
 * of port C through handshake signals.
 *
 * @param src specifies the port (TPIOPort) from which to read the data
 * @return value read from the port
 */
BYTE ChipPIO8255::CpuRead(TPIOPort src)
{
	BYTE ret_val;
	BYTE oldVal;
	BYTE mode;

	switch (src) {
		case PP_PortA :
			mode = (BYTE)(CWR & (GA_MODE | PORTA_DIR));

			// notification is enabled only in mode 0, input,
			// in other modes there are handshake signals
			if (mode == (GA_MODE0 | PORTA_INP))
				OnCpuReadA();

			// Mode 0, input
			if (mode == (GA_MODE0 | PORTA_INP))
				ret_val = InBufferA;  // reading of value from input
			// Mode 0, output
			else if (mode == (GA_MODE0 | PORTA_OUT))
				// reading of value from output latch with respect to state of input
				ret_val = OutLatchA & InBufferA;
			// Mode 1, output
			else if (mode == (GA_MODE1 | PORTA_OUT))
				ret_val = OutLatchA; // reading of value from output latch
			// Mode 1, input or Mode 2
			else if (mode == (GA_MODE1 | PORTA_INP) || (CWR & GA_MODE) == GA_MODE2) {
				ret_val = InLatchA;  // reading of value supplied by peripheral

				// /RD signal resets INTRA and IBFA bits
				oldVal = OutLatchC;
				OutLatchC &= ~(INTRA_MASK | IBFA_MASK);
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortB :
			mode = (BYTE)(CWR & (GB_MODE | PORTB_DIR));

			// notification is enabled only in mode 0, input,
			// in other modes there are handshake signals
			if (mode == (GB_MODE0 | PORTB_INP))
				OnCpuReadB();

			// Mode 0, output
			if (mode == (GB_MODE0 | PORTB_OUT))
				ret_val = OutLatchB; // reading of value from output latch
			// Mode 0, input
			else if (mode == (GB_MODE0 | PORTB_INP))
				ret_val = InBufferB; // reading of value from input
			// Mode 1, output
			else if (mode == (GB_MODE1 | PORTB_OUT))
				ret_val = OutLatchB; // reading of value from output latch
			// Mode 1, input
			else if (mode == (GB_MODE1 | PORTB_INP)) {
				ret_val = InLatchB;  // reading of value supplied by peripheral

				// /RD signal resets INTRB and IBFB bits
				oldVal = OutLatchC;
				OutLatchC &= ~(INTRB_MASK | IBFB_MASK);
				NotifyOnWritePortC(oldVal, OutLatchC);
			}
			break;

		case PP_PortC :
			if (OnCpuReadCH.isset() || OnCpuReadCL.isset()) {
				// Mode 0, input
				if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP))
					OnCpuReadCH();

				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP))
					OnCpuReadCL();
			}
			else if (OnCpuReadC.isset()) {
				// Mode 0, input
				if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP)
				 || (CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP))
					OnCpuReadC();
			}

			ret_val = 0;

			if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))      // PB Mode 0, PCL output
				ret_val |= (BYTE)(OutLatchC & 0x0F);
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP)) // PB Mode 0, PCL input
				ret_val |= (BYTE)(InBufferC & 0x0F);
			if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))      // PA Mode 0, PCH output
				ret_val |= (BYTE)(OutLatchC & 0xF0);
			else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP)) // PA Mode 0, PCH input
				ret_val |= (BYTE)(InBufferC & 0xF0);

			if ((CWR & GA_MODE) == GA_MODE1) {
				// PA Mode 1
				ret_val &= 0x07; // PC3 is INTRA for PA
				ret_val |= (BYTE) (OutLatchC & INTRA_MASK);
				if ((CWR & PORTA_DIR) == PORTA_INP) {
					// PA Mode 1 - input
					if (InteAin)
						ret_val |= INTEAIN_MASK; // PC4
					ret_val |= (BYTE) (OutLatchC & IBFA_MASK); // PC5
					if ((CWR & PORTCH_DIR) == PORTCH_INP)
						ret_val |= (BYTE) (InBufferC & 0xC0);  // PC7, PC6 - input
					else
						ret_val |= (BYTE) (OutLatchC & 0xC0);  // PC7, PC6 - output
				}
				else {
					// PA Mode 1 - output
					if (InteAout)
						ret_val |= INTEAOUT_MASK;  // PC6
					ret_val |= (BYTE) (OutLatchC & _OBFA_MASK); // PC7
					if ((CWR & PORTCH_DIR) == PORTCH_INP)
						ret_val |= (BYTE) (InBufferC & 0x30);   // PC5, PC4 - input
					else
						ret_val |= (BYTE) (OutLatchC & 0x30);   // PC5, PC4 - output
				}
			}
			else if ((CWR & GA_MODE) == GA_MODE2) {
				// PA Mode 2
				ret_val &= 0x07; // PC3 is INTRA for PA
				ret_val |= (BYTE) (OutLatchC & INTRA_MASK);
				if (InteAin)
					ret_val |= INTEAIN_MASK; // PC4
				ret_val |= (BYTE) (OutLatchC & IBFA_MASK);  // PC5
				if (InteAout)
					ret_val |= INTEAOUT_MASK; // PC6
				ret_val |= (BYTE) (OutLatchC & _OBFA_MASK); // PC7
			}

			if ((CWR & GB_MODE) == GB_MODE1) {
				ret_val &= 0xF8;
				if (InteB)
					ret_val |= INTEB_MASK; // PC2
				ret_val |= (BYTE) (OutLatchC & INTRB_MASK);     // PC0
				if ((CWR & PORTB_DIR) == PORTB_INP)
					ret_val |= (BYTE) (OutLatchC & IBFB_MASK);  // PC1
				else
					ret_val |= (BYTE) (OutLatchC & _OBFB_MASK); // PC1
			}
			break;

		case PP_CWR :       // NMOS version of 8255 doesn't allow read of CWR
			ret_val = CWR;  // this is possible only in CMOS version 82C55
			break;

		default :
			// there could be (theoretically) an exception raised here for 'src' values
			// PP_PortCH, PP_PortCL, which from the CPU point of view cannot be used
			warning("ChipPIO8255", "CpuRead > invalid PIO port: %d", src);
			ret_val = 0;
			break;
	}

	return ret_val;
}
//---------------------------------------------------------------------------
void ChipPIO8255::PeripheralWriteByte(TPIOPort dest, BYTE val)
{
	BYTE oldVal;

	switch (dest) {
		case PP_PortA :
			InBufferA = val;
			break;

		case PP_PortB :
			InBufferB = val;
			break;

		case PP_PortC :
			oldVal = OutLatchC;
			if ((CWR & GB_MODE) == GB_MODE1) {
				// input
				if ((CWR & PORTB_DIR) == PORTB_INP) {
					if ((InBufferC & _STBB_MASK) == _STBB_MASK && (val & _STBB_MASK) == 0) {
						// /STB  --__
						InLatchB = InBufferB;
						OutLatchC |= IBFB_MASK;
						OutLatchC &= ~INTRB_MASK;
					}
					else if ((InBufferC & _STBB_MASK) == 0 && (val & _STBB_MASK) == _STBB_MASK) {
						// /STB  __--
						if (InteB && (OutLatchC & IBFB_MASK) == IBFB_MASK)
							OutLatchC |= INTRB_MASK;
						else
							OutLatchC &= ~INTRB_MASK;
					}
				}
				// output
				else {
					if ((InBufferC & _ACKB_MASK) == _ACKB_MASK && (val & _ACKB_MASK) == 0) {
						// /ACK  --__
						OutLatchC |= _OBFB_MASK;
						OutLatchC &= ~INTRB_MASK;
					}
					else if ((InBufferC & _ACKB_MASK) == 0 && (val & _ACKB_MASK) == _ACKB_MASK) {
						// /ACK  __--
						if (InteB && (OutLatchC & _OBFB_MASK) == _OBFB_MASK)
							OutLatchC |= INTRB_MASK;
						else
							OutLatchC &= ~INTRB_MASK;
					}
				}
			}

			if ((CWR & GA_MODE) != GA_MODE0) {
				// input
				if ((CWR & GA_MODE) == GA_MODE2 || (CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_INP)) {
					if ((InBufferC & _STBA_MASK) == _STBA_MASK && (val & _STBA_MASK) == 0) {
						// /STB  --__
						InLatchA = InBufferA;
						OutLatchC |= IBFA_MASK;
						OutLatchC &= ~INTRA_MASK;
					}
					else if ((InBufferC & _STBA_MASK) == 0 && (val & _STBA_MASK) == _STBA_MASK) {
						// /STB  __--
						if (InteAin && (OutLatchC & IBFA_MASK) == IBFA_MASK)
							OutLatchC |= INTRA_MASK;
						else
							OutLatchC &= ~INTRA_MASK;
					}
				}

				// output
				if ((CWR & GA_MODE) == GA_MODE2 || (CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_OUT)) {
					if ((InBufferC & _ACKA_MASK) == _ACKA_MASK && (val & _ACKA_MASK) == 0) {
						// /ACK  --__
						OutLatchC |= _OBFA_MASK;
						OutLatchC &= ~INTRA_MASK;
					}
					else if ((InBufferC & _ACKA_MASK) == 0 && (val & _ACKA_MASK) == _ACKA_MASK) {
						// /ACK  __--
						if (InteAout && (OutLatchC & _OBFA_MASK) == _OBFA_MASK)
							OutLatchC |= INTRA_MASK;
						else
							OutLatchC &= ~INTRA_MASK;
					}
				}
			}

			InBufferC = val;
			NotifyOnWritePortC(oldVal, OutLatchC);
			break;

		default :
			warning("ChipPIO8255", "PeripheralWriteByte > invalid PIO port: %d", dest);
			break;
	}
}
//---------------------------------------------------------------------------
void ChipPIO8255::PeripheralChangeBit(TPIOPort dest, TPIOPortBit bit, bool state)
{
	BYTE val;

	switch (dest) {
		case PP_PortA :
			InBufferA &= (BYTE)(~(1 << bit));
			InBufferA |= (BYTE)(state ? (1 << bit) : 0);
			break;

		case PP_PortB :
			InBufferB &= (BYTE)(~(1 << bit));
			InBufferB |= (BYTE)(state ? (1 << bit) : 0);
			break;

		case PP_PortC :
			if ((CWR & GB_MODE) == GB_MODE1 && (bit == _STBB || bit == _ACKB)) {
				if (bit == _STBB)
					val = (BYTE)((InBufferC & ~_STBB_MASK) | (state ? _STBB_MASK : 0));
				else
					val = (BYTE)((InBufferC & ~_ACKB_MASK) | (state ? _ACKB_MASK : 0));
				PeripheralWriteByte(dest, val);
			}
			else if ((CWR & GA_MODE) != GA_MODE0 && (bit == _STBA || bit == _ACKA)) {
				if (bit == _STBA)
					val = (BYTE)((InBufferC & ~_STBA_MASK) | (state ? _STBA_MASK : 0));
				else
					val = (BYTE)((InBufferC & ~_ACKA_MASK) | (state ? _ACKA_MASK : 0));
				PeripheralWriteByte(dest, val);
			}
			else {
				InBufferC &= (BYTE)(~(1 << bit));
				InBufferC |= (BYTE)(state ? (1 << bit) : 0);
			}
			break;

		default :
			warning("ChipPIO8255", "PeripheralChangeBit > invalid PIO port: %d", dest);
			break;
	}
}
//---------------------------------------------------------------------------
BYTE ChipPIO8255::PeripheralReadByte(TPIOPort src)
{
	BYTE ret;

	switch (src) {
		case PP_PortA :
			if ((CWR & GA_MODE) == GA_MODE2) {
				if ((OutLatchC & _OBFA_MASK) == 0)
					ret = OutLatchA;
				else if ((OutLatchC & IBFA_MASK) == 0)
					ret = InBufferA;
				else
					ret = 0xFF;
			}
			else {
				if ((CWR & PORTA_DIR) == PORTA_OUT)
					ret = OutLatchA;
				else
					ret = InBufferA;
			}
			break;

		case PP_PortB :
			if ((CWR & PORTB_DIR) == PORTB_OUT)
				ret = OutLatchB;
			else
				ret = InBufferB;
			break;

		case PP_PortC :
			ret = 0;
			if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_OUT))
				ret |= (BYTE)(OutLatchC & 0x07); // PCL output
			else if ((CWR & (GB_MODE | PORTCL_DIR)) == (GB_MODE0 | PORTCL_INP))
				ret |= (BYTE)(InBufferC & 0x07); // PCL input
			else if ((CWR & GB_MODE) == GB_MODE1) {
				ret |= (BYTE)(OutLatchC & 0x03); // IBFB/OBFB, INTRB - output
				ret |= (BYTE)(InBufferC & 0x04); // STBB/ACKB - input
			}

			if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_OUT))
				ret |= (BYTE)(OutLatchC & 0xF0); // PCH - output
			else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE0 | PORTCH_INP))
				ret |= (BYTE)(InBufferC & 0xF0); // PCH - input
			else if ((CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_OUT)) {
				ret |= (BYTE)(OutLatchC & 0x88); // OBFA, INTRA - output
				ret |= (BYTE)(InBufferC & 0x40); // ACKA - input
				if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE1 | PORTCH_OUT))
					ret |= (BYTE) (OutLatchC & 0x30); // PC5, PC4 - output
				else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE1 | PORTCH_INP))
					ret |= (BYTE) (InBufferC & 0x30); // PC5, PC4 - input
			}
			else if ((CWR & (GA_MODE | PORTA_DIR)) == (GA_MODE1 | PORTA_INP)) {
				ret |= (BYTE)(OutLatchC & 0x28); // IBFA, INTRA - output
				ret |= (BYTE)(InBufferC & 0x10); // STBA - input
				if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE1 | PORTCH_OUT))
					ret |= (BYTE) (OutLatchC & 0xC0); // PC7, PC6 - output
				else if ((CWR & (GA_MODE | PORTCH_DIR)) == (GA_MODE1 | PORTCH_INP))
					ret |= (BYTE) (InBufferC & 0xC0); // PC7, PC6 - input
			}
			else if ((CWR & GA_MODE) == GA_MODE2) {
				ret |= (BYTE)(OutLatchC & 0xA8); // OBFA, IBFA, INTRA - output
				ret |= (BYTE)(InBufferC & 0x50); // ACKA, STBA - input
			}

			if ((CWR & GA_MODE) == GA_MODE0) {
				// PC3, if PA is in Mode 0
				if ((CWR & PORTCL_DIR) == PORTCL_OUT)
					ret |= (BYTE)(OutLatchC & 0x08); // PC3 - output
				else if ((CWR & PORTCL_DIR) == PORTCL_INP)
					ret |= (BYTE)(InBufferC & 0x08); // PC3 - input
			}
			break;

		default :
			warning("ChipPIO8255", "PeripheralReadByte > invalid PIO port: %d", src);
			ret = 0xFF;
			break;
	}

	return ret;
}
//---------------------------------------------------------------------------
bool ChipPIO8255::PeripheralReadBit(TPIOPort src, TPIOPortBit bit)
{
	bool ret;

	switch (src) {
		case PP_PortA :
			ret = PeripheralReadByte(PP_PortA) & (1 << bit);
			break;

		case PP_PortB :
			ret = PeripheralReadByte(PP_PortB) & (1 << bit);
			break;

		case PP_PortC :
			ret = PeripheralReadByte(PP_PortC) & (1 << bit);
			break;

		default :
			warning("ChipPIO8255", "PeripheralReadBit > invalid PIO port: %d", src);
			ret = false;
			break;
	}

	return ret;
}
//---------------------------------------------------------------------------
