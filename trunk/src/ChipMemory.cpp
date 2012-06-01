/*  ChipMemory.cpp: Class for memory management
    Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

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
 * @param totalSizeKB total memory size in kB (ROM + RAM)
 */
ChipMemory::ChipMemory(WORD totalSizeKB)
{
	MemSize = totalSizeKB * 1024;
	Memory = new BYTE[MemSize];
	memset(Memory, 0, MemSize);
	Blocks = NULL;
	LastBlock = NULL;
	Page = NO_PAGED;
	C2717Remapped = false;
}
//---------------------------------------------------------------------------
/**
 * Destructor will free up allocated memory space and destroy defined
 * memory blocks.
 */
ChipMemory::~ChipMemory()
{
	BLOCK *block, *next;

	block = Blocks;
	while (block) {
		next = block->next;
		delete block;
		block = next;
	}

	delete[] Memory;
}
//---------------------------------------------------------------------------
/*
 * Defines memory block with required size, physical location, location in
 * virtual area, requested page number and access attribute.
 *
 * @param physAddrKB physical memory address in kB - 0 az 63 kB
 * @param sizeKB block size in kB - 1 az 64 kB
 * @param virtOffset offset to virtual area where block is located
 * @param page number of page where block belongs to;
 *        NO_PAGED for non-paged block
 * @param memAccess access type for block - MA_RO, MA_RW, MA_WO, MA_NA
 * @return true, if block defined succefully
 *         false, if some of parameter is incorrect
 */
bool ChipMemory::AddBlock(BYTE physAddrKB, BYTE sizeKB, int virtOffset, int page, int memAccess)
{
	if (sizeKB > 64 || sizeKB < 1 || physAddrKB > 63 || (sizeKB + physAddrKB) > 64
		|| virtOffset >= MemSize || (virtOffset + sizeKB * 1024) > MemSize)
		return false;

	if (Blocks == NULL) {
		Blocks = new BLOCK;
		LastBlock = Blocks;
	}
	else {
		LastBlock->next = new BLOCK;
		LastBlock = LastBlock->next;
	}

	LastBlock->size = sizeKB * 1024;
	LastBlock->address = physAddrKB * 1024;
	if (memAccess == MA_NA)
		LastBlock->pointer = NULL;
	else
		LastBlock->pointer = Memory + virtOffset;
	LastBlock->page = page;
	LastBlock->access = memAccess;
	LastBlock->next = NULL;

	return true;
}
//---------------------------------------------------------------------------
/*
 * Returns address to virtual area corresponding to requested physical
 * address in memory, page number and operation.
 *
 * @param physAddr physical address in memory - 0 to FFFFh
 * @param page requested page number
 * @param oper requested type of operation - OP_WRITE or OP_READ
 * @return address to virtual area or NULL, if this block doesn’t exist
 */
BYTE* ChipMemory::GetMemPointer(int physAddr, int page, int oper)
{
	BYTE *mem;

	if (physAddr < 0 || physAddr > 0xFFFF)
		return NULL;

	if (FindPointer(physAddr, 1, page, oper, &mem) <= 0)
		return NULL;

	return mem;
}
//---------------------------------------------------------------------------
/*
 * Copies data from source address to physical memory which represents ROM
 * Returns true if succcess; false if parameters are incorrect or memory
 * is not defined.
 *
 * @param physAddrKB physical kilobyte address to memory - 0 az 63
 * @param page number of page which should be paged in
 * @param src pointer to memory from where to copy data to physical memory
 * @param size number of bytes to be copied
 * @return true if success; false otherwise
 */
bool ChipMemory::PutRom(BYTE physAddrKB, int page, BYTE *src, int size)
{
	DWORD physAddr;
	int cnt;
	BYTE *mem;

	physAddr = physAddrKB * 1024;
	if (physAddrKB > 63 || size < 1 || size > 0x10000 || (physAddr + size) > 0x10000 || src == NULL)
		return false;

	do {
		cnt = FindPointer(physAddr, size, page, OP_READ, &mem);
		if (cnt <= 0 || mem == NULL)
			return false;
		memcpy(mem, src, cnt);

		src += cnt;
		physAddr += cnt;
		size -= cnt;
	} while (size > 0);

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
 * @param src pointer to memory from where to copy data to physical memory
 * @param size number of bytes to be copied
 * @return true if success; false otherwise
 */
bool ChipMemory::PutMem(int physAddr, int page, BYTE *src, int size)
{
	int cnt;
	BYTE *mem;

	if (physAddr < 0 || physAddr > 0xFFFF || size < 1 || size > 0x10000 || (physAddr + size) > 0x10000 || src == NULL)
		return false;

	do {
		cnt = FindPointer(physAddr, size, page, OP_WRITE, &mem);
		if (cnt <= 0)
			return false;
		if (mem)
			memcpy(mem, src, cnt);

		src += cnt;
		physAddr += cnt;
		size -= cnt;
	} while (size > 0);

	return true;
}
//---------------------------------------------------------------------------
/*
 * Copies requested part of physical memory to desired location.
 * Returns true if succcess; false if parameters are incorrect or memory
 * is not defined.
 *
 * @param dest pointer to memory where to store part of physical memory
 * @param physAddr physical memory address - 0 to FFFFh
 * @param page number of page which should be paged in
 * @param size number of bytes to be copied
 * @return true if success; false otherwise
 */
bool ChipMemory::GetMem(BYTE *dest, int physAddr, int page, int size)
{
	int cnt;
	BYTE *mem;

	if (physAddr < 0 || physAddr > 0xFFFF || size < 1 || size > 0x10000 || (physAddr + size) > 0x10000 || dest == NULL)
		return false;

	do {
		cnt = FindPointer(physAddr, size, page, OP_READ, &mem);
		if (cnt <= 0)
			return false;
		if (mem)
			memcpy(dest, mem, cnt);
		else
			memset(dest, NA_BYTE, cnt); // neobsadeny blok pamate

		dest += cnt;
		physAddr += cnt;
		size -= cnt;
	} while (size > 0);

	return true;
}
//---------------------------------------------------------------------------
/*
 * Fills in given memory area by desired value. Returns true if success;
 * false if parameters are incorrect or memory is not defined.
 *
 * @param destAddr physical destination address - 0 to FFFFh
 * @param page number of page which should be paged in
 * @param value fill memory with this value
 * @param size memory size to be filled in
 * @return true if success; false otherwise
 */
bool ChipMemory::FillMem(int destAddr, int page, BYTE value, int size)
{
	int cnt;
	BYTE *mem;

	if (destAddr < 0 || destAddr > 0xFFFF || size < 1 || size > 0x10000 || (destAddr + size) > 0x10000)
		return false;

	do {
		cnt = FindPointer(destAddr, size, page, OP_WRITE, &mem);
		if (cnt <= 0)
			return false;
		if (mem)
			memset(mem, value, cnt);

		destAddr += cnt;
		size -= cnt;
	} while (size > 0);

	return true;
}
//---------------------------------------------------------------------------
/*
 * Reads the byte from given memory address. If type of memory is MA_WO
 * or MA_NA then returns value NA_BYTE. Method is heavily using CPU.
 * Current page and remapping are considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @return byte from memory
 */
BYTE ChipMemory::ReadByte(int physAdr)
{
	BYTE *mem;

	if (C2717Remapped)
		physAdr = C2717Remap(physAdr);

	if (physAdr >= 0 && physAdr < 0x10000) {
		if (FindPointer(physAdr, 1, Page, OP_READ, &mem) > 0 && mem)
			return *mem;
	}

	return NA_BYTE;
}
//---------------------------------------------------------------------------
/*
 * Reads 2 consecutive bytes from given memory address. If type of memory
 * is MA_WO or MA_NA then returns value NA_WORD.
 * Method is heavily using CPU. Current page and remapping are considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @return 2 bytes from memory
 */
WORD ChipMemory::ReadWord(int physAdr)
{
	WORD val;

	val = ReadByte(physAdr);
	val |= (WORD) (ReadByte((physAdr + 1) & 0xFFFF) << 8);

	return val;
}
//---------------------------------------------------------------------------
/*
 * Writes requested byte to given memory address. If memory type at given
 * location is MA_WO or MA_NA, nothing will happen.
 * Method is heavily using CPU. Current page is considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @param value value to be written to memory
 */
void ChipMemory::WriteByte(int physAdr, BYTE value)
{
	BYTE *mem;

	if (C2717Remapped)
		physAdr = C2717Remap(physAdr);

	if (physAdr < 0 || physAdr > 0xFFFF)
		return;

	if (FindPointer(physAdr, 1, Page, OP_WRITE, &mem) > 0 && mem)
		*mem = value;
}
//---------------------------------------------------------------------------
/*
 * Writes 2 bytes to given memory address. If memory type at given
 * location is MA_WO or MA_NA, nothing will happen.
 * Method is heavily using CPU. Current page is considered.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @param value value to be written to memory
 */
void ChipMemory::WriteWord(int physAdr, WORD value)
{
	WriteByte(physAdr, (BYTE) (value & 0xFF));
	WriteByte((physAdr + 1) & 0xFFFF, (BYTE) ((value >> 8) & 0xFF));
}
//---------------------------------------------------------------------------
int ChipMemory::C2717Remap(int address)
{
	if (address < 0xC000)
		return address;

	return (address & 0xCFCF)
		| (((address << 8) ^ 0xFF00) & 0x3000)
		| (((address >> 8) ^ 0xFF) & 0x30);
}
//---------------------------------------------------------------------------
/*
 * Searches for requested block in virtual area, returns its address into
 * address of output parameter 'point' and returns number of bytes that
 * can fit to the block. If requested block has no address defined in virtual
 * area (MA_NA), returns NULL value into address of output parameter 'point'.
 * If requested block doesn't exist or parameters are incorrect, returns
 * value -1 and NULL in 'point' adress.
 *
 * @param physAddr physical address to memory - 0 to FFFFh
 * @param len lenght of requested block
 * @param page requested page number
 * @param oper requested type of operation - OP_WRITE or OP_READ
 * @param point address of variable where to store address to virtual area
 * @return number of bytes that can fit to block found
 */
int ChipMemory::FindPointer(int physAdr, int len, int page, int oper, BYTE **point)
{
	if (physAdr >= 0 && physAdr <= 0xFFFF && len > 0 && len <= 0x10000) {
		BLOCK *bl = Blocks;
		while (bl) {
			if ((page == PAGE_ANY || bl->page == NO_PAGED || bl->page == page) && ((bl->access & oper)
				|| bl->access == MA_NA) && physAdr >= bl->address && physAdr < (bl->address + bl->size)) {

				if (bl->pointer)
					*point = (bl->pointer + physAdr - bl->address);
				else
					*point = NULL;
				if ((physAdr + len) <= (bl->address + bl->size))
					return len;
				else
					return bl->size - physAdr + bl->address;
			}

			bl = bl->next;
		}
	}

	*point = NULL;
	return -1;
}
//---------------------------------------------------------------------------
