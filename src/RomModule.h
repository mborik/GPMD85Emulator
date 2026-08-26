/*	RomModule.h: Class for emulation of plugged ROM module
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

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
#ifndef RomModuleH
#define RomModuleH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIO8255.h"
//---------------------------------------------------------------------------
#define ROM_MODULE_MASK       0x8C
#define ROM_MODULE_ADR        0x88

#define ROM_MODULE_REG_MASK   0x8F
#define ROM_MODULE_REG_A      0x88
#define ROM_MODULE_REG_B      0x89
#define ROM_MODULE_REG_C      0x8A
#define ROM_MODULE_REG_CWR    0x8B

#define ROM_PACK_SIZE_KB      32
#define ROM_PACK_SIZE         (ROM_PACK_SIZE_KB * 1024)
//---------------------------------------------------------------------------
class RomModule: public PeripheralDevice, public ChipPIO8255
{
	public:
		RomModule();
		virtual ~RomModule();

		virtual void ResetDevice(int ticks);
		virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
		virtual BYTE ReadFromDevice(BYTE port, int ticks);

		bool InsertRom(BYTE addressKB, BYTE sizeKB, BYTE *src);
		void RemoveRom(BYTE addressKB, BYTE sizeKB);
		void RemoveRomPack();

		void ReadFromRom();

	protected:
		BYTE *RomPack;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
