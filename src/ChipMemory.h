/*  ChipMemory.h: Base class for memory management
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

// memory mapper of 256k extension
#define MM256_REG_MASK        0xFF
#define MM256_REG_ADR         0x6D
//---------------------------------------------------------------------------
class ChipMemory
{
public:
	ChipMemory(WORD initRomSizeKB);
	virtual ~ChipMemory();

	// set "mapping state" of memory after reset
	virtual void ResetOn() = 0;
	// reset "mapping state" of memory after reset
	virtual void ResetOff() = 0;

	// returns pointer where VRAM was stored in the memory space
	virtual BYTE* GetVramPointer() = 0;

	// returns memory page number - value stored on paging port
	virtual BYTE GetPage() { return (BYTE) 0; }
	// set the memory page number - store the value on paging port
	virtual void SetPage(BYTE btPage) { return; }

	inline bool IsInReset() { return resetState; }
	inline bool IsAllRAM() { return allRAM; }
	inline void SetAllRAM(bool allram) { allRAM = allram; }
	inline bool IsRemapped() { return remapped; }
	inline void SetRemapped(bool remap) { remapped = remap; }
	inline bool IsMem256() { return mem256; }
	inline bool IsSplit8k() { return split8k; }
	inline void SetSplit8k(bool split)
		{ split8k = split8k && (sizeRomKB > 4 && sizeRomKB <= 8); }

	// read/write/fill of memory space
	bool PutRom(BYTE *src, int size);
	bool PutMem(int physAddr, BYTE *src, int size);
	bool GetMem(BYTE *dest, int physAddr, int size);
	bool FillMem(int destAddr, BYTE value, int size);

	// methods called by CPU
	BYTE ReadByte(int physAddr);
	WORD ReadWord(int physAddr);
	void WriteByte(int physAddr, BYTE value);
	void WriteWord(int physAddr, WORD value);

protected:
	BYTE *memROM;      // pointer to virtual ROM memory area
	BYTE *memRAM;      // pointer to virtual RAM memory area
	int  sizeRomKB;    // size of ROM in kilobytes
	int  sizeROM;      // size of ROM in bytes
	int  sizeRAM;      // size of ROM in bytes

	// memory status flags (not all used in all models)
	bool resetState;   // state of memory addressing after the reset happen
	bool allRAM;       // enabled RW access in whole memory space
	bool remapped;     // enabled virtual mapping of addresses to another
	bool mem256;       // extended memory to 256 kilobytes
	bool split8k;      // enable spliting of the ROM into 8000h & A000h

	virtual int FindPointer(int physAddr, int len, int oper, BYTE **ptr) = 0;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
