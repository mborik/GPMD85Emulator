/*	ChipMemoryC2717.cpp: Derived class for memory management of Consul 2717
	Copyright (c) 2015-2026 Roman Borik <pmd85emu@gmail.com>

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
#include "ChipMemoryC2717.h"
//---------------------------------------------------------------------------
ChipMemoryC2717::ChipMemoryC2717(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 64 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	vramOffset = 0xC000;
	hasAllRAM = true;
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
 * In C2717 there are two different remapping modes for addresses above C000h.
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
		if (remapped && physAddr >= 0xC000) {
			if (remapType != 2) {
				// Version "1"
				// Address : F E D C B A 9 8  7 6 5 4 3 2 1 0
				//                    _ _
				// Mapping : F E 5 4 B A 9 8  7 6 H H 3 2 1 0
				physAddr = (physAddr & 0xCFCF)
						 | (((physAddr & 0x0030) ^ 0x0030) << 8)
						 | 0x0030;
			}
			else {
				// Version "2"
				// Address : F E D C B A 9 8  7 6 5 4 3 2 1 0
				//                                     _ _
				// Mapping : F E 5 4 B A 9 8  7 6 D C 3 2 1 0
				physAddr = (physAddr & 0xCFCF)
						 | ((physAddr & 0x0030) << 8)
						 | (((physAddr & 0x3000) ^ 0x3000) >> 8);
			}
		}

		if (!allRAM || resetState) {
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
		}

		*ptr = memRAM + physAddr;
		int lenX = 0x10000 - physAddr;
		return (len < lenX) ? len : lenX;
	}

	*ptr = NULL;
	return -1;
}
//---------------------------------------------------------------------------
