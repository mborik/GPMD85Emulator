/*  ChipMemory.h: Class for memory management
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
#ifndef ChipMemoryH
#define ChipMemoryH
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
// type of memory block access
#define MA_RO     1   // ROM, read only, write attempt is ignored
#define MA_WO     2   // RWM, write only, read attempt is ignored
#define MA_RW     3   // RWM, read and write
#define MA_NA     0   // unallocated memory block, write attempt is ignored
                      // read attempt returns NA_BYTE or NA_WORD

// operations
#define OP_READ   1   // read operation
#define OP_WRITE  2   // write operation

// return value for non-existing memory access
#define NA_BYTE   0xFF
#define NA_WORD   (NA_BYTE | (NA_BYTE << 8))

// pages
#define NO_PAGED  -1
#define PAGE_ANY  -99999
//---------------------------------------------------------------------------
class ChipMemory
{
public:
	ChipMemory(WORD totalSizeKB);
	~ChipMemory();

	bool AddBlock(BYTE physAddrKB, BYTE sizeKB, int virtOffset, int page, int memAccess);
	BYTE *GetMemPointer(int physAddr, int page, int oper);
	bool PutRom(BYTE physAddrKB, int page, BYTE *src, int size);
	bool PutMem(int physAddr, int page, BYTE *src, int size);
	bool GetMem(BYTE *dest, int physAddr, int page, int size);
	bool FillMem(int destAddr, int page, BYTE value, int size);

	// methods called by CPU
	BYTE ReadByte(int physAdr);
	WORD ReadWord(int physAdr);
	void WriteByte(int physAdr, BYTE value);
	void WriteWord(int physAdr, WORD value);

	int  Page;          // current page number
	bool C2717Remapped; // turn on/off Consul 2717 re-addressing

private:

	typedef struct Block {
		int size;      // block size 0400h - 10000h (1kB - 64kB)
		int address;   // physical address 0000h - FC00h
		BYTE *pointer; // pointer to virtual memory area
		int page;      // page number (-1 means non-paged memory block)
		int access;    // "memory type" and its access type
		Block *next;   // pointer to next memory block
	} BLOCK;

	BYTE *Memory;      // pointer to virtual memory area
	int MemSize;       // size of virtual memory area
	BLOCK *Blocks;     // pointer to first memory block
	BLOCK *LastBlock;  // pointer to last memory block

	int C2717Remap(int address);
	int FindPointer(int physAdr, int len, int page, int oper, BYTE **point);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
