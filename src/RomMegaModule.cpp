/*	RomMegaModule.cpp: Class for emulation of plugged ROM MEGAmodule
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
	OnCpuReadA.connect(&RomMegaModule::ReadFromRom, this);
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
 * Method is called by the processor when writing to the PIO ports of the RomModule.
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

	debug("RomMegaModule", "MEGAmodule loaded (%d bytes, %d pages)", size, ++i);
	return true;
}
//---------------------------------------------------------------------------
void RomMegaModule::ReadFromRom()
{
	RomModule::ReadFromRom();
}
//---------------------------------------------------------------------------
