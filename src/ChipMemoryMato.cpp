/*  ChipMemoryMato.cpp: Derived class for memory management of Mato
		Copyright (c) 2015-2026 Roman Borik <pmd85emu@gmail.com>

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
#include "ChipMemoryMato.h"
//---------------------------------------------------------------------------
ChipMemoryMato::ChipMemoryMato(BYTE totalSizeKB, bool hasRAM64kB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 64 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	vramOffset = 0xC000;
	hasAllRAM = hasRAM64kB;
}
//---------------------------------------------------------------------------
void ChipMemoryMato::ResetOn()
{
	// reset happen...
	resetState = true;
}
//---------------------------------------------------------------------------
void ChipMemoryMato::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;
}
//---------------------------------------------------------------------------
BYTE* ChipMemoryMato::GetVramPointer()
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
int ChipMemoryMato::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
{
	if (physAddr >= 0 && physAddr <= 0xFFFF && len > 0 && len <= 0x10000) {
		if (!hasAllRAM || resetState) {
			if ((physAddr >= 0x8000 && physAddr < 0xC000)
					|| (resetState && physAddr >= 0 && physAddr < 0x4000)) {
				if (oper == OP_READ) {
					int offset = physAddr & 0x3FFF;
					*ptr = memROM + offset;
					int lenX = 0x4000 - offset;
					return (len < lenX) ? len : lenX;
				}
				else {
					*ptr = NULL;
					return -1;
				}
			}
			if (resetState) {
				// while reset state, ROM was mirrored in 0000-3FFF range
				// RAM in range 4000-7FFF and C000-FFFF is not accessible
				*ptr = NULL;
				return -1;
			}
		}

		*ptr = memRAM + physAddr;
		int lenX = 0x10000 - physAddr;
		return (len < lenX) ? len : lenX;
	}

	*ptr = NULL;
	return -1;
}
//---------------------------------------------------------------------------
