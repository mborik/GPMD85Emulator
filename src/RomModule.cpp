/*	RomModule.cpp: Class for emulation of plugged ROM module
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
#include "RomModule.h"
//---------------------------------------------------------------------------
RomModule::RomModule() : ChipPIO8255(false)
{
	RomPack = new BYTE[ROM_PACK_SIZE];
	memset(RomPack, 0xFF, ROM_PACK_SIZE);
	OnCpuReadA.connect(&RomModule::ReadFromRom, this);
}
//---------------------------------------------------------------------------
RomModule::~RomModule()
{
	if (RomPack)
		delete [] RomPack;
}
//---------------------------------------------------------------------------
// methods inherited from the PeripheralDevice class
//---------------------------------------------------------------------------
/**
 * Method is called by the processor during its reset.
 */
void RomModule::ResetDevice(int ticks)
{
	ChipReset(false);
}
//---------------------------------------------------------------------------
/**
 * Method is called by the processor when writing to the PIO ports of the RomModule.
 */
void RomModule::WriteToDevice(BYTE port, BYTE value, int ticks)
{
	switch (port & ROM_MODULE_REG_MASK) {
		case ROM_MODULE_REG_A:
			CpuWrite(PP_PortA, value);
			break;

		case ROM_MODULE_REG_B:
			CpuWrite(PP_PortB, value);
			break;

		case ROM_MODULE_REG_C:
			CpuWrite(PP_PortC, value);
			break;

		case ROM_MODULE_REG_CWR:
			CpuWrite(PP_CWR, value);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Method is called by the processor when reading from the PIO ports of the RomModule.
 */
BYTE RomModule::ReadFromDevice(BYTE port, int ticks)
{
	BYTE retval;

	switch (port & ROM_MODULE_REG_MASK) {
		case ROM_MODULE_REG_A:
			retval = CpuRead(PP_PortA);
			break;

		case ROM_MODULE_REG_B:
			retval = CpuRead(PP_PortB);
			break;

		case ROM_MODULE_REG_C:
			retval = CpuRead(PP_PortC);
			break;

		case ROM_MODULE_REG_CWR:
			retval = CpuRead(PP_CWR);
			break;

		default:
			retval = 0xFF;
			break;
	}

	return retval;
}
//---------------------------------------------------------------------------
bool RomModule::InsertRom(BYTE addressKB, BYTE sizeKB, BYTE *src)
{
	if (addressKB >= ROM_PACK_SIZE_KB || sizeKB == 0 || (addressKB + sizeKB)
			> ROM_PACK_SIZE_KB)
		return false;

	memcpy(RomPack + addressKB * 1024, src, sizeKB * 1024);
	return true;
}
//---------------------------------------------------------------------------
void RomModule::RemoveRom(BYTE addressKB, BYTE sizeKB)
{
	if (addressKB >= ROM_PACK_SIZE_KB || sizeKB == 0)
		return;
	if (addressKB + sizeKB > ROM_PACK_SIZE_KB)
		sizeKB = (BYTE) (ROM_PACK_SIZE_KB - addressKB);
	memset(RomPack + addressKB * 1024, 0xFF, sizeKB * 1024);
}
//---------------------------------------------------------------------------
void RomModule::RemoveRomPack()
{
	memset(RomPack, 0xFF, ROM_PACK_SIZE);
}
//---------------------------------------------------------------------------
void RomModule::ReadFromRom()
{
	WORD addr;

	addr = PeripheralReadByte(PP_PortB);
	addr |= (WORD) (PeripheralReadByte(PP_PortC) << 8);

	if ((addr & 0x8000) || RomPack == NULL)
		PeripheralWriteByte(PP_PortA, 0xFF);
	else
		PeripheralWriteByte(PP_PortA, *(RomPack + addr));
}
//---------------------------------------------------------------------------
