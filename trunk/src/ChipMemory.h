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
// typ pristupu k bloku pamati
#define MA_RO     1   // ROM, len citanie, pokus o zapis sa ignoruje
#define MA_WO     2   // RWM, len zapis, pokus o citanie sa ignoruje
#define MA_RW     3   // RWM, citanie i zapis
#define MA_NA     0   // neobsadeny blok pamati, pokus o zapis sa ignoruje
                      // pokus o citanie vrati NA_BYTE alebo NA_WORD

// operacie
#define OP_READ   1   // operacia citania
#define OP_WRITE  2   // operacia zapisu

// hodnota citana z neexistujucej pamate
#define NA_BYTE   0xFF
#define NA_WORD   (NA_BYTE | (NA_BYTE << 8))

// stranky
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

	// metody volane procesorom
	BYTE ReadByte(int physAdr);
	WORD ReadWord(int physAdr);
	void WriteByte(int physAdr, BYTE value);
	void WriteWord(int physAdr, WORD value);

	int  Page; // cislo aktualnej stranky
	bool C2717Remapped; // zapnutie/vypnutie preadresovania

private:

	typedef struct Block {
		int size;      // velkost bloku  0400h - 10000h (1kB - 64kB)
		int address;   // fyzicka adresa 0000h -  FC00h
		BYTE *pointer; // ukazovatel do virtualneho pamatoveho priestoru
		int page;      // cislo stranky (-1 = nestrankovany blok pamati)
		int access;    // "typ pamati" a sposob pristupu k nej
		Block *next;   // ukazovatel na dalsi blok pamati
	} BLOCK;

	BYTE *Memory;     // ukazovatel na virtualny pamatovy priestor
	int MemSize;      // velkost virtualneho pamatoveho priestoru
	BLOCK *Blocks;    // ukazovatel na prvy pamatovy blok
	BLOCK *LastBlock; // ukazovatel na posledny pamatovy blok

	int C2717Remap(int address);
	int FindPointer(int physAdr, int len, int page, int oper, BYTE **point);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
