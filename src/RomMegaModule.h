/*	RomMegaModule.h: Class for emulation of plugged ROM MEGAmodule
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>
	              2024 Jan Krupa <apc.atari@gmail.com>

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

		virtual void ResetDevice(int ticks);
		virtual void WriteToDevice(BYTE port, BYTE value, int ticks);

		bool LoadRom(unsigned int size, BYTE *src);

		void ReadFromRom();

	protected:
		int page;
		BYTE *RomPages[MEGA_MODULE_MAX_PAGES];
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
