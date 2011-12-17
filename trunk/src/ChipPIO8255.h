/*	ChipPIO8255.h: Class for emulation of PIO 8255 chip
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

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
#ifndef ChipPIO8255H
#define ChipPIO8255H
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
enum TPIOPort { PP_PortA = 0, PP_PortB, PP_PortC, PP_CWR, PP_PortCH, PP_PortCL };
enum TPIOPortBit { PP_Bit0 = 0, PP_Bit1, PP_Bit2, PP_Bit3, PP_Bit4, PP_Bit5, PP_Bit6, PP_Bit7 };
//---------------------------------------------------------------------------
#define CWR_MASK      0x80  // maska zapisu CWR

// skupina A
#define GA_MODE       0x60
#define GA_MODE0      0x00
#define GA_MODE1      0x20
#define GA_MODE2      0x40
#define PORTA_DIR     0x10
#define PORTA_INP     0x10
#define PORTA_OUT     0x00
#define PORTCH_DIR    0x08
#define PORTCH_INP    0x08
#define PORTCH_OUT    0x00

// skupina B
#define GB_MODE       0x04
#define GB_MODE0      0x00
#define GB_MODE1      0x04
#define PORTB_DIR     0x02
#define PORTB_INP     0x02
#define PORTB_OUT     0x00
#define PORTCL_DIR    0x01
#define PORTCL_INP    0x01
#define PORTCL_OUT    0x00

// nastavenie CWR po resete - vsetky porty su v mode 0 ako vstupy
#define BASIC_CWR     (CWR_MASK | GA_MODE0 | PORTA_INP | PORTCH_INP | GB_MODE0 | PORTB_INP | PORTCL_INP)
//---------------------------------------------------------------------------
#define _STBA         PP_Bit4         // Strobe A - vstup
#define _STBA_MASK    (1 << _STBA)
#define IBFA          PP_Bit5         // Input buffer A full - vystup
#define IBFA_MASK     (1 << IBFA)
#define _OBFA         PP_Bit7         // Output buffer A full - vystup
#define _OBFA_MASK    (1 << _OBFA)
#define _ACKA         PP_Bit6         // Acknowledge A - vstup
#define _ACKA_MASK    (1 << _ACKA)
#define INTRA         PP_Bit3         // Interrupt request A - vystup
#define INTRA_MASK    (1 << INTRA)
#define INTEAIN       PP_Bit4         // Interrupt enable A input - status
#define INTEAIN_MASK  (1 << INTEAIN)
#define INTEAOUT      PP_Bit6         // Interrupt enable A output - status
#define INTEAOUT_MASK (1 << INTEAOUT)

#define _STBB         PP_Bit2         // Strobe B - vstup
#define _STBB_MASK    (1 << _STBB)
#define IBFB          PP_Bit1         // Input buffer B full - vystup
#define IBFB_MASK     (1 << IBFB)
#define _OBFB         PP_Bit1         // Output buffer B full - vystup
#define _OBFB_MASK    (1 << _OBFB)
#define _ACKB         PP_Bit2         // Acknowledge B - vstup
#define _ACKB_MASK    (1 << _ACKB)
#define INTRB         PP_Bit0         // Interrupt request B - vystup
#define INTRB_MASK    (1 << INTRB)
#define INTEB         PP_Bit2         // Interrupt enable B - status
#define INTEB_MASK    (1 << INTEB)
//---------------------------------------------------------------------------
class ChipPIO8255 : public sigslot::has_slots<>
{
public:
	// konstruktor
	ChipPIO8255(bool reset);

	int GetChipState(BYTE *buffer);
	void SetChipState(BYTE *buffer);

	// notifikacne funkcie
	sigslot::signal0<> OnCpuReadA;
	sigslot::signal0<> OnCpuReadB;
	sigslot::signal0<> OnCpuReadC;
	sigslot::signal0<> OnCpuReadCH;
	sigslot::signal0<> OnCpuReadCL;

	sigslot::signal0<> OnCpuWriteA;
	sigslot::signal0<> OnCpuWriteB;
	sigslot::signal0<> OnCpuWriteC;
	sigslot::signal0<> OnCpuWriteCH;
	sigslot::signal0<> OnCpuWriteCL;

	sigslot::signal0<> OnCpuReadCWR;
	sigslot::signal1<BYTE> OnCpuWriteCWR;

protected:
	// strana CPU
	void ChipReset(bool clearNotifyFunc);
	void CpuWrite(TPIOPort dest, BYTE val);
	BYTE CpuRead(TPIOPort src);

	// strana periferie (portov)
	void PeripheralWriteByte(TPIOPort dest, BYTE val);
	BYTE PeripheralReadByte(TPIOPort src);
	void PeripheralChangeBit(TPIOPort dest, TPIOPortBit bit, bool state);
	bool PeripheralReadBit(TPIOPort src, TPIOPortBit bit);

private:
	void NotifyOnWritePortC(BYTE oldVal, BYTE newVal);
	void ClearAllNotifyFunctions();

	// interne registre
	BYTE CWR;
	BYTE InBufferA;
	BYTE InLatchA;
	BYTE OutLatchA;
	BYTE InBufferB;
	BYTE InLatchB;
	BYTE OutLatchB;
	BYTE InBufferC;
	BYTE OutLatchC;

	// INTE klopne obvody (Flip-Flop)
	// povolenie prerusenia v modoch 1 a 2
	bool InteAin;  // PC4
	bool InteAout; // PC6
	bool InteB;    // PC2
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
