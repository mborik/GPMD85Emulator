/*  ChipMemoryC2717.h: Derived class for memory management of Consul 2717
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
#ifndef ChipMemoryC2717H
#define ChipMemoryC2717H
//---------------------------------------------------------------------------
#include "ChipMemory.h"
//---------------------------------------------------------------------------
class ChipMemoryC2717 : public ChipMemory {
public:
	ChipMemoryC2717(BYTE initRomSizeKB);

	virtual void ResetOn();
	virtual void ResetOff();

	virtual inline BYTE* GetVramPointer();
	virtual int FindPointer(int physAddr, int len, int oper, BYTE **ptr);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
