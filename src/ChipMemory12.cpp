/*  ChipMemory12.cpp: Derived class for memory management of Model 1 & 2
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
#include "ChipMemory12.h"
//---------------------------------------------------------------------------
ChipMemory12::ChipMemory12(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 48 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	vramOffset = 0x8000;
}
//---------------------------------------------------------------------------
void ChipMemory12::ResetOn()
{
	// reset happen...
	resetState = true;

	// <0000h, 0FFFh> - ROM
	pointers[0] = memROM;
	rw[0] = MA_RO;
	// <1000h, 1FFFh> - NC
	pointers[1] = NULL;
	rw[1] = MA_NA;
	// <2000h, 2FFFh> - mirrored ROM or 2nd hald of 8kB ROM
	pointers[2] = (split8k) ? memROM + 4096 : memROM;
	rw[2] = MA_RO;
	// <3000h, 3FFFh> - NC
	pointers[3] = NULL;
	rw[3] = MA_NA;

	// <8000h, 8FFFh> - ROM
	pointers[8] = memROM;
	rw[8] = MA_RO;
	// <9000h, 9FFFh> - NC
	pointers[9] = NULL;
	rw[9] = MA_NA;
	// <A000h, AFFFh> - mirrored ROM or 2nd hald of 8kB ROM
	pointers[10] = (split8k) ? memROM + 4096 : memROM;
	rw[10] = MA_RO;
	// <B000h, BFFFh> - NC
	pointers[11] = NULL;
	rw[11] = MA_NA;

	// <4000h, 7FFFh> - VideoRAM (!)
	// <C000h, FFFFh> - VideoRAM
	for (int i = 4; i < 8; i++) {
		// 4xxxh
		pointers[i] = memRAM + (i + 4) * 4096;
		rw[i] = MA_RW;
		// Cxxxh
		pointers[i + 8] = memRAM + (i + 4) * 4096;
		rw[i + 8] = MA_RW;
	}
}
//---------------------------------------------------------------------------
void ChipMemory12::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;
	int i = 0;

	// <0000h, 7FFFh> - RAM
	for (; i < 8; i++) {
		pointers[i] = memRAM + i * 4096;
		rw[i] = MA_RW;
	}

	// <8000h, BFFFh> - ROM
	for (; i < 12; i++) { // reset pointers
		pointers[i] = NULL;
		rw[i] = MA_NA;
	}

	if (split8k) {
		// <8000h, 8FFFh> - ROM
		pointers[8] = memROM;
		rw[8] = MA_RO;
		// <9000h, 9FFFh> - NC
		pointers[9] = NULL;
		rw[9] = MA_NA;
		// <A000h, AFFFh> - 2nd half of 8kB ROM
		pointers[10] = memROM + 4096;
		rw[10] = MA_RO;
		// <B000h, BFFFh> - NC
		pointers[11] = NULL;
		rw[11] = MA_NA;
	}
	else {
		for (i = 8; i < (8 + (sizeRomKB + 3) / 4); i++) {
			pointers[i] = memROM + (i - 8) * 4096;
			rw[i] = MA_RO;

			if (sizeRomKB <= 8) {
				pointers[i + 2] = memROM + (i - 8) * 4096;
				rw[i + 2] = MA_RO;
			}
		}
	}

	// <C000h, FFFFh> - Video RAM
	for (i = 12; i < 16; i++) {
		pointers[i] = memRAM + (i - 4) * 4096;
		rw[i] = MA_RW;
	}
}
//---------------------------------------------------------------------------
BYTE* ChipMemory12::GetVramPointer()
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
int ChipMemory12::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
{
	if (physAddr >= 0 && physAddr <= 0xFFFF && len > 0 && len <= 0x10000) {
		int idx = physAddr / 4096;
		int offset = physAddr & 0x0FFF;
		int lenX = 4096 - offset;

		if (rw[idx] & oper) {
			BYTE *block = pointers[idx];
			if (block != NULL) {
				block += offset;
				*ptr = block;
				return (len < lenX) ? len : lenX;
			}
		}
	}

	*ptr = NULL;
	return -1;
}
//---------------------------------------------------------------------------
