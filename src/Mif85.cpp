/*	Mif85.cpp: Class for emulation of sound interface MIF 85
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
