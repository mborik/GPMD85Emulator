/*	Mif85.h: Class for emulation of sound interface MIF 85
	Copyright (c) 2008-2014 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2019 Martin Borik <mborik@users.sourceforge.net>

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
