/*	RomMegaModule.cpp: Class for emulation of plugged ROM MEGAmodule
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
#include "RomMegaModule.h"
//---------------------------------------------------------------------------
RomMegaModule::RomMegaModule()
{
	page = 0;
	RomPages[0] = new BYTE[ROM_PACK_SIZE];
	memset(RomPages[0], 0xFF, ROM_PACK_SIZE);
	RomPack = RomPages[0];
	for (int i = 1; i < MEGA_MODULE_MAX_PAGES; i++)
		RomPages[i] = NULL;
	OnCpuReadA.connect(this, &RomMegaModule::ReadFromRom);
}
//---------------------------------------------------------------------------
RomMegaModule::~RomMegaModule()
{
	for (int i = 0; i < MEGA_MODULE_MAX_PAGES; i++) {
		if (RomPages[i] != NULL) {
			delete [] RomPages[i];
			RomPages[i] = NULL;
		}
	}
	RomPack = NULL;
}
//---------------------------------------------------------------------------
void RomMegaModule::ResetDevice(int ticks)
{
	debug("RomMegaModule", "Reset to page 0");
	RomModule::ResetDevice(ticks);
	page = 0;
	RomPack = RomPages[0];
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty PIO RomModulu.
 */
void RomMegaModule::WriteToDevice(BYTE port, BYTE value, int ticks)
{
	if ((port & MEGA_MODULE_MASK) == MEGA_MODULE_ADR) {
		debug("RomMegaModule", "Selected page %d", value);

		page = value;
		RomPack = RomPages[page];
	}
	else
		RomModule::WriteToDevice(port, value, ticks);
}
//---------------------------------------------------------------------------
bool RomMegaModule::LoadRom(unsigned int size, BYTE *src)
{
	int remain = size;
	int toCopy, i;

	if (size == 0 || size > MEGA_MODULE_MAX_PAGES * ROM_PACK_SIZE)
		return false;

	for (i = 0; i < MEGA_MODULE_MAX_PAGES; i++)
	{
		if (RomPages[i] == NULL) {
			RomPages[i] = new BYTE[ROM_PACK_SIZE];
		}
		memset(RomPages[i], 0xFF, ROM_PACK_SIZE);
		toCopy = remain > ROM_PACK_SIZE ? ROM_PACK_SIZE : remain;
		memcpy(RomPages[i], src + i * ROM_PACK_SIZE, toCopy);

		remain -= toCopy;
		if (remain == 0)
			break;
	}

	debug("RomMegaModule", "Mega module loaded (%d bytes, %d pages)", size, ++i);
	return true;
}
//---------------------------------------------------------------------------
void RomMegaModule::ReadFromRom()
{
	RomModule::ReadFromRom();
}
//---------------------------------------------------------------------------
