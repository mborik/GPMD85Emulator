/*	Mif85.h: Class for emulation of sound interface MIF 85
	Copyright (c) 2008-2014 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2019 Martin Borik <martin@borik.net>

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
#ifndef Mif85H
#define Mif85H
//---------------------------------------------------------------------------
#include "CommonUtils.h"
#include "ChipCpu8080.h"
#include "SAASound.h"
//---------------------------------------------------------------------------
// real device MIF 85 using these ports: #EC, #EE & #EF
#define MIF85_MASK            0xFC
#define MIF85_ADR             0xEC

#define MIF85_REG_MASK        0xFF
#define MIF85_REG_INT         0xEC
#define MIF85_REG_DATA        0xEE
#define MIF85_REG_ADR         0xEF
//---------------------------------------------------------------------------
class Mif85 : public PeripheralDevice
{
public:
	Mif85();
	virtual ~Mif85();

	virtual void ResetDevice(int ticks);
	virtual void WriteToDevice(BYTE port, BYTE val, int ticks);
	virtual BYTE ReadFromDevice(BYTE port, int ticks);

	inline bool InterruptEnabled() { return intEna; }

	int  GetDeviceState(BYTE *buffer);
	void SetDeviceState(BYTE *buffer, bool intEnabled);

private:
	bool intEna;
	BYTE regs[32];
	int lastReg;
};
//---------------------------------------------------------------------------
extern CSAASound *SAA1099;
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
