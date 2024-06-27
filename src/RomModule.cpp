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
void RomModule::ResetDevice(int ticks)
{
	ChipReset(false);
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty PIO RomModulu.
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
 * Metoda je volana procesorom pri citani z portov PIO RomModulu.
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
//---------------------------------------------------------------------------
MegaModule::MegaModule()
{
    // debug("MegaModule", "ctor");
	page = 0;
	RomPages[0] = new BYTE[ROM_PACK_SIZE];
	memset(RomPages[0], 0xFF, ROM_PACK_SIZE);
	RomPack = RomPages[0];
	for (int i = 1; i < MEGA_MODULE_MAX_PAGES; i++)
		RomPages[i] = NULL;
	OnCpuReadA.connect(this, &MegaModule::ReadFromRom);
}
//---------------------------------------------------------------------------
MegaModule::~MegaModule()
{
    // debug("MegaModule", "dtor");
	for (int i = 0; i < MEGA_MODULE_MAX_PAGES; i++) {
		if (RomPages[i] != NULL) {
			delete [] RomPages[i];
			RomPages[i] = NULL;
		}
	}
	RomPack = NULL;
}
//---------------------------------------------------------------------------
void MegaModule::ResetDevice(int ticks)
{
    debug("MegaModule", "reset");
	RomModule::ResetDevice(ticks);
	page = 0;
	RomPack = RomPages[0];
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty PIO RomModulu.
 */
void MegaModule::WriteToDevice(BYTE port, BYTE value, int ticks)
{
	if ((port & MEGA_MODULE_MASK) == MEGA_MODULE_ADR) {
		debug("MegaModule", "Selected ROM module No.%d", value);

		page = value;
		RomPack = RomPages[page];
	}
	else
		RomModule::WriteToDevice(port, value, ticks);
}
//---------------------------------------------------------------------------
bool MegaModule::LoadRom(unsigned int size, BYTE *src)
{
	int remain = size;
	int toCopy;

	if (size == 0 || size > MEGA_MODULE_MAX_PAGES * ROM_PACK_SIZE)
		return false;

	for (int i = 0; i < MEGA_MODULE_MAX_PAGES; i++)
	{
		if (RomPages[i] == NULL) {
			RomPages[i] = new BYTE[ROM_PACK_SIZE];
		}
		memset(RomPages[i], 0xFF, ROM_PACK_SIZE);
		toCopy = remain > ROM_PACK_SIZE ? ROM_PACK_SIZE : remain;
		memcpy(RomPages[i], src + i * ROM_PACK_SIZE, toCopy);
		debug("MegaModule", "ROM module %d loaded (%d bytes)", i, toCopy);

		remain -= toCopy;
		if (remain == 0)
			break;
	}
	return true;
}
//---------------------------------------------------------------------------
void MegaModule::ReadFromRom()
{
	RomModule::ReadFromRom();
}
//---------------------------------------------------------------------------
