/*  ChipMemory3Ex.h: Derived class for memory management and
		peripheral device handling of memory expansion for Model 3.
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
#ifndef ChipMemory3ExH
#define ChipMemory3ExH
//---------------------------------------------------------------------------
#include "ChipMemory.h"
#include "PeripheralDevice.h"
//---------------------------------------------------------------------------
class ChipMemory3Ex : public ChipMemory, public PeripheralDevice {
public:
	ChipMemory3Ex(BYTE initRomSizeKB);

	virtual void ResetOn();
	virtual void ResetOff();

	virtual BYTE* GetVramPointer();
	virtual BYTE GetPage();
	virtual void SetPage(BYTE btPage);
	virtual int FindPointer(int physAddr, int len, int oper, BYTE **ptr);

	virtual inline void ResetDevice(int ticks) { ResetOn(); }
	virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE ReadFromDevice(BYTE port, int ticks);

private:
	void FillPointers(int part, int map, int bank);

	BYTE* pointers[2][8]; // pointers to 8kB memory blocks

	int  bank;   // 16kB memory bank number <0, 15>
	int  map;    // bank mapping into 16kB memspace <0, 3>
	bool vram2;  // second VRAM
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
