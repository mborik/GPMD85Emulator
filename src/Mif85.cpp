/*	Mif85.cpp: Class for emulation of sound interface MIF 85
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
#include "Mif85.h"
//---------------------------------------------------------------------------
CSAASound *SAA1099 = NULL;
//---------------------------------------------------------------------------
Mif85::Mif85()
{
	SAA1099 = CreateCSAASound();
	SAA1099->SetSoundParameters(SAAP_NOFILTER | SAAP_44100 | SAAP_8BIT | SAAP_STEREO);
}
//---------------------------------------------------------------------------
Mif85::~Mif85()
{
	if (SAA1099)
		delete SAA1099;
	SAA1099 = NULL;
}
//---------------------------------------------------------------------------
void Mif85::ResetDevice(int ticks)
{
	intEna = false;
	SAA1099->Clear();

	memset(regs, 0, 32);
	lastReg = 0;
}
//---------------------------------------------------------------------------
void Mif85::WriteToDevice(BYTE port, BYTE val, int /* ticks */)
{
	switch (port & MIF85_REG_MASK) {
		case MIF85_REG_ADR:
			SAA1099->WriteAddress(val);
			lastReg = val & 0x1F;
			break;

		case MIF85_REG_DATA:
			SAA1099->WriteData(val);
			regs[lastReg] = val;
			break;

		case MIF85_REG_INT:
			intEna = (val & 1) ? true : false;
			break;
	}
}
//---------------------------------------------------------------------------
BYTE Mif85::ReadFromDevice(BYTE /* port */, int /* ticks */)
{
	return 0xFF;
}
//---------------------------------------------------------------------------
int Mif85::GetDeviceState(BYTE *buffer)
{
	if (buffer != NULL)
		memcpy(buffer, regs, 32);
	return 32;
}
//---------------------------------------------------------------------------
void Mif85::SetDeviceState(BYTE *buffer, bool intEnabled)
{
	if (buffer != NULL) {
		memcpy(regs, buffer, 32);

		for (int ii = 0; ii < 32; ii++) {
			SAA1099->WriteAddress((BYTE) ii);
			SAA1099->WriteData(regs[ii]);
		}

		intEna = intEnabled;
	}
}
//---------------------------------------------------------------------------
