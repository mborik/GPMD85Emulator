/*	IifGPIO.h: Class for emulation of base GPIO interface
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom
	the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
	OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
	OR OTHER DEALINGS IN THE SOFTWARE.
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

	virtual void ResetDevice(int ticks);
	virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE ReadFromDevice(BYTE port, int ticks);

	sigslot::signal<> OnBeforeResetA;
	sigslot::signal<> OnBeforeResetB;
	sigslot::signal<> OnAfterResetA;
	sigslot::signal<> OnAfterResetB;

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

