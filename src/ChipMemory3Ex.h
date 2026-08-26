/*	ChipMemory3Ex.h: Derived class for memory management and
		peripheral device handling of memory expansion for Model 3.
	Copyright (c) 2015-2016 Roman Borik <pmd85emu@gmail.com>

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

	virtual inline BYTE* GetVramPointer();
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
