/*  ChipMemory.cpp: Base class for memory management
    Copyright (c) 2006-2016 Roman Borik <pmd85emu@gmail.com>

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
#include "ChipMemory.h"
//---------------------------------------------------------------------------
/**
 * Constructor will create memory area.
 *
 * @param initRomSizeKB size of ROM in kB
 */
ChipMemory::ChipMemory(WORD initRomSizeKB)
{
	sizeRomKB = initRomSizeKB;
	if (sizeRomKB < 4)
		sizeRomKB = 4;
	if (sizeRomKB > 16)
		sizeRomKB = 16;
	sizeROM = sizeRomKB * 1024;
	memROM = new BYTE[sizeROM];
	memset(memROM, 0xFF, sizeROM);

	memRAM = NULL;

	resetState = false;
	allRAM = false;
	remapped = false;
	mem256 = false;
	split8k = false;

	drawRegion.tl = 0x3FFF;
	drawRegion.br = 0x0000;
}
//---------------------------------------------------------------------------
/**
 * Destructor will free up allocated memory space.
 */
ChipMemory::~ChipMemory()
{
	if (memROM != NULL) {
		delete [] memROM;
		memROM = NULL;
	}
	if (memRAM != NULL) {
		delete [] memRAM;
		memRAM = NULL;
	}
}
//---------------------------------------------------------------------------
bool ChipMemory::WasVramModified()
{
	if (drawRegion.br < drawRegion.tl)
		return false;

	drawRegion.tl = 0x3FFF;
	drawRegion.br = 0x0000;

	return true;
}
//---------------------------------------------------------------------------
/*
 * Copies data from source address to physical memory which represents ROM
 *
 * @param src pointer to memory from where the date will be copied
 * @param size number of bytes to be copied
 * @return true if success; false otherwise
 */
bool ChipMemory::PutRom(BYTE *src, int size)
{
	if (size < 1 || size > sizeROM || src == NULL)
		return false;

	memcpy(memROM, src, (size > sizeROM) ? sizeROM : size);
	if (size < sizeROM)
		memset(memROM + size, 0xFF, sizeROM - size);
	return true;
}
//---------------------------------------------------------------------------
/*
 * Copies data to physical memory where it is possible to write data from
 * base address. Returns true if succcess; false if parameters are incorrect
 * or memory is not defined.
 *
 * @param physAddr physical memory address - 0 to FFFFh
 * @param page number of page which should be paged in
 * @param src pointer to memory from where the date will be copied
 * @param size number of bytes to be copied
 * @return true if success; false otherwise
 */
bool ChipMemory::PutMem(int physAddr, BYTE *src, int size)
{
	int count;
	BYTE *ptr;

	if (physAddr < 0 || physAddr > 0xFFFF || size < 1 ||
			size > 0x10000 || (physAddr + size) > 0x10000 || src == NULL)
		return false;

	do {
		count = FindPointer(physAddr, size, OP_WRITE, &ptr);
		if (count <= 0)
			return false;
		if (ptr)
			memcpy(ptr, src, count);

		src += count;
		physAddr += count;
		size -= count;
	} while (size > 0);

	return true;
}
//---------------------------------------------------------------------------
/*
 * Copies requested part of physical memory to desired location.
 *
 * @param dest pointer to memory where to store part of physical memory
 * @param physAddr physical memory address - 0 to FFFFh
 * @param size number of bytes to be copied
 * @return true if success; false otherwise
 */
bool ChipMemory::GetMem(BYTE *dest, int physAddr, int size)
{
	int count;
	BYTE *ptr;

	if (physAddr < 0 || physAddr > 0xFFFF || size < 1 ||
			size > 0x10000 || (physAddr + size) > 0x10000 || dest == NULL)
		return false;

	do {
		count = FindPointer(physAddr, size, OP_READ, &ptr);
		if (count <= 0)
			return false;
		if (ptr)
			memcpy(dest, ptr, count);
		else
			memset(dest, NA_BYTE, count); // non-existing block of memory

		dest += count;
		physAddr += count;
		size -= count;
	} while (size > 0);

	return true;
}
//---------------------------------------------------------------------------
/*
 * Fills in given memory area with desired value.
 *
 * @param destAddr physical destination address - 0 to FFFFh
 * @param value fill memory with this value
 * @param size memory size to be filled in
 * @return true if success; false otherwise
 */
bool ChipMemory::FillMem(int destAddr, BYTE value, int size)
{
	int count;
	BYTE *ptr;

	if (destAddr < 0 || destAddr > 0xFFFF || size < 1 ||
			size > 0x10000 || (destAddr + size) > 0x10000)
		return false;

	do {
		count = FindPointer(destAddr, size, OP_WRITE, &ptr);
		if (count <= 0)
			return false;
		if (ptr)
			memset(ptr, value, count);

		destAddr += count;
		size -= count;
	} while (size > 0);

	return true;
}
//---------------------------------------------------------------------------
/*
 * Reads the byte from given memory address. If type of memory is MA_WO
 * or MA_NA then returns value NA_BYTE. Method is heavily using CPU.
 * Current paging and remapping are considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @return byte from memory
 */
BYTE ChipMemory::ReadByte(int physAddr)
{
	BYTE *ptr;
	if (FindPointer(physAddr, 1, OP_READ, &ptr) > 0 && ptr)
		return *ptr;

	return NA_BYTE;
}
//---------------------------------------------------------------------------
/*
 * Reads 2 consecutive bytes from given memory address. If type of memory
 * is MA_WO or MA_NA then returns value NA_WORD.
 * Method is heavily using CPU. Current paging and remapping are considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @return 2 bytes from memory
 */
WORD ChipMemory::ReadWord(int physAddr)
{
	WORD value;

	value = ReadByte(physAddr);
	value |= (WORD) (ReadByte((physAddr + 1) & 0xFFFF) << 8);

	return value;
}
//---------------------------------------------------------------------------
/*
 * Writes requested byte to given memory address. If memory type at given
 * location is MA_WO or MA_NA, nothing will happen.
 * Method is heavily using CPU. Current paging is considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @param value value to be written to memory
 */
void ChipMemory::WriteByte(int physAddr, BYTE value)
{
	BYTE *ptr;
	if (FindPointer(physAddr, 1, OP_WRITE, &ptr) > 0 && ptr) {
		*ptr = value;

		int offset = (ptr - memRAM);
		if ((offset & 0xFC000) == vramOffset && (offset & 0x3F) < 48) {
			offset &= 0x3FFF;

			drawRegion.tl = SDL_min(drawRegion.tl, offset);
			drawRegion.br = SDL_max(drawRegion.br, offset);
		}
	}
}
//---------------------------------------------------------------------------
/*
 * Writes 2 bytes to given memory address. If memory type at given
 * location is MA_WO or MA_NA, nothing will happen.
 * Method is heavily using CPU. Current paging is considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @param value value to be written to memory
 */
void ChipMemory::WriteWord(int physAddr, WORD value)
{
	WriteByte(physAddr, (BYTE) (value & 0xFF));
	WriteByte((physAddr + 1) & 0xFFFF, (BYTE) ((value >> 8) & 0xFF));
}
//---------------------------------------------------------------------------
