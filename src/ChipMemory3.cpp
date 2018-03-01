/*  ChipMemory3.cpp: Derived class for memory management of Model 3
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
#include "ChipMemory3.h"
//---------------------------------------------------------------------------
ChipMemory3::ChipMemory3(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 64 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	vramOffset = 0xC000;
}
//---------------------------------------------------------------------------
void ChipMemory3::ResetOn()
{
	// reset happen...
	resetState = true;

	// RAM into whole addressing space
	for (int i = 0; i < 8; i++)
		pointers[0][i] = memRAM + i * 8192;

	// mirrored ROM also under whole addressing space
	for (int i = 0; i < 8; i++)
		pointers[1][i] = memROM;
}
//---------------------------------------------------------------------------
void ChipMemory3::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;

	// RAM space instead of ROM paged after reset
	for (int i = 0; i < 7; i++)
		pointers[1][i] = memRAM + i * 8192;
}
//---------------------------------------------------------------------------
BYTE* ChipMemory3::GetVramPointer()
{
	return memRAM + vramOffset;
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
int ChipMemory3::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
{
	if (physAddr >= 0 && physAddr <= 0xFFFF && len > 0 && len <= 0x10000) {
		int idx = physAddr / 8192;
		int offset = physAddr & 0x1FFF;
		int lenX = 8192 - offset;

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
