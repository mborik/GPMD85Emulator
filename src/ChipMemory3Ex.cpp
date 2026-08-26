/*	ChipMemory3Ex.cpp: Derived class for memory management and
		peripheral device handling of memory expansion for Model 3.
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
#include "ChipMemory3Ex.h"
//---------------------------------------------------------------------------
ChipMemory3Ex::ChipMemory3Ex(BYTE totalSizeKB) : ChipMemory(totalSizeKB)
{
	mem256 = true;
	sizeRAM = 256 * 1024;
	memRAM = new BYTE[sizeRAM];
	memset(memRAM, 0, sizeRAM);
	hasAllRAM = true;
}
//---------------------------------------------------------------------------
void ChipMemory3Ex::ResetOn()
{
	// reset happen...
	resetState = true;

	bank = 0;
	map = 0;
	vram2 = false;

	int i;

	// at first, set RAM into whole addressing space
	for (i = 0; i < 8; i++)
		pointers[0][i] = memRAM + i * 8192;

	// mirrored ROM also under whole addressing space
	for (int i = 0; i < 8; i++)
		pointers[1][i] = memROM;
}
//---------------------------------------------------------------------------
void ChipMemory3Ex::ResetOff()
{
	if (!resetState)
		return;

	// reset button released...
	resetState = false;

	bank = 0;
	map = 0;
	vram2 = false;

	// RAM space instead of ROM paged after reset
	for (int i = 0; i < 7; i++)
		pointers[1][i] = memRAM + i * 8192;
}
//---------------------------------------------------------------------------
BYTE* ChipMemory3Ex::GetVramPointer()
{
	return memRAM + (vramOffset = (vram2 ? 7 : 3) * 0x4000);
}
//---------------------------------------------------------------------------
BYTE ChipMemory3Ex::GetPage()
{
	return (BYTE) (bank | (map << 4) | (vram2 << 6));
}
//---------------------------------------------------------------------------
void ChipMemory3Ex::SetPage(BYTE pg)
{
	// revert initial bank onto previous space
	FillPointers(0, map, map);
	FillPointers(1, map, map);

	// perform full redraw of the screen when VRAM is switching...
	bool newVRAM = (pg & 0x40);
	if (newVRAM != vram2) {
		drawRegion.tl = 0x0000;
		drawRegion.br = 0x3FEF;
	}

	// new mapping set
	bank = (pg & 0x0F);
	map = (pg & 0x30) >> 4;
	vram2 = newVRAM;

	// map selected bank onto selected space
	FillPointers(0, map, bank);
	FillPointers(1, map, bank);
}
//---------------------------------------------------------------------------
void ChipMemory3Ex::FillPointers(int part, int map, int bank)
{
	if (part == 1 && map == 2) {
		// ROM space
		pointers[1][map * 2] = memRAM + (bank * 2) * 8192;
	}
	else {
		pointers[part][map * 2 + 0] = memRAM + (bank * 2 + 0) * 8192;
		pointers[part][map * 2 + 1] = memRAM + (bank * 2 + 1) * 8192;
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
int ChipMemory3Ex::FindPointer(int physAddr, int len, int oper, BYTE **ptr)
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
void ChipMemory3Ex::WriteToDevice(BYTE port, BYTE value, int UNUSED_VARIABLE ticks)
{
	if ((port & MM256_REG_MASK) == MM256_REG_ADR)
		SetPage(value);
}
//---------------------------------------------------------------------------
BYTE ChipMemory3Ex::ReadFromDevice(BYTE port, int UNUSED_VARIABLE ticks)
{
	return ((port & MM256_REG_MASK) == MM256_REG_ADR) ? GetPage() : 0xFF;
}
//---------------------------------------------------------------------------
