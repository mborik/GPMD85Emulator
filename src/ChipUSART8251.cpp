/*	ChipUSART8251.cpp: Class for emulation of USART 8251 chip
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
#include "ChipUSART8251.h"
//---------------------------------------------------------------------------
/**
 * Constructor for creating a USART 8251 chip object.
 * Corresponds to the Power-up state (power supply is connected to the chip RESET input).
 * At the same time, notification functions are disconnected.
 */
ChipUSART8251::ChipUSART8251()
{
	ChipReset(true);

	ByteTransferMode = false;
	CWR = 0x4E;   // Async 8N1 k16
	Command = SyncChar1 = SyncChar2 = 0;
}
//---------------------------------------------------------------------------
/**
 * The GetChipState method is used when creating a Snapshot and stores some
 * internal chip registers into a buffer.
 * If the buffer is null, it returns the required size of the buffer in bytes.
 * Data is stored in the buffer in the order: CWR, SyncChar1, SyncChar1, Command
 *
 * @param buffer buffer where the chip state is stored
 * @return number of bytes stored in the buffer
 */
int ChipUSART8251::GetChipState(BYTE *buffer)
{
	if (buffer != NULL) {
		*buffer = CWR;
		*(buffer + 1) = SyncChar1;
		*(buffer + 2) = SyncChar2;
		*(buffer + 3) = Command;
	}

	return 4;
}
//---------------------------------------------------------------------------
/**
 * The SetChipState method is used after opening a Snapshot to preset
 * some internal chip registers.
 * Data in the buffer must be in the order: CWR, SyncChar1, SyncChar1, Command
 *
 * @param buffer buffer where the chip state is stored
 * @return number of bytes stored in the buffer
 */
void ChipUSART8251::SetChipState(BYTE *buffer)
{
	if (buffer != NULL && *buffer != 0) {
		ChipReset(false);
		CpuWrite(UR_CTRL, *buffer);
		if ((*buffer & 3) == 0)
			CpuWrite(UR_CTRL, *(buffer + 1));
		if ((*buffer & 131) == 0)
			CpuWrite(UR_CTRL, *(buffer + 2));
		CpuWrite(UR_CTRL, *(buffer + 3));
	}
}
//---------------------------------------------------------------------------
/**
 * The ChipReset method performs a chip reset.
 * It corresponds to bringing logic H level to the RESET input (21).
 * Optionally, all notification functions can be disconnected.
 *
 * @param clearNotifyFunc if true, clears all notification functions
 */
void ChipUSART8251::ChipReset(bool clearNotifyFunc)
{
	InitState = INIT_MODE;
	SynDetState = false;

	if (clearNotifyFunc)
		ClearAllNotifyFunctions();
}
//---------------------------------------------------------------------------
/**
 * Private method to set all notification functions to NULL.
 */
void ChipUSART8251::ClearAllNotifyFunctions()
{
	OnTxDChange.disconnect_all();
	OnTxRChange.disconnect_all();
	OnTxEChange.disconnect_all();
	OnRxRChange.disconnect_all();
	OnDtrChange.disconnect_all();
	OnRtsChange.disconnect_all();
	OnSynDetChange.disconnect_all();
	OnBrkDetChange.disconnect_all();

	OnTxDSet.disconnect_all();
	OnTxRSet.disconnect_all();
	OnTxESet.disconnect_all();
	OnRxRSet.disconnect_all();
	OnDtrSet.disconnect_all();
	OnRtsSet.disconnect_all();
	OnSynDetSet.disconnect_all();
	OnBrkDetSet.disconnect_all();

	OnCwrWrite.disconnect_all();
}
//---------------------------------------------------------------------------
/**
 * The CpuWrite method is called during the execution of an OUT instruction - write to
 * a port (CPU -> PIO).
 * It corresponds to bringing logic L level to inputs /CS (11) and /WR (10).
 * According to the type of destination register and the state of the circuit,
 * a write is performed to the control, command, or data register.
 *
 * @param dest specifies the register (TUSARTReg) to which the value 'val' is written
 * @param val value to be written
 */
void ChipUSART8251::CpuWrite(TUSARTReg dest, BYTE val)
{
	switch (dest) {
		case UR_CTRL :
			switch (InitState) {
				case INIT_MODE :  // control word
					CWR = val;

					CharLen = ((CWR & CL_MASK) >> CL_SHIFT) + 5;
					InitRxBitCounter();
					TxBitCounter = CharLen;

					switch (val & BRF_MASK) {
						case BRF_SYNC :
							Factor = 0;
							break;

						case BRF_ASYNC_1 :
							Factor = 1;
							break;

						case BRF_ASYNC_16 :
							Factor = 16;
							break;

						case BRF_ASYNC_64 :
							Factor = 64;
							break;
					}

					SyncMode = ((val & BRF_MASK) == BRF_SYNC);
					SynDetState = false;
					OnSynDetSet();

					StatusTxE = true;
					OnTxESet();
					StatusTxR = true;
					OnTxRSet();
					StatusRxR = false;
					OnRxRSet();
					ParityError = false;
					OverrunError = false;
					FrameError = false;
					RxState = 0;
					RxBreakState = false;
					RxBreakCounter = 0;
					TxState = 0;
					TxBreakState = false;

					RxD = true;
					RxC = true;
					TxD = true;
					OnTxDSet();
					TxC = false;
					_DSR = true;
					_CTS = true;
//					debug("_CTS=%d", _CTS);

					if (SyncMode)
						InitState = INIT_SYNC1;
					else {
						InitState = COMMAND_MODE;
						PrepareAsyncTx();
						RxState = WAIT_START;
					}

					break;

				case INIT_SYNC1 : // 1st sync character
					SyncChar1 = val;
					if ((CWR & SCS_MASK) == SCS_2)
						InitState = INIT_SYNC2;
					else {
						InitState = COMMAND_MODE;
						PrepareSyncTx();
						PrepareSyncRx(true, false);
					}
					break;

				case INIT_SYNC2 : // 2nd sync character
					SyncChar2 = val;
					InitState = COMMAND_MODE;
					PrepareSyncTx();
					PrepareSyncRx(true, false);
					break;

				case COMMAND_MODE : // command word
					BYTE oldCommand = Command;
					Command = val;

					if ((val & IR_MASK) == IR_RESET)  // internal reset
						InitState = INIT_MODE;
					else {
						// zeroing of error flags
						if ((val & ER_MASK) == ER_RESET) {
							ParityError = false;
							OverrunError = false;
							FrameError = false;
						}

						// transmitting BREAK
						if ((val & SBC_MASK) == SBC_BREAK) {
							TxBreakState = true;
							if (TxD) {
								TxD = false;
								OnTxDSet();
								OnTxDChange();
							}
						}
						else {
							// cancellation of BREAK transmission
							if (TxBreakState) {
								TxBreakState = false;
								TxD = true;
								OnTxDSet();
								OnTxDChange();
								if (SyncMode)
									PrepareSyncTx();
								else
									PrepareAsyncTx();
							}
						}

						// setting of DTR
						OnDtrSet();
						if ((oldCommand & DTR_MASK) ^ (Command & DTR_MASK))
							OnDtrChange();

						// setting of RTS
//						debug("!(Command & RTS_MASK)=%d", !(Command & RTS_MASK));
						OnRtsSet();
						if ((oldCommand & RTS_MASK) ^ (Command & RTS_MASK))
							OnRtsChange();

						// start of synchronization searching
						if (SyncMode  && (val & EHM_MASK) == EHM_ENABLED) {
							if ((CWR & ESD_MASK) == ESD_INTERNAL)
								RxState = SYNC_HUNT | SYNC_CHAR1;
							else
								RxState = SYNC_HUNT;
						}
					}
					break;
			}

			OnCwrWrite(InitState);
			break;

		case UR_DATA :
			if (InitState != COMMAND_MODE)
				return;

			TxChar = val;

			bool oldStatusTxE = StatusTxE;
			bool oldStatusTxR = StatusTxR;
			StatusTxE = false;
			StatusTxR = false;
			OnTxESet();
			if (oldStatusTxE)
				OnTxEChange();
			OnTxRSet();
			if (oldStatusTxR)
				OnTxRChange();
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * The CpuRead method is called during the execution of an IN instruction - read from
 * a port (CPU <- PIO).
 * It corresponds to bringing logic L level to inputs /CS (11) and /RD (13).
 * According to the type of source register and the state of the circuit,
 * a status word or contents of the data register are returned.
 *
 * @param src specifies the register (TUSARTReg) from which to read the data
 * @return value read from the register
 */
BYTE ChipUSART8251::CpuRead(TUSARTReg src)
{
	BYTE val = 0xFF;

	switch (src) {
		case UR_STATUS :
			val = (BYTE)((StatusTxR ? TXRDY_EMPTY : TXRDY_FULL)
					| (StatusRxR ? RXRDY_YES : RXRDY_NO)
					| (StatusTxE ? TXE_EMPTY : TXE_FULL)
					| (ParityError ? PE_YES : PE_NO)
					| (OverrunError ? OE_YES : OE_NO)
					| (FrameError ? FE_YES : FE_NO)
					| (SynDetState ? SYNDET_YES : SYNDET_NO)
					| (_DSR ? DSR_OFF : DSR_ON));

			if (SyncMode && (CWR & ESD_MASK) == ESD_INTERNAL && SynDetState) {
				SynDetState = false;
				OnSynDetSet();
				OnSynDetChange();
			}
			break;

		case UR_DATA :
			if (StatusRxR) {
				val = RxChar;
				StatusRxR = false;
				OnRxRSet();
				OnRxRChange();
			}
			break;
	}

	return val;
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to set the desired logic level on the RxD input (3).
 * In asynchronous mode, it evaluates the arrival of the start bit
 * and the termination of the Break state.
 *
 * @param state logic level applied to the RxD input
 */
void ChipUSART8251::PeripheralSetRxD(bool state)
{
	if (!SyncMode && RxState == WAIT_START && RxD && !state) {
		RxState = START_BIT;
		RxFactorCounter = (Factor > 1) ? Factor / 2 : Factor;
	}
	if (!SyncMode && state) {
		bool oldRxBreakState = RxBreakState;
		RxBreakState = false;
		OnBrkDetSet();
		if (oldRxBreakState)
			OnBrkDetChange();
		RxBreakCounter = 0;
	}

	RxD = state;
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level on the TxD output (19).
 * This output controls the transmitter.
 *
 * @return logic level on the TxD output
 */
bool ChipUSART8251::PeripheralReadTxD()
{
	return TxD;
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to control the transmitter clock signal, specifically,
 * to set the desired logic level on the _TxC input (9).
 * On the falling edge of this signal, the state of the TxD output changes.
 *
 * @param state logic level on the _TxC input
 */
void ChipUSART8251::PeripheralSetTxC(bool state)
{
	// bit change occurs at the falling edge of TxC --__
	bool oldTxC = TxC;
	TxC = state;
	if (InitState != COMMAND_MODE || TxBreakState || oldTxC == TxC || TxC)
		return;

	bool oldTxD = TxD;
	if (SyncMode) { // Synchronous mode
		switch (TxState & STATE_MASK) {
			case DATA_BITS :
				if ((_CTS || (Command & TEN_MASK) == TEN_DISABLED) && TxBitCounter == CharLen)
					return; // transmission is not enabled

				TxD = (TxShift & 1);
				TxShift >>= 1;
				if (--TxBitCounter == 0) {
					if ((CWR & PEN_MASK) == PEN_ENABLED)
						TxState = (TxState & SYNC_MASK) | PARITY_BIT;
					else
						PrepareSyncTx();
				}
				break;

			case PARITY_BIT :
				TxD = TxParity;
				PrepareSyncTx();
				break;
		}
	}
	else {  // Asynchronous mode
		if (--TxFactorCounter > 0)
			return;
		TxFactorCounter = Factor;

		switch (TxState) {
			case START_BIT :
				if (_CTS || (Command & TEN_MASK) == TEN_DISABLED)
					return; // transmission is not enabled

				TxD = false;
				TxState = DATA_BITS;
				break;

			case DATA_BITS :
				TxD = (TxShift & 1);
				TxShift >>= 1;
				if (--TxBitCounter == 0)
					TxState = ((CWR & PEN_MASK) == PEN_ENABLED) ? PARITY_BIT : STOP_BIT;
				break;

			case PARITY_BIT :
				TxD = TxParity;
				TxState = STOP_BIT;
				break;

			case STOP_BIT : // 1st Stop bit
				TxD = true;
				switch (CWR & SBL_MASK) {
					case SBL_15 :
						TxState = STOP_BIT15;
						break;

					case SBL_2 :
						TxState = STOP_BIT2;
						break;

					default :
						TxState = ASYNC_TX_IDLE;
						break;
				}
				break;

		case STOP_BIT15 : // 1.5 Stop bit
			if (TxFactorCounter > 1)
				TxFactorCounter /= 2;
			/* no break */
		case STOP_BIT2 :  // 2nd Stop bit
			TxD = true;
			TxState = ASYNC_TX_IDLE;
			break;

		case ASYNC_TX_IDLE :
			PrepareAsyncTx();
			if (TxState == START_BIT) {
				TxD = false;
				TxState = DATA_BITS;
			}
			break;
		}
	}

	OnTxDSet();
	if (oldTxD != TxD)
		OnTxDChange();
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to control the receiver clock signal, specifically,
 * to set the desired logic level on the _RxC input (25).
 * At the rising edge of this signal, the state of the RxD input is sampled.
 *
 * @param state logic level on the _RxC input
 */
void ChipUSART8251::PeripheralSetRxC(bool state)
{
	// bit change occurs at the rising edge of RxC __--
	bool oldRxC = RxC;
	RxC = state;
	if (InitState != COMMAND_MODE || oldRxC == RxC || !RxC)
		return;

	if (!SyncMode) { // Asynchronous mode
		if (--RxFactorCounter > 0)
		 return;
		RxFactorCounter = Factor;
	}

	switch (RxState & STATE_MASK) {
		case SYNC_HUNT :  // Synchronous mode
			if ((CWR & ESD_MASK) == ESD_INTERNAL) {
				RxBitCounter--;
				if (RxBitCounter == 0 && (CWR & PEN_MASK) == PEN_ENABLED) {
					if (RxParity == RxD)
						SyncDetected(true);
					else
						PrepareSyncRx(true, false);
					}
				else {
					if ((bool)(RxShift & 1) == RxD) {
						RxShift >>= 1;
						if (RxBitCounter == 0)
							SyncDetected(true);
					}
					else
						PrepareSyncRx(true, false);
				}
			}
			break;

		case WAIT_START : // Asynchronous mode
			if (!RxD && RxBreakCounter > 0) {
				RxState = START_BIT;
				RxFactorCounter = (Factor > 1) ? Factor / 2 : Factor;
			}
			break;

		case START_BIT :  // Asynchronous mode
			if (RxD)
				RxState = WAIT_START; // it was not a Start bit, just a false noise
			else {
				RxState = DATA_BITS;  // it was a valid Start bit
				RxShift = 0;          // data bits follow
				RxBitCounter = CharLen;
			}
			break;

		case DATA_BITS :  // Both modes
			RxShift >>= 1;
			if (RxD)
				RxShift |= 0x80;
//			debug("RxD=%d, RxBitCounter=%d, RxShift=%08X", RxD, RxBitCounter, RxShift);
			if (--RxBitCounter == 0) {
				if (CharLen < 8)
					RxShift >>= (8 - CharLen);
				if ((CWR & PEN_MASK) == PEN_ENABLED) {
					RxState = (RxState & SYNC_MASK) | PARITY_BIT;
					RxParity = CalculateParity(RxShift);
				}
				else {
					if (SyncMode) {
						if (((RxState & SYNC_MASK) == SYNC_CHAR1 && RxShift == SyncChar1)
						 || ((RxState & SYNC_MASK) == SYNC_CHAR2 && RxShift == SyncChar2))
							SyncDetected(false);

						InitRxBitCounter();
					}
					else
						RxState = STOP_BIT;
				}
			}
			break;

		case PARITY_BIT :
			if (RxD != RxParity)
				ParityError = true;
			if (SyncMode) {
				CharReceived();
				if (((RxState & SYNC_MASK) == SYNC_CHAR1 && RxShift == SyncChar1)
				 || ((RxState & SYNC_MASK) == SYNC_CHAR2 && RxShift == SyncChar2))
					SyncDetected(false);

				InitRxBitCounter();
			}
			else
				RxState = STOP_BIT;
			break;

		case STOP_BIT :
			if (!RxD) {                     // invalid Stop bit
				FrameError = true;          // frame termination error
				RxBreakCounter++;
				if (RxBreakCounter > 1) {   // two consecutive invalid Stop bits
					RxBreakCounter = 2;     // including Start, Data and Parity bits
					RxBreakState = true;    // means reception of BREAK state
					OnBrkDetSet();
					OnBrkDetChange();
				}
			}

			CharReceived();
			RxState = WAIT_START;
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level on the TxRDY output (15).
 * This output is at logic 1 only if the transmitter buffer register is empty,
 * transmission is enabled, and the input signal _CTS is at logic 0.
 *
 * @return logic level on the TxRDY output
 */
bool ChipUSART8251::PeripheralReadTxR()
{
	return (StatusTxR && (Command & TEN_MASK) == TEN_ENABLED && !_CTS);
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level on the TxEMPTY output (18).
 * This output is at logic 1 if both the buffer and shift registers of the transmitter
 * are empty.
 *
 * @return logic level on the TxEMPTY output
 */
bool ChipUSART8251::PeripheralReadTxE()
{
	return StatusTxE;
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level on the RxRDY output (14).
 * This output is at logic 1 if a serial character has been received into the
 * buffer register of the receiver.
 *
 * @return logic level on the RxRDY output
 */
bool ChipUSART8251::PeripheralReadRxR()
{
	return StatusRxR;
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to set the desired logic level on the _DSR input (22).
 *
 * @param state logic level applied to the _DSR input
 */
void ChipUSART8251::PeripheralSetDSR(bool state)
{
	_DSR = state;
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level on the _DTR output (24).
 * This output is set by the command word and has an inverted level.
 *
 * @return logic level on the _DTR output
 */
bool ChipUSART8251::PeripheralReadDTR()
{
	return !(Command & DTR_MASK);
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to set the desired logic level on the _CTS input (17).
 *
 * @param state logic level applied to the _CTS input
 */
void ChipUSART8251::PeripheralSetCTS(bool state)
{
	_CTS = state;
//	debug("_CTS=%d", _CTS);
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level on the _RTS output (23).
 * This output is set by the command word and has an inverted level.
 *
 * @return logic level on the _RTS output
 */
bool ChipUSART8251::PeripheralReadRTS()
{
	return !(Command & RTS_MASK);
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral device to change the logic level on the SYNDET input (16).
 * The change in value takes effect only if external synchronization is set in sync mode.
 * The rising edge of the pulse __-- marks the beginning of a character.
 *
 * @param state logic level applied to the SYNDET input
 */
void ChipUSART8251::PeripheralSetSynDet(bool state)
{
	if (SyncMode && (CWR & ESD_MASK) == ESD_EXTERNAL) {
		if (SynDetState == state)
			return;
		SynDetState = state;
		if (state)
			PrepareSyncRx(false, false);
	}
}
//---------------------------------------------------------------------------
/**
 * Allows the peripheral to determine the logic level of the SYNDET/BRKDET output.
 * In synchronous mode, it corresponds to the state of internal synchronization.
 * True means that one or both synchronization characters have been received.
 * In asynchronous mode, it corresponds to the Break state, when two consecutive
 * stop bits (including start, data, and parity bits) are at logic 0.
 *
 * @return level of the SYNDET/BRKDET output
 */
bool ChipUSART8251::PeripheralReadSynBrk()
{
	if (SyncMode && (CWR & ESD_MASK) == ESD_INTERNAL)
		return SynDetState;
	else if (!SyncMode)
		return RxBreakState;

	return false;
}
//---------------------------------------------------------------------------
void ChipUSART8251::SetByteTransferMode(bool byteTransferMode)
{
	ByteTransferMode = byteTransferMode;
}
//---------------------------------------------------------------------------
void ChipUSART8251::PeripheralWriteByte(BYTE value)
{
	RxShift = value;
	CharReceived();
}
//---------------------------------------------------------------------------
BYTE ChipUSART8251::PeripheralReadByte()
{
	if (!StatusTxR) {
		if (ByteTransferMode) {
			StatusTxR = true;
			OnTxRSet();
			OnTxRChange();
			if (SyncMode)
				PrepareSyncTx();
			else
				PrepareAsyncTx();
		}

		return TxChar;
	}

	return 0;
}
//---------------------------------------------------------------------------
/**
 * Calculates the parity of the character 'value'.
 * Parity is calculated from 'CharLen' bits of this value and the returned value
 * corresponds to the required parity. It therefore has a value
 * that needs to be transmitted or is expected at reception.
 *
 * @param value character for which parity is to be calculated
 * @return calculated parity
 */
bool ChipUSART8251::CalculateParity(BYTE value)
{
	BYTE p = 0;

	for (int ii = 0; ii < CharLen; ii++) {
		p ^= (BYTE)(value & 1);
		value >>= 1;
	}

	if ((CWR & PT_MASK) == PT_EVEN)
		return (p & 1);

	return !(p & 1);
}
//---------------------------------------------------------------------------
/**
 * Prepares the internal transmitter variables for transmission of the next character,
 * if the CPU has written it to the transmitter buffer register.
 * Otherwise, it sets the transmitter to Idle state.
 */
void ChipUSART8251::PrepareAsyncTx()
{
	if (!StatusTxR) {
		TxState = START_BIT;  // character is ready for transmission
		TxShift = TxChar;
		TxParity = CalculateParity(TxShift);
		TxBitCounter = CharLen;
		StatusTxR = true;
		OnTxRSet();
		OnTxRChange();
	}
	else {  // nothing to transmit
		StatusTxE = true;
		OnTxESet();
		OnTxEChange();

		TxState = ASYNC_TX_IDLE;
	}

	TxFactorCounter = Factor;
}
//---------------------------------------------------------------------------
/**
 * Based on the state of variables TxState, CWR, and StatusTxR,
 * prepares transmission of the next character in synchronous mode.
 * This can be a character prepared in TxChar or one of the synchronization characters.
 * StatusTxE is set to true if there is nothing more to transmit (only sync characters).
 * At the same time, parity (TxParity) of the prepared character in the shift register
 * is calculated and TxBitCounter is set.
 */
void ChipUSART8251::PrepareSyncTx()
{
	if ((TxState & SYNC_MASK) == SYNC_CHAR1 && (CWR & SCS_MASK) == SCS_2) {
		TxState = SYNC_CHAR2 | DATA_BITS; // transmission of 2nd sync character
		TxShift = SyncChar2;              // move to shift register
	}
	else if (!StatusTxR) {
		TxState = DATA_BITS;  // character is ready for transmission
		TxShift = TxChar;
		StatusTxR = true;
		OnTxRSet();
		OnTxRChange();
	}
	else {  // nothing to transmit, sync characters will be transmitted
		TxState = SYNC_CHAR1 | DATA_BITS;
		TxShift = SyncChar1;
		StatusTxE = true;
		OnTxESet();
		OnTxEChange();
	}

	TxParity = CalculateParity(TxShift);
	TxBitCounter = CharLen;
}
//---------------------------------------------------------------------------
void ChipUSART8251::PrepareSyncRx(bool hunt, bool sync2)
{
	if (hunt) {
		if ((CWR & ESD_MASK) == ESD_INTERNAL) {
			if (sync2) {
				RxState = SYNC_HUNT | SYNC_CHAR2;
				RxShift = SyncChar2;
			}
			else {
				RxState = SYNC_HUNT | SYNC_CHAR1;
				RxShift = SyncChar1;
			}
			RxParity = CalculateParity(RxShift);
			InitRxBitCounter();
		}
		else
			RxState = SYNC_HUNT;

		if (StatusRxR) {
			StatusRxR = false;
			OnRxRSet();
			OnRxRChange();
		}
	}
	else {
		if (sync2)
			RxState = DATA_BITS | SYNC_CHAR2;
		else
			RxState = DATA_BITS | SYNC_CHAR1;
		InitRxBitCounter();
		RxShift = 0;
	}
}
//---------------------------------------------------------------------------
void ChipUSART8251::SyncDetected(bool inHunt)
{
	if ((RxState & SYNC_CHAR1) && (CWR & SCS_MASK) == SCS_2)
		PrepareSyncRx(inHunt, true);
	else {
		PrepareSyncRx(false, false);
		bool oldSynDetState = SynDetState;
		SynDetState = true;
		OnSynDetSet();
		if (!oldSynDetState)
			OnSynDetChange();
	}
}
//---------------------------------------------------------------------------
void ChipUSART8251::CharReceived()
{
	bool oldStatusRxR = StatusRxR;
	RxChar = RxShift;        // received character
	if (StatusRxR)
		OverrunError = true; // CPU did not read the previous character
	StatusRxR = true;
	OnRxRSet();
	if (!oldStatusRxR)
		OnRxRChange();
}
//---------------------------------------------------------------------------
void ChipUSART8251::InitRxBitCounter()
{
	RxBitCounter = CharLen;
	if ((CWR & PEN_MASK) == PEN_ENABLED)
		RxBitCounter++;
}
//---------------------------------------------------------------------------
