/*	RomModule.cpp: Class for emulation of plugged ROM module
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
#include "RomModule.h"
//---------------------------------------------------------------------------
RomModule::RomModule() : ChipPIO8255(false)
{
	RomPack = new BYTE[ROM_PACK_SIZE];
	memset(RomPack, 0xFF, ROM_PACK_SIZE);
	OnCpuReadA.connect(this, &RomModule::ReadFromRom);
}
//---------------------------------------------------------------------------
RomModule::~RomModule()
{
	if (RomPack)
		delete RomPack;
}
//---------------------------------------------------------------------------
// metody zdedene z triedy PeripheralDevice
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri jeho resete.
 */
void RomModule::resetDevice(int ticks)
{
	ChipReset(false);
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty PIO RomModulu.
 */
void RomModule::writeToDevice(BYTE port, BYTE value, int ticks)
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
 * Metoda je volana procesorom pri citani z portov PIO RomModulu.
 */
BYTE RomModule::readFromDevice(BYTE port, int ticks)
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

	if (addr & 0x8000)
		PeripheralWriteByte(PP_PortA, 0xFF);
	else
		PeripheralWriteByte(PP_PortA, *(RomPack + addr));
}
//---------------------------------------------------------------------------
