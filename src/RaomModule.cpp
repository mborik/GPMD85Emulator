/*	RaomModule.cpp: Class for emulation of RAOM modules
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
#include "RaomModule.h"
//---------------------------------------------------------------------------
RaomModule::RaomModule(TRaomType type) : ChipPIO8255(false)
{
	this->type = type;
	hFile = NULL;

	switch (type) {
		case RT_CHTF:
			romSize = CHTF_ROM_SIZE;
			ramSize = CHTF_RAM_SIZE;
			break;

		case RT_KUVI:
			romSize = KUVI_ROM_SIZE;
			ramSize = KUVI_RAM_SIZE;
			break;
	}

	RomPack = new BYTE[romSize];
	memset(RomPack, 0xFF, romSize);

	RamBlock = new BYTE[RAM_BLOCK_SIZE];
	memset(RamBlock, 0, RAM_BLOCK_SIZE);

	ramAddr = 0;
	writeRamBlock = false;
	writeProtect = false;
}
//---------------------------------------------------------------------------
RaomModule::~RaomModule()
{
	RemoveDisk();

	if (RomPack)
		delete RomPack;
	if (RamBlock)
		delete RamBlock;
}
//---------------------------------------------------------------------------
// metody zdedene z triedy PeripheralDevice
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri jeho resete.
 */
void RaomModule::ResetDevice(int ticks)
{
	ChipReset(false);
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty RaomModulu.
 */
void RaomModule::WriteToDevice(BYTE port, BYTE value, int ticks)
{
	switch (port & RAOM_REG_MASK) {
		case RAOM_REG_DATA_CHTF:
		case RAOM_REG_DATA_KUVI:
			if (hFile == NULL || writeProtect == true)
				return;

			ComposeAdddress();
			if (type == RT_CHTF) {
				if (addr < CHTF_ROM_SIZE)
					return;
				addr -= CHTF_ROM_SIZE;
			}

			if (addr >= ramSize)
				return;
			if (!(addr >= ramAddr && addr < (ramAddr + RAM_BLOCK_SIZE))) {
				WriteBlock();
				ramAddr = (addr & ~(RAM_BLOCK_SIZE - 1));
				ReadBlock();
			}
			addr &= (RAM_BLOCK_SIZE - 1);
			*(RamBlock + addr) = value;
			writeRamBlock = true;
			break;

		case RAOM_REG_LOW_CHTF:
		case RAOM_REG_HIGH_KUVI:
			CpuWrite(PP_PortA, value);
			break;

		case RAOM_REG_MID_CHTF:
		case RAOM_REG_LOW_KUVI:
			CpuWrite(PP_PortB, value);
			break;

		case RAOM_REG_HIGH_CHTF:
		case RAOM_REG_MID_KUVI:
			CpuWrite(PP_PortC, value);
			break;

		case RAOM_REG_CWR_CHTF:
		case RAOM_REG_CWR_KUVI:
			cwr = value;
			CpuWrite(PP_CWR, value);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri citani z portov RaomModulu.
 */
BYTE RaomModule::ReadFromDevice(BYTE port, int ticks)
{
	BYTE retval;

	switch (port & RAOM_REG_MASK) {
		case RAOM_REG_DATA_CHTF:
		case RAOM_REG_DATA_KUVI:
			ComposeAdddress();
			if (type == RT_CHTF) {
				if (addr < CHTF_ROM_SIZE)
					return *(RomPack + addr);

				addr -= CHTF_ROM_SIZE;
			}

			if (addr >= ramSize || hFile == NULL)
				return 0xFF;

			if (!(addr >= ramAddr && addr < (ramAddr + RAM_BLOCK_SIZE))) {
				WriteBlock();
				ramAddr = (addr & ~(RAM_BLOCK_SIZE - 1));
				ReadBlock();
			}
			addr &= (RAM_BLOCK_SIZE - 1);
			retval = *(RamBlock + addr);
			break;

		case RAOM_REG_LOW_CHTF:
			retval = CpuRead(PP_PortA);
			break;

		case RAOM_REG_ROMD_KUVI:
			ComposeAdddress();

			if (addr < romSize)
				retval = *(RomPack + addr);
			else
				retval = 0xFF;
			break;

		case RAOM_REG_MID_CHTF:
		case RAOM_REG_LOW_KUVI:
			retval = CpuRead(PP_PortB);
			break;

		case RAOM_REG_HIGH_CHTF:
		case RAOM_REG_MID_KUVI:
			retval = CpuRead(PP_PortC);
			break;

		case RAOM_REG_CWR_CHTF:
		case RAOM_REG_CWR_KUVI:
			retval = CpuRead(PP_CWR);
			break;

		default:
			retval = 0xFF;
			break;
	}

	return retval;
}
//---------------------------------------------------------------------------
void RaomModule::ComposeAdddress()
{
	if (type == RT_CHTF) {
		addr = PeripheralReadByte(PP_PortA);
		addr |= PeripheralReadByte(PP_PortB) << 8;
		addr |= PeripheralReadByte(PP_PortC) << 16;
		if ((cwr & (GA_MODE | PORTA_DIR)) == (GA_MODE0 | PORTA_INP))
			addr >>= 8;
	}
	else {
		addr = PeripheralReadByte(PP_PortB);
		addr |= PeripheralReadByte(PP_PortC) << 8;
		if ((cwr & (GA_MODE | PORTA_DIR)) == (GA_MODE0 | PORTA_OUT))
			addr |= PeripheralReadByte(PP_PortA) << 16;
	}
}
//---------------------------------------------------------------------------
bool RaomModule::InsertRom(BYTE addressKB, BYTE sizeKB, BYTE *src)
{
	if (addressKB >= (romSize / KB) || sizeKB == 0 || (addressKB + sizeKB)
			> (romSize / KB))
		return false;

	memcpy(RomPack + addressKB * KB, src, sizeKB * KB);
	return true;
}
//---------------------------------------------------------------------------
void RaomModule::RemoveRom(BYTE addressKB, BYTE sizeKB)
{
	if (addressKB >= (romSize / KB) || sizeKB == 0)
		return;

	if (addressKB + sizeKB > (romSize / KB))
		sizeKB = (BYTE) ((romSize / KB) - addressKB);
	memset(RomPack + (addressKB * KB), 0xFF, (sizeKB * KB));
}
//---------------------------------------------------------------------------
void RaomModule::RemoveRomPack()
{
	memset(RomPack, 0xFF, romSize);
}
//---------------------------------------------------------------------------
void RaomModule::InsertDisk(char *file)
{
	RemoveDisk();
	hFile = fopen(file, "rb");
	ramAddr = 0;
	writeRamBlock = false;
	ReadBlock();
}
//---------------------------------------------------------------------------
void RaomModule::RemoveDisk()
{
	if (hFile != NULL) {
		if (writeRamBlock == true)
			WriteBlock();

		fclose(hFile);
		hFile = NULL;
	}
}
//---------------------------------------------------------------------------
void RaomModule::ReadBlock()
{
	if (hFile == NULL)
		return;

	fseek(hFile, ramAddr, SEEK_SET);
	if (ftell(hFile) < 0) {
		memset(RamBlock, 0xAA, RAM_BLOCK_SIZE);
		return;
	}

	dwRW = fread(RamBlock, sizeof(BYTE), RAM_BLOCK_SIZE, hFile);
	if (dwRW == 0) {
		memset(RamBlock, 0xCC, RAM_BLOCK_SIZE);
		return;
	}

	if (dwRW < RAM_BLOCK_SIZE)
		memset(RamBlock + dwRW, 0x55, RAM_BLOCK_SIZE - dwRW);
}
//---------------------------------------------------------------------------
void RaomModule::WriteBlock()
{
	if (hFile == NULL)
		return;

	if (writeRamBlock == false)
		return;

	fseek(hFile, ramAddr, SEEK_SET);
	if (ftell(hFile) < 0)
		return;

	dwRW = fwrite(RamBlock, sizeof(BYTE), RAM_BLOCK_SIZE, hFile);
	writeRamBlock = false;
}
//---------------------------------------------------------------------------
