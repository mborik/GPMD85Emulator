/*	ChipMemoryMato.cpp: Derived class for memory management of Mato
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
