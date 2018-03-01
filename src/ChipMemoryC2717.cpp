/*  ChipMemoryC2717.cpp: Derived class for memory management of Consul 2717
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
#include "ChipMemoryC2717.h"
//---------------------------------------------------------------------------
ChipMemoryC2717::ChipMemoryC2717(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 64 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	vramOffset = 0xC000;
}
//---------------------------------------------------------------------------
void ChipMemoryC2717::ResetOn()
{
	// reset happen...
	resetState = true;
}
//---------------------------------------------------------------------------
void ChipMemoryC2717::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;
}
//---------------------------------------------------------------------------
BYTE* ChipMemoryC2717::GetVramPointer()
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
int ChipMemoryC2717::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
{
	if (physAddr >= 0 && physAddr <= 0xFFFF && len > 0 && len <= 0x10000) {
		int offset = physAddr & 0x3FFF;
		int lenX = 16384 - offset;

		if (remapped && physAddr >= 0xC000) {
			physAddr  = (physAddr & 0xCFCF);
			physAddr |= (((physAddr << 8) ^ 0xFF00) & 0x3000);
			physAddr |= (((physAddr >> 8) ^ 0xFF) & 0x30);
		}

		if (!allRAM || resetState) {
			if ((physAddr >= 0x8000 && physAddr < 0xC000) ||
					(resetState && physAddr >= 0 && physAddr < 0x4000)) {

				if (oper == OP_READ) {
					*ptr = memROM + offset;
					return (len < lenX) ? len : lenX;
				}
				else {
					*ptr = NULL;
					return -1;
				}
			}
		}

		*ptr = memRAM + physAddr;
		lenX = 0x10000 - physAddr;
		return (len < lenX) ? len : lenX;
	}

	*ptr = NULL;
	return -1;
}
//---------------------------------------------------------------------------
