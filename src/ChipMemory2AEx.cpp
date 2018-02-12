/*  ChipMemory2AEx.h: Derived class for memory management and
        peripheral device handling of memory expansion for Model 2A.
    Copyright (c) 2015-2016 Roman Borik <pmd85emu@gmail.com>

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
#include "ChipMemory2AEx.h"
//---------------------------------------------------------------------------
ChipMemory2AEx::ChipMemory2AEx(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	mem256 = true;
	sizeRAM = 256 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
}
//---------------------------------------------------------------------------
void ChipMemory2AEx::ResetOn()
{
	// reset happen...
	resetState = true;

	bank = 0;
	map = 0;
	vram2 = false;

	int i;
	// at first, set RAM into whole addressing space
	for (i = 0; i < 8; i++)
		FillPointers(i & 1, i >> 1, i >> 1);

	// <0000h, 0FFFh> - ROM
	// <2000h, 2FFFh> - mirrored ROM or 2nd hald of 8kB ROM
	for (i = 0; i < 4; i++) {
		pointers[1][i] = memROM + i * 1024;
		pointers[1][i + 8] = memROM + i * 1024 + ((split8k) ? 4096 : 0);
	}

	if (split8k) {
		// <8000h, 8FFFh> - ROM
		for (i = 32; i < (32 + 4); i++)
			pointers[1][i] = memROM + (i - 32) * 1024;
		// <A000h, AFFFh> - 2nd half of 8kB ROM
		for (i = 40; i < (40 + (sizeRomKB - 4)); i++)
			pointers[1][i] = memROM + (i - 40) * 1024 + 4096;
	}
	else {
		// <8000h, 8FFFh> - ROM
		// <A000h, AFFFh> - mirrored ROM if length is less than 8kB
		for (i = 32; i < (32 + sizeRomKB); i++) {
			pointers[1][i] = memROM + (i - 32) * 1024;
			if (sizeRomKB <= 8)
				pointers[1][i + 8] = memROM + (i - 32) * 1024;
		}
	}
}
//---------------------------------------------------------------------------
void ChipMemory2AEx::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;

	bank = 0;
	map = 0;
	vram2 = false;

	// RAM space instead of ROM paged after reset
	FillPointers(1, 0, 0);
	FillPointers(1, 1, 1);
}
//---------------------------------------------------------------------------
BYTE* ChipMemory2AEx::GetVramPointer()
{
	return memRAM + (vram2 ? 7 : 3) * 16384;
}
//---------------------------------------------------------------------------
BYTE ChipMemory2AEx::GetPage()
{
	return (BYTE) (bank | (map << 4) | (vram2 << 6));
}
//---------------------------------------------------------------------------
void ChipMemory2AEx::SetPage(BYTE pg)
{
	// revert initial bank onto previous space
	FillPointers(0, map, map);
	FillPointers(1, map, map);

	// new mapping set
	bank = (pg & 0x0F);
	map = (pg & 0x30) >> 4;
	vram2 = (pg & 0x40);

	// map selected bank onto selected space
	FillPointers(0, map, bank);
	FillPointers(1, map, bank);
}
//---------------------------------------------------------------------------
void ChipMemory2AEx::FillPointers(int part, int map, int bank)
{
	int i;
	if (part == 1 && map == 2) {
		// ROM space
		if (split8k) {
			for (i = 4; i < 8; i++)
				pointers[1][32 + i] = memROM + (bank * 16 + i) * 1024;
			for (i = (sizeRomKB - 4) + 8; i < 16; i++)
				pointers[1][32 + i] = memROM + (bank * 16 + i) * 1024;
		}
		else if (sizeRomKB <= 8) {
			for (i = sizeRomKB; i < 8; i++)
				pointers[1][32 + i] = memROM + (bank * 16 + i) * 1024;
			for (i = sizeRomKB + 8; i < 16; i++)
				pointers[1][32 + i] = memROM + (bank * 16 + i) * 1024;
		}
		else {
			for (i = sizeRomKB; i < 16; i++)
				pointers[1][32 + i] = memROM + (bank * 16 + i) * 1024;
		}
	}
	else {
		for (i = 0; i < 16; i++)
			pointers[part][map * 16 + i] = memROM + (bank * 16 + i) * 1024;
	}
}
//---------------------------------------------------------------------------
/*
 * Searches for requested block in virtual area, returns its address into
 * address of output parameter 'ptr' and returns number of bytes that
 * can fit to the block. If requested block has no address defined in virtual
 * area (MA_NA), returns NULL value into address of output parameter 'ptr'.
 * If requested block doesn't exist or parameters are incorrect, returns
 * value -1 and NULL in 'ptr' adress.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @param len lenght of requested block
 * @param oper requested type of operation - OP_WRITE or OP_READ
 * @param ptr address of variable where to store address to virtual area
 * @return number of bytes that can fit to block found
 */
int ChipMemory2AEx::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
{
	if (physAddr >= 0 && physAddr <= 0xFFFF && len > 0 && len <= 0x10000) {
		int idx = physAddr / 1024;
		int offset = physAddr & 0x03FF;
		int lenX = 1024 - offset;

		int part = (oper == OP_READ && (!allRAM || resetState)) ? 1 : 0;

		BYTE *block = pointers[part][idx];
		if (block != NULL) {
			block += offset;
			*ptr = block;
			return (len < lenX) ? len : lenX;
		}
	}

	*ptr = NULL;
	return -1;
}
//---------------------------------------------------------------------------
void ChipMemory2AEx::WriteToDevice(BYTE port, BYTE value, int UNUSED_VARIABLE ticks)
{
	if ((port & MM256_REG_MASK) == MM256_REG_ADR)
		SetPage(value);
}
//---------------------------------------------------------------------------
BYTE ChipMemory2AEx::ReadFromDevice(BYTE port, int UNUSED_VARIABLE ticks)
{
	return ((port & MM256_REG_MASK) == MM256_REG_ADR) ? GetPage() : 0xFF;
}
//---------------------------------------------------------------------------
