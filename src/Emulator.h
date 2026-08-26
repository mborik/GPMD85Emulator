/*	Emulator.h: Core of emulation and interface.
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2026 Martin Borik <martin@borik.net>

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
//-----------------------------------------------------------------------------
#ifndef EMULATOR_H_
#define EMULATOR_H_
//-----------------------------------------------------------------------------
#include "ChipCpu8080P.h"
#include "ChipMemory.h"
#include "ChipMemory12.h"
#include "ChipMemory2A.h"
#include "ChipMemory2AEx.h"
#include "ChipMemory3.h"
#include "ChipMemory3Ex.h"
#include "ChipMemoryC2717.h"
#include "ChipMemoryMato.h"
#include "SystemPIO.h"
#include "IifTimer.h"
#include "IifTape.h"
#include "IifTapePMD85.h"
#include "IifTapeMato.h"
#include "IifGPIO.h"
#include "Joy4004482.h"
#include "Mouse602.h"
#include "Pmd32.h"
#include "RomModule.h"
#include "RomMegaModule.h"
#include "Settings.h"
#include "Debugger.h"
#include "TapeBrowser.h"
#include "ScreenPMD85.h"
#include "UserInterface.h"
#include "SoundDriver.h"
//-----------------------------------------------------------------------------
class TEmulator
{
	public:
		bool isActive;
		bool isRunning;
		BYTE keyBuffer[SDL_NUM_SCANCODES];

		TEmulator();
		virtual ~TEmulator();
		inline void RefreshDisplay() { video->RefreshDisplay(); }

		void ProcessArgvOptions(bool memModifiers = false);
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
		void ActionDiskImages();
		void ActionPMD32LoadDisk(int drive);
		void ActionSnapLoad();
		void ActionSnapSave();
		void ActionRawFile(bool save);
		void ActionROMLoad();
		void ActionMegaRomLoad();
		int  ActionMegaModulePage(bool set = false, BYTE page = 0);

		void ActionReset();
		void ActionHardReset();
		void ActionSound(BYTE action);
		void ActionPlayPause();
		void ActionPlayPause(bool play, bool globalChange = true);
		void ActionSpeedChange();
		void ActionSizeChange(int mode);
		void ActionMouseState(int x, int y, int leftBtn = 0, int rightBtn = 0, int middleBtn = 0);
		void ActionHideCursor(bool hide = false);
		int  ActionJoyControllers(SDL_GameController ***controllers = NULL, bool refresh = false);

		// access to ScreenPMD85 class
		ScreenPMD85 *video;

		sigslot::signal<> actionCallback;

	private:
		bool inmenu;
		int  cpuUsage;

		ChipCpu8080 *cpu;
		ChipMemory  *memory;
		SoundDriver *sound;

		SystemPIO   *systemPIO;
		IifTimer    *ifTimer;
		IifTape     *ifTape;
		IifGPIO     *ifGpio;
		Joy4004482  *joystick;
		Mif85       *mif85;
		Mouse602    *mouse602;
		Pmd32       *pmd32;
		RomModule   *romModule;

		TComputerModel model;
		int  monitorLength;

		bool romChanged;
		bool romSplit8kMode;
		bool compatibilityMode;
		bool ramExpansion256k;
		bool matoAllRAM64k;

		bool mouse602connected;
		bool mif85connected;
		bool pmd32connected;
		int  pmd32workdrive;
		bool romModuleConnected;
		bool megaModuleEnabled;

		void SetComputerModel(bool fromSnap = false, int snapRomLen = 0, BYTE *snapRom = NULL);
		void InsertRomModule(bool inserted);
		void InsertRomMegaModule(bool inserted);
		void ConnectMIF85(bool init);
		void ConnectMouse602(bool init);
		void ConnectPMD32(bool init);
		void ProcessSnapshot(char *fileName, BYTE *flag);
		void PrepareSnapshot(char *fileName, BYTE *flag);
		void InsertTape(char *fileName, BYTE *flag);
		void SaveTape(char *fileName, BYTE *flag);
		void InsertPMD32Disk(char *fileName, BYTE *flag);
		void ChangeROMFile(char *fileName, BYTE *flag);
		void ChangeMegaRomFile(char *fileName, BYTE *flag);
		void SelectRawFile(char *fileName, BYTE *flag);
};
//---------------------------------------------------------------------------
extern TEmulator *Emulator;
extern TSettings *Settings;
extern TDebugger *Debugger;
extern TTapeBrowser *TapeBrowser;
extern UserInterface *GUI;
//-----------------------------------------------------------------------------
#endif
