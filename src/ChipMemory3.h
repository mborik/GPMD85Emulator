/*  ChipMemory3.h: Derived class for memory management of Model 3
    Copyright (c) 2015-2016 Roman Borik <pmd85emu@gmail.com>

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
#ifndef ChipMemory3H
#define ChipMemory3H
//---------------------------------------------------------------------------
#include "ChipMemory.h"
//---------------------------------------------------------------------------
class ChipMemory3 : public ChipMemory {
public:
	ChipMemory3(BYTE initRomSizeKB);

	virtual void ResetOn();
	virtual void ResetOff();

	virtual inline BYTE* GetVramPointer();
	virtual int FindPointer(int physAddr, int len, int oper, BYTE **ptr);

private:
	BYTE* pointers[2][8]; // pointers to 8kB memory blocks
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
