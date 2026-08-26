/*	ChipMemory3.cpp: Derived class for memory management of Model 3
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
#include "ChipMemory3.h"
//---------------------------------------------------------------------------
ChipMemory3::ChipMemory3(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	sizeRAM = 64 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	vramOffset = 0xC000;
	hasAllRAM = true;
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
