/*	GPMD85main.h: Core of emulation and interface.
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2012 Martin Borik <mborik@users.sourceforge.net>

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
//-----------------------------------------------------------------------------
#ifndef GPMD85MAIN_H_
#define GPMD85MAIN_H_
//-----------------------------------------------------------------------------
#include "ChipCpu8080.h"
#include "ChipMemory.h"
#include "SystemPIO.h"
#include "IifTimer.h"
#include "IifTape.h"
#include "IifGPIO.h"
#include "Pmd32.h"
#include "RomModule.h"
#include "RaomModule.h"
#include "Settings.h"
#include "Debugger.h"
#include "TapeBrowser.h"
#include "ScreenPMD85.h"
#include "UserInterface.h"
#include "SoundDriver.h"
//-----------------------------------------------------------------------------
class TEmulator : public sigslot::has_slots<>
{
	public:
		bool isActive;
		bool isRunning;

		SDL_TimerID BaseTimer;
		BYTE *keyBuffer;

		TEmulator();
		virtual ~TEmulator();

		void ProcessSettings(BYTE filter);
		bool ProcessRawFile(bool save);

		void BaseTimerCallback();
		void CpuTimerCallback();
		bool TestHotkeys();

		void ActionExit();
		void ActionDebugger();
		void ActionTapeBrowser();
		void ActionTapePlayStop();
		void ActionTapeNew();
		void ActionTapeLoad(bool import = false);
		void ActionTapeSave();
		void ActionPMD32LoadDisk(int drive);
		void ActionSnapLoad();
		void ActionSnapSave();
		void ActionROMLoad(BYTE type);
		void ActionRawFile(bool save);

		void ActionReset();
		void ActionHardReset();
		void ActionSound(BYTE action);
		void ActionPlayPause();
		void ActionPlayPause(bool play, bool globalChange = true);
		void ActionSpeedChange();
		void ActionSizeChange(int mode);

	private:
		bool inmenu;

		int cpuUsage;
		int ledState;
		int diskIcon;
		int tapeIcon;
		int diskHold;

		ChipCpu8080 *cpu;
		ChipMemory *memory;
		ScreenPMD85 *video;
		SoundDriver *sound;

		SystemPIO *systemPIO;
		IifTimer *ifTimer;
		IifTape *ifTape;
		IifGPIO *ifGpio;
		Pmd32 *pmd32;
		RomModule *romModule;
		RaomModule *raomModule;

		BYTE *videoRam;
		TComputerModel model;
		bool compatible32;
		int  monitorLength;
		bool romChanged;

		int  pmd32workdrive;
		bool pmd32connected;
		bool romModuleConnected;
		bool raomModuleConnected;

		void SetComputerModel(bool fromSnap = false, int snapRomLen = 0, BYTE *snapRom = NULL);
		void InsertRomModul(bool inserted, bool toRaom);
		void ConnectPMD32(bool init);
		void ProcessSnapshot(char *fileName, BYTE *flag);
		void PrepareSnapshot(char *fileName, BYTE *flag);
		void InsertTape(char *fileName, BYTE *flag);
		void SaveTape(char *fileName, BYTE *flag);
		void InsertPMD32Disk(char *fileName, BYTE *flag);
		void ChangeROMFile(char *fileName, BYTE *flag);
		void SelectRawFile(char *fileName, BYTE *flag);
};
//---------------------------------------------------------------------------
extern TEmulator *Emulator;
extern TSettings *Settings;
extern TDebugger *Debugger;
extern TTapeBrowser *TapeBrowser;
extern UserInterface *GUI;
//-----------------------------------------------------------------------------
inline DWORD FormMain_BaseTimerCallback(DWORD interval, void *param)
{
	((TEmulator *) param)->BaseTimerCallback();
	return interval;
}
//-----------------------------------------------------------------------------
#endif
