/*  ChipMemory2A.cpp: Derived class for memory management of Model 2A
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
#include "ChipMemory2A.h"
//---------------------------------------------------------------------------
ChipMemory2A::ChipMemory2A(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 64 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
}
//---------------------------------------------------------------------------
void ChipMemory2A::ResetOn()
{
	// reset happen...
	resetState = true;
	int i;

	// at first, set RAM into whole addressing space
	for (i = 0; i < 64; i++) {
		pointers[0][i] = memRAM + i * 1024;
		pointers[1][i] = memRAM + i * 1024;
	}

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
void ChipMemory2A::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;

	// RAM space instead of ROM paged after reset
	for (int i = 0; i < 32; i++)
		pointers[1][i] = memRAM + i * 1024;
}
//---------------------------------------------------------------------------
BYTE* ChipMemory2A::GetVramPointer()
{
	return memRAM + 0xC000;
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
int ChipMemory2A::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
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
