/*  IifGPIO.h: Class for emulation of base GPIO interface
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
#ifndef IifGPIOH
#define IifGPIOH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIO8255.h"
//---------------------------------------------------------------------------
#define IIF_GPIO_MASK       0xFC
#define IIF_GPIO_ADR        0x4C

#define IIF_GPIO_REG_MASK   0xFF
#define IIF_GPIO_REG_A      0x4C
#define IIF_GPIO_REG_B      0x4D
#define IIF_GPIO_REG_C      0x4E
#define IIF_GPIO_REG_CWR    0x4F
//---------------------------------------------------------------------------
class IifGPIO: public PeripheralDevice, public ChipPIO8255
{
public:
	IifGPIO();

	virtual void resetDevice(int ticks);
	virtual void writeToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE readFromDevice(BYTE port, int ticks);

	sigslot::signal0<> OnBeforeResetA;
	sigslot::signal0<> OnBeforeResetB;
	sigslot::signal0<> OnAfterResetA;
	sigslot::signal0<> OnAfterResetB;

	void WriteByte(TPIOPort dest, BYTE val);
	BYTE ReadByte(TPIOPort src);
	void ChangeBit(TPIOPort dest, TPIOPortBit bit, bool state);
	bool ReadBit(TPIOPort src, TPIOPortBit bit);

	inline int GetCurrentTicks() { return currentTicks; }

private:
	int currentTicks;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

