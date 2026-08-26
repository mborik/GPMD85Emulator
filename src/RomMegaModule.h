/*	RomMegaModule.h: Class for emulation of plugged ROM MEGAmodule
	Copyright (c) 2023 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2024 Jan Krupa <apc.atari@gmail.com>

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
#ifndef RomMegaModuleH
#define RomMegaModuleH
//---------------------------------------------------------------------------
#include "RomModule.h"
//---------------------------------------------------------------------------
#define MEGA_MODULE_MASK      0xFF

#define MEGA_MODULE_ADR       0x6F
#define MEGA_MODULE_MAX_PAGES 256
//---------------------------------------------------------------------------
class RomMegaModule: public RomModule
{
	public:
		RomMegaModule();
		virtual ~RomMegaModule();

		void ResetDevice(int ticks);
		void WriteToDevice(BYTE port, BYTE value, int ticks);

		bool LoadRom(unsigned int size, BYTE *src);
		void ReadFromRom();

		inline int GetCurrentPage() { return page; }

	protected:
		int page;
		BYTE *RomPages[MEGA_MODULE_MAX_PAGES];
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
