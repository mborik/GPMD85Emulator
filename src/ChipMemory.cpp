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
 * Konstruktor vytvori pamatovy priestor.
 *
 * @param totalSizeKB celkova velkost pamate v kB (ROM + RAM)
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
 * Destruktor uvolni alokovanu pamat pamatoveho priestoru a zaroven zrusi
 * definovane bloky pamate.
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
 * Definuje blok pamate o pozadovanej velkosti, fyzickom umiestneni, umiestneni
 * vo virtualnom priestore, pozadovanom cisle stranky a pristupovym atributom.
 *
 * @param physAddrKB fyzicka adresa v pamati v kB - 0 az 63 kB
 * @param sizeKB vekost bloku v kB - 1 az 64 kB
 * @param virtOffset offset do virtualneho priestoru, kde sa dany blok nachadza
 * @param page cislo stranky, do ktorej blok patri; NO_PAGED pre nestrankovany
 *                                                  blok
 * @param memAccess sposob pristupu do bloku - MA_RO, MA_RW, MA_WO, MA_NA
 * @return true, ak sa podarilo blok zadefinovat
 *         false, ak niektory z parametrov nevyhovoval
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
 * Vrati adresu do virtualneho priestoru zodpovedajucu pozadovanej fyzickej
 * adrese v pamati, cislu stranky a operacii.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @param page pozadovane cislo stranky
 * @param oper pozadovany typ operacie - OP_WRITE alebo OP_READ
 * @return adresa do virtualneho priestoru; NULL, ak taky blok pamate neexistuje
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
 * Skopiruje do fyzickej pamate, ktora predstavuje ROM, udaje zo zdrojovej
 * adresy. Vrati true pri uspechu; false, ak su chybne parametre alebo pamat
 * nie je zadefinovana.
 *
 * @param physAddrKB fyzicka "kilobytova" adresa do pamate - 0 az 63
 * @param page cislo stranky, ktora by mala byt "nastrankovana"
 * @param src pointer do pamate odkial sa kopiruju data do fyzickej pamate
 * @param size pocet bytov, ktore maju byt skopirovane
 * @return true pri uspechu; false inak
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
 * Skopiruje do fyzickej pamate, do ktorej je mozne zapisovat data zo zdrojovej
 * adresy. Vrati true pri uspechu; false, ak su chybne parametre alebo pamat
 * nie je zadefinovana.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @param page cislo stranky, ktora by mala byt "nastrankovana"
 * @param src pointer do pamate odkial sa kopiruju data do fyzickej pamate
 * @param size pocet bytov, ktore maju byt skopirovane
 * @return true pri uspechu; false inak
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
 * Skopiruje pozadovanu cast z fyzickej pamate na pozadovane miesto.
 * Vrati true pri uspechu; false, ak su chybne parametre alebo pamat nie je
 * zadefinovana.
 *
 * @param dest pointer do pamate kam sa ulozi cast fyzickej pamate
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @param page cislo stranky, ktora by mala byt "nastrankovana"
 * @param size pocet bytov, ktore maju byt skopirovane
 * @return true pri uspechu; false inak
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
 * Vyplni danu pamatovu oblast pozadovanou hodnotou. Vrati true pri uspechu;
 * false, ak su chybne parametre alebo pamat nie je zadefinovana.
 *
 * @param destAddr fyzicka cielova adresa do pamate - 0 az FFFFh
 * @param page cislo stranky, ktora by mala byt "nastrankovana"
 * @param value hodnota, ktorou sa pamat vyplni
 * @param size velkost pamate, ktora ma byt vyplnena
 * @return true pri uspechu; false inak
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
 * Precita byte z pamate z danej adresy. Ak je na danej adrese pamat
 * typu MA_WO alebo MA_NA, vrati sa hodnota NA_BYTE.
 * Metodu pouziva hlavne procesor. Zohladnuje sa aktualna stranka a premapovanie.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @return byte z pamate
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
 * Precita 2 nasledujuce byty z pamate z danej adresy. Ak je na danej adrese
 * pamat typu MA_WO alebo MA_NA, vrati sa hodnota NA_WORD.
 * Metodu pouziva hlavne procesor. Zohladnuje sa aktualna stranka a premapovanie.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @return 2 byte z pamate
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
 * Zapise pozadovany byte do pamate na danu adresu. Ak je na danej adrese pamat
 * typu MA_RO alebo MA_NA, nic sa nestane.
 * Metodu pouziva hlavne procesor. Zohladnuje sa aktualna stranka.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @param value hodnota zapisovana do pamate
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
 * Zapise 2 byty do pamate na danu adresu. Ak je na danej adrese pamat
 * typu MA_RO alebo MA_NA, nic sa nestane.
 * Metodu pouziva hlavne procesor. Zohladnuje sa aktualna stranka.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @param value hodnota zapisovana do pamate
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
 * Vyhlada pozadovany blok vo virtualnom priestore, ulozi jeho adresu na adresu
 * vstupneho parametra 'point' a vrati pocet bytov, ktore sa do daneho bloku
 * vojdu. Ak dany blok nema zadefinovanu adresu vo virtualnom priestore (MA_NA),
 * ulozi sa adresa NULL. Ak zodpovedajuci blok vobec neexistuje alebo su zadane
 * parametre chybne, je vratena hodnota -1 a ulozena adresa NULL.
 *
 * @param physAddr fyzicka adresa do pamate - 0 az FFFFh
 * @param len velkost pozadovaneho bloku
 * @param page pozadovane cislo stranky
 * @param oper pozadovany typ operacie - OP_WRITE alebo OP_READ
 * @point adresa premennej do ktorej sa ulozi adresa do virtualneho priestoru
 * @return pocet bytov, ktore sa "vojdu" do najdeneho bloku
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
