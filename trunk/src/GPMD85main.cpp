/*	GPMD85main.cpp: Core of emulation and interface.
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
#include "CommonUtils.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
TEmulator *Emulator;
//-----------------------------------------------------------------------------
TEmulator::TEmulator()
{
	cpu = NULL;
	memory = NULL;
	video = NULL;
	videoRam = NULL;
	systemPIO = NULL;
	ifTimer = NULL;
	ifTape = NULL;
	ifGpio = NULL;
	pmd32 = NULL;
	romModule = NULL;
	raomModule = NULL;
	sound = NULL;
	keyBuffer = NULL;
	cpuUsage = 0;

	model = CM_UNKNOWN;
	romChanged = false;
	compatible32 = false;
	pmd32connected = false;
	romModuleConnected = false;
	raomModuleConnected = false;

	Settings = new TSettings();
	Debugger = new TDebugger();
	TapeBrowser = new TTapeBrowser();
	GUI = new UserInterface();

	isRunning = false;
	isActive = false;
	inmenu = false;
}
//-----------------------------------------------------------------------------
TEmulator::~TEmulator()
{
	isRunning = false;

	if (cpu)
		delete cpu;
	cpu = NULL;

	videoRam = NULL;
	if (video)
		delete video;
	video = NULL;

	if (memory)
		delete memory;
	memory = NULL;

	keyBuffer = NULL;
	if (systemPIO)
		delete systemPIO;
	systemPIO = NULL;

	if (ifTimer)
		delete ifTimer;
	ifTimer = NULL;

	if (ifTape)
		delete ifTape;
	ifTape = NULL;

	if (pmd32) // must be preceded by ifGpio
		delete pmd32;
	pmd32 = NULL;

	if (ifGpio)
		delete ifGpio;
	ifGpio = NULL;

	if (romModule)
		delete romModule;
	romModule = NULL;

	if (raomModule)
		delete raomModule;
	raomModule = NULL;

	if (sound)
		delete sound;
	sound = NULL;

	if (GUI)
		delete GUI;
	GUI = NULL;

	if (TapeBrowser)
		delete TapeBrowser;
	TapeBrowser = NULL;

	if (Debugger)
		delete Debugger;
	Debugger = NULL;

	if (Settings)
		delete Settings;
	Settings = NULL;
}
//-----------------------------------------------------------------------------
void TEmulator::ProcessSettings(BYTE filter)
{
	filter &= (0xFF ^ PS_CLOSEALL);
	if (filter == 0)
		return;

	if (!isActive)
		video = new ScreenPMD85(Settings->Screen->size, Settings->Screen->border);
	else if (filter & PS_SCREEN_SIZE) {
		video->SetDisplayMode(Settings->Screen->size, Settings->Screen->border);
		Settings->Screen->realsize = (TDisplayMode) video->GetMultiplier();
	}

	if (filter & PS_SCREEN_MODE) {
		video->SetHalfPassMode(Settings->Screen->halfPass);
		video->SetLcdMode(Settings->Screen->lcdMode);

		switch (Settings->Screen->colorPalette) {
			case CL_RGB:
				video->SetColorAttr(0, LIME);
				video->SetColorAttr(1, RED);
				video->SetColorAttr(2, BLUE);
				video->SetColorAttr(3, FUCHSIA);
				break;

			case CL_VIDEO:
				video->SetColorAttr(0, WHITE);
				video->SetColorAttr(1, LIME);
				video->SetColorAttr(2, RED);
				video->SetColorAttr(3, MAROON);
				break;

			case CL_DEFINED:
				video->SetColorAttr(0, Settings->Screen->attr00);
				video->SetColorAttr(1, Settings->Screen->attr01);
				video->SetColorAttr(2, Settings->Screen->attr10);
				video->SetColorAttr(3, Settings->Screen->attr11);
				break;
		}

		video->SetColorProfile(Settings->Screen->colorProfile);
	}

	if (!isActive)
		sound = new SoundDriver(2, Settings->Sound->volume);
	else if (filter & PS_SOUND)
		sound->SetVolume(Settings->Sound->volume);

	if (filter & PS_SOUND) {
		if (Settings->Sound->mute)
			sound->SoundMute();
		else
			sound->SoundOn();
	}

	if (filter & PS_MACHINE) {
		if (!romChanged && model == Settings->CurrentModel->type && compatible32 == Settings->CurrentModel->compatibilityMode)
			filter ^= PS_MACHINE;
		else {
			model = Settings->CurrentModel->type;
			compatible32 = Settings->CurrentModel->compatibilityMode;
		}
	}

	if (!isActive || (filter & PS_MACHINE)) {
		SetComputerModel();
		Debugger->SetParams(cpu, memory, model);
		filter |= PS_PERIPHERALS | PS_CONTROLS;
	}

	if (!isActive || (filter & PS_PERIPHERALS)) {
		ConnectPMD32((filter & PS_MACHINE) | romChanged);

		if (romModuleConnected != Settings->CurrentModel->romModuleInserted) {
			romModuleConnected = Settings->CurrentModel->romModuleInserted;
			InsertRomModul(romModuleConnected, false);
		}

		if (raomModuleConnected != Settings->RaomModule->inserted) {
			raomModuleConnected = Settings->RaomModule->inserted;
			InsertRomModul(raomModuleConnected, false);
		}
	}

	if (!isActive || (filter & (PS_MACHINE | PS_PERIPHERALS)))
		ActionReset();

	if (systemPIO && (filter & PS_CONTROLS)) {
		systemPIO->exchZY  = Settings->Keyboard->changeZY;
		systemPIO->numpad  = Settings->Keyboard->useNumpad;
		systemPIO->extMato = Settings->Keyboard->useMatoCtrl;
	}

	romChanged = false;

	if (!isActive)
		isActive = true;
}
//-----------------------------------------------------------------------------
void TEmulator::BaseTimerCallback()
{
	static DWORD blinkCounter = 0;
	static DWORD lastTick = 0;
	static DWORD nextTick = SDL_GetTicks() + MEASURE_PERIOD;
	static DWORD thisTime = 0;
	static BYTE  frames = 0;
	static BYTE  i = 0;
	static float perc;

	thisTime = SDL_GetTicks();

	if (!isActive)
		return;

	if (blinkCounter >= 500) {
		blinkCounter = 0;
		video->ToggleBlinkStatus();
	}
	else
		blinkCounter += (thisTime - lastTick);

	if (isRunning) {
		if (systemPIO) {
			systemPIO->ScanKeyboard(keyBuffer);
			i = systemPIO->width384;
			if ((i & 1) && (bool)(i & 0xFE) != video->IsWidth384()) {
				video->SetWidth384(i & 0xFE);
				systemPIO->width384 = (i & 0xFE);
			}
		}

		i = (ifTape) ? ifTape->GetTapeIcon() :
			(pmd32) ? pmd32->diskIcon : 0;

		video->SetIconState(i);

		if (videoRam)
			video->FillBuffer(videoRam);
	}

	if (thisTime >= nextTick) {
		perc = cpuUsage * 100.0f / (float)(thisTime - (nextTick - MEASURE_PERIOD));

		video->SetStatusPercentage(GUI->isInMenu() ? 0 : (Settings->isPaused ? -1 : perc));
		video->SetStatusFPS(frames);

		cpuUsage = 0;
		frames = 0;

		nextTick = thisTime + MEASURE_PERIOD;
	}

	video->RefreshDisplay();

	lastTick = thisTime;
	frames++;
}
//-----------------------------------------------------------------------------
void TEmulator::CpuTimerCallback()
{
	DWORD beg = SDL_GetTicks();
	static int tc, tci;
	static WORD wrd, pc;
	static bool cy, fl;

	if (!isActive || !isRunning)
		return;

	if (sound)
		sound->PrepareBuffer();

	fl = (model == CM_MATO) ? false : ifTape->IsFlashLoadOn();

	tc = 0;
	do {
		pc = cpu->GetPC();

		// catch debugger breakpoint
		if (Debugger->CheckBreakPoint(pc)) {
			Debugger->Reset();
			ActionDebugger();
			return;
		}

		// switch PMD 85-3 to compatibility mode
		if (pc == 0xE04C && model == CM_V3 && compatible32)
			cpu->SetPC(0xFFF0);

		if (fl) {
			if (model != CM_V1) {
				if ((pc == 0xEB6C && model == CM_V3) || (pc == 0x8B6C && model != CM_V3)) {
					BYTE byte = 0;
					cy = ifTape->GetTapeByte(&byte);

					wrd = (byte << 8);
					cpu->SetAF(wrd | (cy ? FLAG_CY : 0));
					if (model == CM_V3)
						cpu->SetPC(0xEB9B);
					else
						cpu->SetPC(0x8B9B);

					cpu->SetTCycles(cpu->GetTCycles() + 200);
				}
			}
		}

		// back to debugger after RET, Rx instructions
		if (Debugger->flag == 9 && Debugger->CheckDebugRet(&tci)) {
			Debugger->Reset();
			ActionDebugger();
			return;
		}
		else
			tci = cpu->DoInstruction();

		tc += tci;
		while (tc > 13) {
			tc -= 13;
			cpu->IncTCycles();

			if (systemPIO)
				video->SetLedState(systemPIO->ledState);
		}
	} while (cpu->GetTCycles() < TCYCLES_PER_FRAME);

	cpu->SetTCycles(cpu->GetTCycles() - TCYCLES_PER_FRAME);
	cpuUsage += (SDL_GetTicks() - beg);
}
//-----------------------------------------------------------------------------
bool TEmulator::TestHotkeys()
{
	WORD i, key = 0;

	for (i = SDLK_LAST; i > SDLK_COMPOSE; i--)
		if (keyBuffer[i])
			key = i;

	for (; i >= SDLK_NUMLOCK; i--) {
		if (keyBuffer[i]) {
			switch (i) {
				case SDLK_LSHIFT:
				case SDLK_RSHIFT:
					key |= KM_SHIFT;
					break;
				case SDLK_LCTRL:
				case SDLK_RCTRL:
					key |= KM_CTRL;
					break;
				case SDLK_LALT:
				case SDLK_RALT:
				case SDLK_LMETA:
				case SDLK_RMETA:
				case SDLK_LSUPER:
				case SDLK_RSUPER:
				case SDLK_MODE:
					key |= KM_ALT;
					break;
			}
		}
	}

	for (; i > SDLK_FIRST; i--) {
		if ((i <= SDLK_DELETE || i >= SDLK_KP0) && keyBuffer[i]) {
			key = (key & 0xFE00) | i;
			break;
		}
	}

	// special key replacements
	switch (key) {
		case SDLK_BREAK:
			key = SDLK_ESCAPE;
			break;

		case SDLK_MENU:
			key = KM_ALT | SDLK_F1;
			break;

		case SDLK_PAUSE:
			key = KM_ALT | SDLK_F3;
			break;

		case SDLK_POWER:
			key = KM_ALT | SDLK_F4;
			break;
	}

	if (GUI->isInMenu()) {
		GUI->menuHandleKey(key);

		if (GUI->uiSetChanges & PS_CLOSEALL)
			key = SDLK_ESCAPE;

		if (!GUI->isInMenu() && key == SDLK_ESCAPE) {
			if (GUI->uiSetChanges) {
				ProcessSettings(GUI->uiSetChanges);
				GUI->uiSetChanges = 0;
			}

			if (videoRam)
				video->FillBuffer(videoRam);

			SDL_EnableKeyRepeat(0, 0);

			GUI->uiCallback();
			GUI->uiCallback.disconnect_all();

			if (!GUI->isInMenu()) {
				ActionPlayPause(!Settings->isPaused, false);
				return true;
			}
		}

		bool ret = GUI->needRelease;
		GUI->needRelease = false;
		return ret;
	}

	if (key & KM_ALT) {
		i = key & 0x01FF;

		switch (i) {
			case SDLK_1:	// SCREEN SIZE 1x1
				if (gvi.wm)
					ActionSizeChange(1);
				break;

			case SDLK_2:	// SCREEN SIZE 2x2
				if (gvi.wm)
					ActionSizeChange(2);
				break;

			case SDLK_3:	// SCREEN SIZE 3x3
				if (gvi.wm)
					ActionSizeChange(3);
				break;

			case SDLK_4:	// SCREEN SIZE 4x4
				if (gvi.wm)
					ActionSizeChange(4);
				break;

#ifndef OPENGL
			case SDLK_5:	// SCALER: LCD EMULATION
				video->SetLcdMode(true);
				video->SetHalfPassMode(HP_OFF);
				Settings->Screen->lcdMode = true;
				Settings->Screen->halfPass = HP_OFF;
				break;

			case SDLK_6:	// SCALER: HALFPASS 0%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_0);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_0;
				break;

			case SDLK_7:	// SCALER: HALFPASS 25%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_25);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_25;
				break;

			case SDLK_8:	// SCALER: HALFPASS 50%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_50);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_50;
				break;

			case SDLK_9:	// SCALER: HALFPASS 75%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_75);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_75;
				break;

			case SDLK_0:	// SCALER: PIXEL PRECISE
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_OFF);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_OFF;
				break;
#endif

			case SDLK_f:	// FULL-SCREEN
				if (gvi.wm && (gvi.w + gvi.h))
					ActionSizeChange(0);
				break;

			case SDLK_m:	// MONO/STANDARD MODES
				if (video->GetColorProfile() == CP_STANDARD) {
					video->SetColorProfile(CP_MONO);
					Settings->Screen->colorProfile = CP_MONO;
				}
				else {
					video->SetColorProfile(CP_STANDARD);
					Settings->Screen->colorProfile = CP_STANDARD;
				}
				break;

			case SDLK_c:	// COLOR MODES
				if (video->GetColorProfile() == CP_COLOR) {
					video->SetColorProfile(CP_MULTICOLOR);
					Settings->Screen->colorProfile = CP_MULTICOLOR;
				}
				else {
					video->SetColorProfile(CP_COLOR);
					Settings->Screen->colorProfile = CP_COLOR;
				}
				break;

			case SDLK_p:	// PLAY/STOP TAPE
				ActionTapePlayStop();
				break;

			case SDLK_t:	// TAPE BROWSER
				ActionTapeBrowser();
				break;

			case SDLK_F1:	// MAIN MENU
				ActionPlayPause(false, false);
				GUI->menuOpen(UserInterface::GUI_TYPE_MENU);
				break;

			case SDLK_F2:	// LOAD/SAVE TAPE
				if (key & KM_SHIFT)
					ActionTapeSave();
				else
					ActionTapeLoad();
				break;

			case SDLK_F3:	// PLAY/PAUSE
				ActionPlayPause();
				break;

			case SDLK_F4:	// EXIT
				ActionExit();
				break;

			case SDLK_F5:	// RESET
				if (key & KM_SHIFT)
					ActionHardReset();
				else
					ActionReset();
				break;

			case SDLK_F6:	// DISK IMAGES
				ActionPlayPause(false, false);
				GUI->menuOpen(UserInterface::GUI_TYPE_DISKIMAGES);
				break;

			case SDLK_F7:	// LOAD/SAVE SNAPSHOT
				if (key & KM_SHIFT)
					ActionSnapSave();
				else
					ActionSnapLoad();
				break;

			case SDLK_F8:	// SOUND ON/OFF
				ActionSound(!Settings->Sound->mute);
				break;

			case SDLK_F9:	// MODEL SELECT/MEMORY MENU
				ActionPlayPause(false, false);
				if (key & KM_SHIFT)
					GUI->menuOpen(UserInterface::GUI_TYPE_MEMORY);
				else
					GUI->menuOpen(UserInterface::GUI_TYPE_SELECT);
				break;

			case SDLK_F10:	// PERIPHERALS
				ActionPlayPause(false, false);
				GUI->menuOpen(UserInterface::GUI_TYPE_PERIPHERALS);
				break;

			case SDLK_F11:
				break;

			case SDLK_F12:	// DEBUGGER
				ActionDebugger();
				break;
		}
	}

	return false;
}
//---------------------------------------------------------------------------
void TEmulator::ActionExit()
{
	ActionPlayPause(false, false);

	if (TapeBrowser->tapeChanged) {
		BYTE result = GUI->queryDialog("SAVE CHANGES?", true);
		if (result == GUI_QUERY_SAVE) {
			ActionTapeSave();
			return;
		}
		else if (result != GUI_QUERY_DONTSAVE) {
			GUI->menuCloseAll();
			ActionPlayPause(!Settings->isPaused, false);
			return;
		}
	}

	if (GUI->queryDialog("REALLY EXIT?", false) == GUI_QUERY_YES)
		isActive = false;

	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionDebugger()
{
	ActionPlayPause(false, false);
	GUI->menuOpen(UserInterface::GUI_TYPE_DEBUGGER);
}
//---------------------------------------------------------------------------
void TEmulator::ActionTapeBrowser()
{
	ActionPlayPause(false, false);
	GUI->menuOpen(UserInterface::GUI_TYPE_TAPEBROWSER);
}
//---------------------------------------------------------------------------
void TEmulator::ActionTapePlayStop()
{
	if (TapeBrowser->playing)
		TapeBrowser->ActionStop();
	else
		TapeBrowser->ActionPlay();

	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionTapeNew()
{
	ActionPlayPause(false, false);

	BYTE result = GUI_QUERY_DONTSAVE;
	if (TapeBrowser->tapeChanged) {
		result = GUI->queryDialog("SAVE CHANGES?", true);
		if (result == GUI_QUERY_SAVE) {
			GUI->menuCloseAll();
			ActionTapeSave();
			return;
		}
	}

	if (result == GUI_QUERY_DONTSAVE) {
		TapeBrowser->SetNewTape();
		delete [] Settings->TapeBrowser->fileName;
		Settings->TapeBrowser->fileName = NULL;
		GUI->menuCloseAll();
	}

	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionTapeLoad(bool import)
{
	static const char *tape_filter[] = { "ptp", "pmd", NULL };
	static char tape_title[32] = "";

	ActionPlayPause(false, false);

	if (!import && TapeBrowser->tapeChanged) {
		BYTE result = GUI->queryDialog("SAVE CHANGES?", true);
		if (result == GUI_QUERY_SAVE) {
			ActionTapeSave();
			return;
		}
		else if (result != GUI_QUERY_DONTSAVE) {
			GUI->menuCloseAll();
			ActionPlayPause(!Settings->isPaused, false);
			return;
		}
	}

	sprintf(tape_title, "%s TAPE FILE (*.ptp, *.pmd)", (import ? "IMPORT" : "OPEN"));

	GUI->fileSelector->tag = (BYTE) import;
	GUI->fileSelector->type = GUI_FS_BASELOAD;
	GUI->fileSelector->title = tape_title;
	GUI->fileSelector->extFilter = (char **) tape_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::InsertTape);

	char *recentFile = import ? TapeBrowser->orgTapeFile : Settings->TapeBrowser->fileName;
	if (recentFile) {
		char *file = ComposeFilePath(recentFile);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		while (!TestDir(GUI->fileSelector->path, (char *) "..", NULL));
	}
	else
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->menuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionTapeSave()
{
	static const char *tape_filter[] = { "ptp", NULL };

	ActionPlayPause(false, false);

	GUI->fileSelector->type = GUI_FS_BASESAVE;
	GUI->fileSelector->title = "SAVE TAPE FILE (*.ptp)";
	GUI->fileSelector->extFilter = (char **) tape_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::SaveTape);

	if (Settings->TapeBrowser->fileName) {
		char *file = ComposeFilePath(Settings->TapeBrowser->fileName);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		while (!TestDir(GUI->fileSelector->path, (char *) "..", NULL));
	}
	else
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->menuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionPMD32LoadDisk(int drive)
{
	static const char *p32_filter[] = { "p32", NULL };

	ActionPlayPause(false, false);

	GUI->fileSelector->type = GUI_FS_BASELOAD;
	GUI->fileSelector->title = "OPEN PMD 32 DISK FILE (*.p32)";
	GUI->fileSelector->extFilter = (char **) p32_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::InsertPMD32Disk);
	pmd32workdrive = drive;

	char *fileName = NULL;
	switch (drive) {
		case 1:
			fileName = Settings->PMD32->driveA.image;
			break;
		case 2:
			fileName = Settings->PMD32->driveB.image;
			break;
		case 3:
			fileName = Settings->PMD32->driveC.image;
			break;
		case 4:
			fileName = Settings->PMD32->driveD.image;
			break;
		default:
			break;
	}

	if (fileName) {
		fileName = ComposeFilePath(fileName);
		strcpy(GUI->fileSelector->path, fileName);
		delete [] fileName;

		while (!TestDir(GUI->fileSelector->path, (char *) "..", NULL));
	}
	else
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->menuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionSnapLoad()
{
	static const char *snap_filter[] = { "psn", NULL };

	ActionPlayPause(false, false);

	GUI->fileSelector->type = GUI_FS_SNAPLOAD;
	GUI->fileSelector->title = "OPEN SNAPSHOT FILE (*.psn)";
	GUI->fileSelector->extFilter = (char **) snap_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::ProcessSnapshot);

	if (Settings->Snapshot->fileName) {
		char *file = ComposeFilePath(Settings->Snapshot->fileName);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		while (!TestDir(GUI->fileSelector->path, (char *) "..", NULL));
	}
	else
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->menuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionSnapSave()
{
	static const char *snap_filter[] = { "psn", NULL };

	ActionPlayPause(false, false);

	GUI->fileSelector->type = GUI_FS_SNAPSAVE;
	GUI->fileSelector->title = "SAVE SNAPSHOT FILE (*.psn)";
	GUI->fileSelector->extFilter = (char **) snap_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::PrepareSnapshot);

	if (Settings->Snapshot->fileName) {
		char *file = ComposeFilePath(Settings->Snapshot->fileName);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		while (!TestDir(GUI->fileSelector->path, (char *) "..", NULL));
	}
	else
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->menuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionROMLoad(BYTE type)
{
	static const char *rom_filter[] = { "rom", NULL };
	char *fileName;

	ActionPlayPause(false, false);

	if (type == 32)
		fileName = Settings->PMD32->romFile;
	else
		fileName = Settings->CurrentModel->romFile;

	GUI->fileSelector->tag = type;
	GUI->fileSelector->type = GUI_FS_BASELOAD;
	GUI->fileSelector->title = "SELECT ROM FILE (*.rom)";
	GUI->fileSelector->extFilter = (char **) rom_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::ChangeROMFile);

	if (fileName) {
		char *file = LocateROM(fileName);
		if (file == NULL)
			file = fileName;

		strcpy(GUI->fileSelector->path, file);
		while (!TestDir(GUI->fileSelector->path, (char *) "..", NULL));
	}
	else {
		if (stat(PathAppConfig, &CommonUtils::filestat) == 0)
			strcpy(GUI->fileSelector->path, PathAppConfig);
		else if (stat(PathResources, &CommonUtils::filestat) == 0)
			strcpy(GUI->fileSelector->path, PathResources);
		else
			strcpy(GUI->fileSelector->path, PathApplication);
	}

	GUI->menuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionReset()
{
	cpu->Reset();
	if (model != CM_V3)
		memory->Page = 2;
}
//---------------------------------------------------------------------------
void TEmulator::ActionHardReset()
{
	ActionPlayPause(false, false);
	ProcessSettings(PS_MACHINE | PS_PERIPHERALS);
	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionSound(bool mute)
{
	Settings->Sound->mute = mute;

	if (mute)
		sound->SoundMute();
	else
		sound->SoundOn();
}
//---------------------------------------------------------------------------
void TEmulator::ActionPlayPause()
{
	ActionPlayPause(Settings->isPaused, true);
}
//---------------------------------------------------------------------------
void TEmulator::ActionPlayPause(bool play, bool globalChange)
{
	if (isRunning == play)
		return;

	isRunning = play;

	if (globalChange)
		Settings->isPaused = !isRunning;

	if (play && !Settings->Sound->mute)
		sound->SoundOn();
	else
		sound->SoundMute();
}
//---------------------------------------------------------------------------
void TEmulator::ActionSizeChange(int mode)
{
	ActionPlayPause(false, false);

	TDisplayMode newMode = (TDisplayMode) mode;
	if (mode < 0 || mode > 4)
		newMode = DM_NORMAL;

	if (video->GetDisplayMode() != newMode) {
		video->SetDisplayMode(newMode);
		Settings->Screen->size = newMode;
		Settings->Screen->realsize = (TDisplayMode) video->GetMultiplier();
	}

	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::SetComputerModel(bool fromSnap, int snapRomLen, BYTE *snapRom)
{
	int romAdrKB, fileSize;
	BYTE romSize;

	if (cpu)
		delete cpu;
	cpu = NULL;
	videoRam = NULL;
	if (memory)
		delete memory;
	memory = NULL;
	if (ifTape)
		delete ifTape;
	ifTape = NULL;
	if (ifTimer)
		delete ifTimer;
	ifTimer = NULL;
	if (ifGpio)
		delete ifGpio;
	ifGpio = NULL;
	if (systemPIO)
		delete systemPIO;
	systemPIO = NULL;

	BYTE *romBuff = new BYTE[16 * KB];
	memset(romBuff, 0xFF, 16 * KB);

	if (fromSnap == true && snapRomLen >= 4096 && snapRom != NULL) {
		fileSize = snapRomLen;
		memcpy(romBuff, snapRom, snapRomLen);
	}
	else {
		char *romFile = LocateROM(Settings->CurrentModel->romFile);
		if (romFile == NULL)
			romFile = Settings->CurrentModel->romFile;
		fileSize = ReadFromFile(romFile, 0, 16 * KB, romBuff);
		if (fileSize < 0)
			warning("Error reading ROM file!\n%s", romFile);
	}

	fileSize = (fileSize + KB - 1) & (~(KB - 1));
	romSize = (BYTE) (fileSize / KB);
	if (romSize < 4)
		romSize = 4;
	monitorLength = romSize * KB;
	romAdrKB = 32;

	video->SetComputerModel(model);

	switch (model) {
		case CM_UNKNOWN :
			delete[] romBuff;
			return;

		case CM_V1 :    // PMD 85-1
		case CM_ALFA :  // Didaktik Alfa
		case CM_ALFA2 : // Didaktik Alfa 2
		case CM_V2 :    // PMD 85-2
			memory = new ChipMemory((WORD)(48 + romSize));  // 48 kB RAM, x kB ROM

			// nestrankovana pamat - vzdy pristupna
			memory->AddBlock( 4, 28,  4096L, NO_PAGED, MA_RW);  // RAM 1000h - 7FFFh

			if (romSize > 8)
			{
				// nebude sa zrkadlit
				memory->AddBlock(32, romSize, 49152L, NO_PAGED, MA_RO);  // ROM od 8000h
				if (romSize < 16)
					memory->AddBlock((BYTE)(32 + romSize), (BYTE)((WORD)(16 - romSize)), 0L, NO_PAGED, MA_NA);  // nic do 0xBFFF
			}
			else
			{
				// ROM od 8000h
				memory->AddBlock(32,  romSize, 49152L, NO_PAGED, MA_RO);
				if (romSize < 8) // nic do 0x9FFF
					memory->AddBlock((BYTE)(32 + romSize), (BYTE)((WORD)(8 - romSize)), 0L, NO_PAGED, MA_NA);
				// zrkadlo ROM od A000h
				memory->AddBlock(40,  romSize, 49152L, NO_PAGED, MA_RO);
				if (romSize < 8) // nic do 0xBFFF
					memory->AddBlock((BYTE)(40 + romSize), (BYTE)((WORD)(8 - romSize)), 0L, NO_PAGED, MA_NA);
			}

			// RAM C000h - FFFFh
			memory->AddBlock(48, 16, 32768L, NO_PAGED, MA_RW);

			// stranka 1 - normal
			memory->AddBlock( 0,  4,     0L,  1, MA_RW); // RAM 0000h - 0FFFh

			// stranka 2 - po resete
			memory->AddBlock( 0,  romSize, 49152L,  2, MA_RO); // ROM od 0000h
			memory->AddBlock(32,  romSize, 49152L,  2, MA_RO); // ROM od 8000h
			break;

		case CM_V2A : // PMD 85-2A
			memory = new ChipMemory((WORD)(64 + romSize));  // 64 kB RAM, x kB ROM

			// nestrankovana pamat - vzdy pristupna
			memory->AddBlock( 4, 28,  4096L, NO_PAGED, MA_RW);  // RAM 1000h - 7FFFh
			memory->AddBlock(48, 16, 49152L, NO_PAGED, MA_RW);  // RAM C000h - FFFFh

			// stranka 0 - All RAM
			memory->AddBlock( 0,  4,     0L,  0, MA_RW);  // RAM 0000h - 0FFFh
			memory->AddBlock(32, 16, 32768L,  0, MA_RW);  // RAM 8000h - BFFFh

			// stranka 1 - citanie z ROM, zapis do RAM
			memory->AddBlock( 0,  4,     0L,  1, MA_RW);  // RAM 0000h - 0FFFh
			if (romSize > 8)
			{
				// nebude sa zrkadlit
				memory->AddBlock(32, romSize, 65536L, 1, MA_RO);  // ROM od 8000h
				memory->AddBlock(32, romSize, 32768L, 1, MA_WO);  // RAM od 8000h
				if (romSize < 16) // RAM do 0xBFFF
					memory->AddBlock((BYTE)(32 + romSize), (BYTE)((WORD)(16 - romSize)), 32768L + romSize * KB, 1, MA_RW);
			}
			else
			{
				// ROM od 8000h
				memory->AddBlock(32, romSize, 65536L, 1, MA_RO);  // ROM od 8000h
				memory->AddBlock(32, romSize, 32768L, 1, MA_WO);  // RAM od 8000h
				if (romSize < 8) // RAM do 0x9FFF
					memory->AddBlock((BYTE)(32 + romSize), (BYTE)((WORD)(8 - romSize)), 32768L + romSize * KB, 1, MA_RW);
				// zrkadlo ROM od A000h
				memory->AddBlock(40, romSize, 65536L, 1, MA_RO);  // ROM od A000h
				memory->AddBlock(40, romSize, 40960L, 1, MA_WO);  // RAM od A000h
				if (romSize < 8) // RAM do 0xBFFF
					memory->AddBlock((BYTE)(40 + romSize), (BYTE)((WORD)(8 - romSize)), 40960L + romSize * KB, 1, MA_RW);
			}

			// stranka 2 - po resete
			memory->AddBlock( 0,  romSize, 65536L,  2, MA_RO);  // ROM od 0000h
			memory->AddBlock(32,  romSize, 65536L,  2, MA_RO);  // ROM od 8000h
			break;

		case CM_V3 :  // PMD 85-3
			memory = new ChipMemory(64 + 8); // 64 kB RAM, 8kB ROM

			// stranka 0 - All RAM
			memory->AddBlock( 0, 64,     0L,  0, MA_RW);  // RAM 0000h - FFFFh

			// stranka 1 - citanie z ROM, zapis do RAM
			memory->AddBlock( 0, 56,     0L,  1, MA_RW);  // RAM 0000h - DFFFh
			memory->AddBlock(56,  8, 65536L,  1, MA_RO);  // ROM E000h - FFFFh
			memory->AddBlock(56,  8, 57344L,  1, MA_WO);  // RAM E000h - FFFFh

			// stranka 2 - ak SystemPIO.PC5=1
			memory->AddBlock( 0, 64,     0L,  2, MA_WO);  // RAM 0000h - FFFFh
			memory->AddBlock( 0,  8, 65536L,  2, MA_RO);  // ROM 0000h - 1FFFh
			memory->AddBlock( 8,  8, 65536L,  2, MA_RO);  // ROM 2000h - 3FFFh
			memory->AddBlock(16,  8, 65536L,  2, MA_RO);  // ROM 4000h - 5FFFh
			memory->AddBlock(24,  8, 65536L,  2, MA_RO);  // ROM 6000h - 7FFFh
			memory->AddBlock(32,  8, 65536L,  2, MA_RO);  // ROM 8000h - 9FFFh
			memory->AddBlock(40,  8, 65536L,  2, MA_RO);  // ROM A000h - BFFFh
			memory->AddBlock(48,  8, 65536L,  2, MA_RO);  // ROM C000h - DFFFh
			memory->AddBlock(56,  8, 65536L,  2, MA_RO);  // ROM E000h - FFFFh

			if (romSize > 8)
				monitorLength = 8 * KB;
			romAdrKB = 56;
			break;

		case CM_MATO :
			memory = new ChipMemory(48 + 16); // 48 kB RAM, 16kB ROM

			// pamat je nestrankovana
			memory->AddBlock( 4, 28,  4096L, NO_PAGED, MA_RW);  // RAM 1000h - 7FFFh
			memory->AddBlock(32, 16, 49152L, NO_PAGED, MA_RO);  // ROM 8000h - BFFFh
			memory->AddBlock(48, 16, 32768L, NO_PAGED, MA_RW);  // RAM C000h - FFFFh

			// stranka 1 - normal
			memory->AddBlock( 0,  4,     0L,  1, MA_RW);  // RAM 0000h - 0FFFh

			// stranka 2 - po resete
			memory->AddBlock( 0, 16, 49152L,  2, MA_RO);  // ROM 0000h - 3FFFh
			memory->AddBlock(32, 16, 49152L,  2, MA_RO);  // ROM 8000h - BFFFh

			monitorLength = 16 * KB;
			break;

		case CM_C2717 :  // CONSUL 2717
			memory = new ChipMemory(64 + 16); // 64 kB RAM, 16 kB ROM

			// nestrankovana pamat - vzdy pristupna
			memory->AddBlock( 4, 28,  4096L, NO_PAGED, MA_RW);  // RAM 1000h - 7FFFh
			memory->AddBlock(48, 16, 49152L, NO_PAGED, MA_RW);  // RAM C000h - FFFFh

			// stranka 0 - All RAM
			memory->AddBlock( 0,  4,     0L,  0, MA_RW);  // RAM 0000h - 0FFFh
			memory->AddBlock(32, 16, 32768L,  0, MA_RW);  // RAM 8000h - BFFFh

			// stranka 1 - citanie z ROM, zapis do RAM
			memory->AddBlock( 0,  4,     0L,  1, MA_RW);  // RAM 0000h - 0FFFh
			memory->AddBlock(32, 16, 65536L,  1, MA_RO);  // ROM 8000h - BFFFh

			// stranka 2 - po resete
			memory->AddBlock( 0,  4, 65536L,  2, MA_RO);  // ROM 0000h - 0FFFh
			memory->AddBlock(32, 16, 65536L,  2, MA_RO);  // ROM 8000h - BFFFh

			memory->C2717Remapped = true;
			break;
	}

	videoRam = memory->GetMemPointer(0xC000, (model == CM_V3) ? 0 : -1, OP_READ);

	// CPU
	cpu = new ChipCpu8080(memory);

	// System PIO
	systemPIO = new SystemPIO(model, memory);
	systemPIO->PrepareSample.connect(sound, &SoundDriver::PrepareSample);
	cpu->AddDevice(SYSTEM_PIO_ADR, SYSTEM_PIO_MASK, systemPIO, true);

	if (model != CM_MATO) {
		// Sound - tony 1kHz a 4kHz
		cpu->TCyclesListeners.connect(systemPIO, &SystemPIO::SoundService);

		// GPIO
		ifGpio = new IifGPIO();
		cpu->AddDevice(IIF_GPIO_ADR, IIF_GPIO_MASK, ifGpio, true);

		// Timer
		ifTimer = new IifTimer();
		cpu->AddDevice(IIF_TIMER_ADR, IIF_TIMER_MASK, ifTimer, false);
		cpu->TCyclesListeners.connect(ifTimer, &IifTimer::ITimerService);

		// Tape
		ifTape = new IifTape(model);
		ifTape->PrepareSample.connect(sound, &SoundDriver::PrepareSample);
		TapeBrowser->SetIfTape(ifTape);

		if (model == CM_ALFA || model == CM_ALFA2)
			cpu->AddDevice(IIF_TAPE_ADR_A, IIF_TAPE_MASK_A, ifTape, true);
		else
			cpu->AddDevice(IIF_TAPE_ADR, IIF_TAPE_MASK, ifTape, true);

		if (model != CM_V1 && model != CM_ALFA) {
			ifTimer->Counters[1].OnOutChange.connect(ifTape, &IifTape::TapeClockService23);
			ifTimer->EnableUsartClock(true);
		}

		cpu->TCyclesListeners.connect(ifTape, &IifTape::TapeClockService123);
	}

	if (model != CM_C2717)
		systemPIO->width384 = 1;

	if (fileSize > 0)
		memory->PutRom((BYTE) romAdrKB, 2, romBuff, monitorLength);

	delete[] romBuff;
}
//---------------------------------------------------------------------------
void TEmulator::InsertRomModul(bool inserted, bool toRaom)
{
	int i, count, sizeKB;
	DWORD romPackSizeKB, kBadr;
	TSettings::SetRomModuleFile **rl;
	BYTE *buff;

	if (!toRaom && romModule) {
		cpu->RemoveDevice(ROM_MODULE_ADR);
		delete romModule;
		romModule = NULL;
	}

	if (toRaom && raomModule) {
		if (!romModuleConnected || ROM_MODULE_ADR != RAOM_MODULE_ADR)
			cpu->RemoveDevice(RAOM_MODULE_ADR);

		delete raomModule;
		raomModule = NULL;
	}

	if (!inserted)
		return;

	if (toRaom) {
		raomModule = new RaomModule((TRaomType) Settings->RaomModule->type);
		cpu->AddDevice(RAOM_MODULE_ADR, RAOM_MODULE_MASK, raomModule, true);
		raomModule->RemoveRomPack();
		romPackSizeKB = raomModule->GetRomSize() / KB;

		if (FileExists(Settings->RaomModule->file) == true)
			raomModule->InsertDisk(Settings->RaomModule->file);
		else
			raomModule->RemoveDisk();

		rl = Settings->RaomModule->module->files;
		count = Settings->RaomModule->module->count;
	}
	else {
		romModule = new RomModule();
		cpu->AddDevice(ROM_MODULE_ADR, ROM_MODULE_MASK, romModule, true);
		romModule->RemoveRomPack();
		romPackSizeKB = ROM_PACK_SIZE_KB;

		rl = Settings->CurrentModel->romModule->files;
		count = Settings->CurrentModel->romModule->count;
	}

	kBadr = 0;
	buff = new BYTE[romPackSizeKB * KB];
	for (i = 0; i < count && kBadr < romPackSizeKB; i++) {
		sizeKB = (rl[i]->size + KB - 1) / KB;
		if ((kBadr + sizeKB) > romPackSizeKB)
			sizeKB = romPackSizeKB - kBadr;

		memset(buff, 0xFF, sizeKB * KB);
		if (ReadFromFile(LocateROM(rl[i]->rmmFile), 0, sizeKB * KB, buff) > 0) {
			if (toRaom)
				raomModule->InsertRom((BYTE) kBadr, (BYTE) sizeKB, buff);
			else
				romModule->InsertRom((BYTE) kBadr, (BYTE) sizeKB, buff);
		}

		kBadr += sizeKB;
	}

	delete[] buff;
}
//---------------------------------------------------------------------------
void TEmulator::ConnectPMD32(bool init)
{
	if (init || (pmd32connected != Settings->PMD32->connected)) {
		if (pmd32) {
			if (!pmd32connected && Settings->PMD32->connected && !init) {
				cpu->TCyclesListeners.disconnect(pmd32);
				ifGpio->OnBeforeResetA.disconnect(pmd32);
				ifGpio->OnAfterResetA.disconnect(pmd32);
				ifGpio->OnCpuWriteCWR.disconnect(pmd32);
				ifGpio->OnCpuWriteCH.disconnect(pmd32);
			}

			video->SetIconState(0);

			delete pmd32;
			pmd32 = NULL;
		}

		if (cpu && ifGpio && Settings->PMD32->connected) {
			pmd32 = new Pmd32(ifGpio, Settings->PMD32->romFile);
			cpu->TCyclesListeners.connect(pmd32, &Pmd32::Disk32Service);
		}

		pmd32connected = Settings->PMD32->connected;
	}

	if (pmd32) {
		if (Settings->PMD32->sdRoot) {
			pmd32->SetExtraCommands(
				Settings->PMD32->extraCommands,
				Settings->PMD32->sdRoot);
		}

		if (Settings->PMD32->driveA.image) {
			pmd32->InsertDisk(DRIVE_A,
				Settings->PMD32->driveA.image,
				Settings->PMD32->driveA.writeProtect);
		}
		if (Settings->PMD32->driveB.image) {
			pmd32->InsertDisk(DRIVE_B,
				Settings->PMD32->driveB.image,
				Settings->PMD32->driveB.writeProtect);
		}
		if (Settings->PMD32->driveC.image) {
			pmd32->InsertDisk(DRIVE_C,
				Settings->PMD32->driveC.image,
				Settings->PMD32->driveC.writeProtect);
		}
		if (Settings->PMD32->driveD.image) {
			pmd32->InsertDisk(DRIVE_D,
				Settings->PMD32->driveD.image,
				Settings->PMD32->driveD.writeProtect);
		}
	}
}
//---------------------------------------------------------------------------
void TEmulator::ProcessSnapshot(char *fileName, BYTE *flag)
{
	BYTE *buf  = new BYTE[SNAP_HEADER_LEN];
	BYTE *src  = new BYTE[SNAP_BLOCK_LEN];
	BYTE *dest = new BYTE[SNAP_BLOCK_LEN];

	TComputerModel oldModel = model, newModel;
	int verzia, offset, len;

	do {
		*flag = 0xFF;
		if (ReadFromFile(fileName, 0, SNAP_HEADER_LEN, buf) != SNAP_HEADER_LEN)
			break;

		if (memcmp(buf, "PSN", 3) != 0)
			break;

		verzia = (int) *(buf + 3);
		offset = (int) *((WORD *) (buf + 4));

		// we have only one version til now...
		if (verzia != 1 || offset != SNAP_HEADER_LEN)
			break;

		newModel = (TComputerModel) *(buf + 6);
		if (newModel > CM_LAST)
			break;

		if ((*((WORD *) (buf + 20)) & 0x7FFF) > SNAP_BLOCK_LEN
		  || *((WORD *) (buf + 22)) > SNAP_BLOCK_LEN
		  || *((WORD *) (buf + 24)) > SNAP_BLOCK_LEN
		  || *((WORD *) (buf + 26)) > SNAP_BLOCK_LEN
		  || *((WORD *) (buf + 28)) > SNAP_BLOCK_LEN)
			break;

		// ROM - Monitor
		int lenX = (int) *((WORD *) (buf + 20));
		if (lenX > 0) {
			len = lenX & 0x7FFF; // lenX.b15 = 1, ak je ROM v cistom tvare
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			offset += len;
			lenX = UnpackBlock(dest, (lenX & 0x8000) ? len : SNAP_BLOCK_LEN, src, len);
			if (lenX < 0)
				break;
		}

		*flag = 2;
		for (int i = 0; i < Settings->modelsCount; i++) {
			if (Settings->AllModels[i]->type == newModel) {
				Settings->CurrentModel = Settings->AllModels[i];
				break;
			}
		}

		model = newModel;
		SetComputerModel((lenX > 0), lenX, dest);

		if (cpu)
			cpu->SetChipState(buf + 7);

		if (model != CM_V3)
			memory->Page = 2;

		Debugger->SetParams(cpu, memory, model);

		if (systemPIO) {
			systemPIO->resetDevice(0);
			systemPIO->writeToDevice(SYSTEM_REG_CWR, *(buf + 30), 0);
			systemPIO->writeToDevice(SYSTEM_REG_C, *(buf + 31), 0);
			systemPIO->writeToDevice(SYSTEM_REG_A, *(buf + 32), 0);
			systemPIO->readFromDevice(SYSTEM_REG_B, 0); // kvoli prestrankovaniu C2717
		}

		if (ifGpio)
			ifGpio->SetChipState(buf + 33);
//		if (ifIms2)
//			ifIms2->SetChipState(buf + 38);
		if (ifTimer)
			ifTimer->SetChipState(buf + 43);
		if (ifTape)
			ifTape->SetChipState(buf + 52);

		// blok 0 - 3FFFh
		len = (int) *((WORD *) (buf + 22));
		if (len > 0) {
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			offset += len;
			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN)
				break;

			memory->PutMem(0, PAGE_ANY, dest, SNAP_BLOCK_LEN);
		}

		// blok 4000h - 7FFFh
		len = (int) *((WORD *) (buf + 24));
		if (len > 0) {
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			offset += len;
			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN)
				break;

			memory->PutMem(0x4000, PAGE_ANY, dest, SNAP_BLOCK_LEN);
		}

		// blok 8000h - 0BFFFh
		if (model == CM_V2A || model == CM_V3 || model == CM_C2717) {
			len = (int) *((WORD *) (buf + 26));
			if (len > 0) {
				if (ReadFromFile(fileName, offset, len, src) != len)
					break;

				offset += len;
				len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
				if (len < 0 || len != SNAP_BLOCK_LEN)
					break;

				memory->PutMem(0x8000, PAGE_ANY, dest, SNAP_BLOCK_LEN);
			}
		}

		// blok 0C000h - 0FFFFh
		len = (int) *((WORD *) (buf + 28));
		if (len > 0) {
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN)
				break;

			memory->PutMem(0xC000, PAGE_ANY, dest, SNAP_BLOCK_LEN);
		}

		*flag = 0;
	} while (false);

	if (*flag == 0xFF)
		GUI->messageBox("INVALID SNAPHOT FORMAT!");
	else if (*flag == 2) {
		GUI->messageBox("CORRUPTED SNAPSHOT!");

		for (int i = 0; i < Settings->modelsCount; i++) {
			if (Settings->AllModels[i]->type == oldModel) {
				Settings->CurrentModel = Settings->AllModels[i];
				break;
			}
		}

		model = oldModel;
		SetComputerModel();
		Debugger->SetParams(cpu, memory, model);
	}
	else {
		delete [] Settings->Snapshot->fileName;
		Settings->Snapshot->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->Snapshot->fileName, fileName);
	}

	delete [] dest;
	delete [] src;
	delete [] buf;
}
//---------------------------------------------------------------------------
void TEmulator::PrepareSnapshot(char *fileName, BYTE *flag)
{
	BYTE *buf  = new BYTE[SNAP_HEADER_LEN];
	BYTE *src  = new BYTE[SNAP_BLOCK_LEN];
	BYTE *dest = new BYTE[SNAP_BLOCK_LEN];
	int len, offset = SNAP_HEADER_LEN;

	memset(buf, 0, SNAP_HEADER_LEN);
	memcpy(buf, "PSN", 3);
	*(buf + 3) = 1; // we have only one version til now...
	*(WORD *) (buf + 4) = SNAP_HEADER_LEN;
	*(buf + 6) = (BYTE) model;

	if (cpu)
		cpu->GetChipState(buf + 7);
	if (systemPIO)
		systemPIO->GetChipState(buf + 30);

	*(buf + 32) = *(buf + 33);
	*(buf + 33) = 0;

	if (ifGpio)
		ifGpio->GetChipState(buf + 33);
//	if (ifIms2)
//		ifIms2->GetChipState(buf + 38);
	if (ifTimer)
		ifTimer->GetChipState(buf + 43);
	if (ifTape)
		ifTape->GetChipState(buf + 52);

	do {
		*flag = 0xFF;
		if (WriteToFile(fileName, 0, SNAP_HEADER_LEN, buf, true) < 0)
			break;

		*flag = 1;

		// ROM block
		if (Settings->Snapshot->saveWithMonitor) {
			switch (model) {
				case CM_V1:
				case CM_V2:
				case CM_ALFA:
				case CM_ALFA2:
				case CM_MATO:
					memory->GetMem(src, 0x8000, -1, monitorLength);
					break;
				case CM_V2A:
				case CM_C2717:
					memory->GetMem(src, 0x8000, 1, monitorLength);
					break;
				case CM_V3:
					memory->GetMem(src, 0xE000, 1, monitorLength);
					break;
				default:
					break;
			}

			if (Settings->Snapshot->saveCompressed)
				len = PackBlock(dest, src, monitorLength);
			else {
				len = monitorLength;
				memcpy(dest, src, len);
			}

			*(WORD *) (buf + 20) = (WORD) (len | ((len == monitorLength) ? 0x8000 : 0));
			if (WriteToFile(fileName, offset, len, dest, false) != len)
				break;

			offset += len;
		}

		// block 0000h - 3FFFh
		memory->GetMem(src, 0, PAGE_ANY, SNAP_BLOCK_LEN);
		if (Settings->Snapshot->saveCompressed)
			len = PackBlock(dest, src, SNAP_BLOCK_LEN);
		else {
			len = SNAP_BLOCK_LEN;
			memcpy(dest, src, len);
		}

		*(WORD *) (buf + 22) = (WORD) len;
		if (WriteToFile(fileName, offset, len, dest, false) != len)
			break;

		offset += len;

		// block 4000h - 7FFFh
		memory->GetMem(src, 16384, PAGE_ANY, SNAP_BLOCK_LEN);
		if (Settings->Snapshot->saveCompressed)
			len = PackBlock(dest, src, SNAP_BLOCK_LEN);
		else {
			len = SNAP_BLOCK_LEN;
			memcpy(dest, src, len);
		}

		*(WORD *) (buf + 24) = (WORD) len;
		if (WriteToFile(fileName, offset, len, dest, false) != len)
			break;

		offset += len;

		// block 8000h - 0BFFFh
		if (model == CM_V2A || model == CM_V3 || model == CM_C2717) {
			memory->GetMem(src, 32768, PAGE_ANY, SNAP_BLOCK_LEN);
			if (Settings->Snapshot->saveCompressed)
				len = PackBlock(dest, src, SNAP_BLOCK_LEN);
			else {
				len = SNAP_BLOCK_LEN;
				memcpy(dest, src, len);
			}

			*(WORD *) (buf + 26) = (WORD) len;
			if (WriteToFile(fileName, offset, len, dest, false) != len)
				break;

			offset += len;
		}

		// block 0C000h - 0FFFFh
		memory->GetMem(src, 49152, PAGE_ANY, SNAP_BLOCK_LEN);
		if (Settings->Snapshot->saveCompressed)
			len = PackBlock(dest, src, SNAP_BLOCK_LEN);
		else {
			len = SNAP_BLOCK_LEN;
			memcpy(dest, src, len);
		}

		*(WORD *) (buf + 28) = (WORD) len;
		if (WriteToFile(fileName, offset, len, dest, false) != len)
			break;

		if (WriteToFile(fileName, 0, SNAP_HEADER_LEN, buf, false) != SNAP_HEADER_LEN)
			break;

		*flag = 0;
	} while (false);

	if (*flag == 0xFF)
		GUI->messageBox("CAN'T OPEN FILE FOR WRITING!");
	else if (*flag == 1) {
		GUI->messageBox("ERROR WRITING FILE...\nSNAPSHOT WILL BE CORRUPTED!");
		*flag = 0;
	}
	else {
		if (Settings->Snapshot->fileName)
			delete [] Settings->Snapshot->fileName;
		Settings->Snapshot->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->Snapshot->fileName, fileName);
	}

	delete [] dest;
	delete [] src;
	delete [] buf;
}
//---------------------------------------------------------------------------
void TEmulator::InsertTape(char *fileName, BYTE *flag)
{
	bool import = (bool) *flag;

	if (import)
		*flag = TapeBrowser->ImportFileName(fileName);
	else
		*flag = TapeBrowser->SetTapeFileName(fileName);

	if (*flag == 0xFF)
		GUI->messageBox("FATAL ERROR!\nOR CAN'T OPEN FILE!");
	else if (*flag == 1) {
		GUI->messageBox("CORRUPTED TAPE FORMAT!");
		*flag = 0;
	}

	if (import) {
		if (TapeBrowser->orgTapeFile)
			delete [] TapeBrowser->orgTapeFile;
		TapeBrowser->orgTapeFile = new char[(strlen(fileName) + 1)];
		strcpy(TapeBrowser->orgTapeFile, fileName);
	}
	else {
		if (Settings->TapeBrowser->fileName)
			delete [] Settings->TapeBrowser->fileName;
		Settings->TapeBrowser->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->TapeBrowser->fileName, fileName);
	}

	GUI->uiCallback.connect(this, &TEmulator::ActionTapeBrowser);
}
//---------------------------------------------------------------------------
void TEmulator::SaveTape(char *fileName, BYTE *flag)
{
	*flag = TapeBrowser->SaveTape(fileName, NULL, true);

	if (*flag == 0xFF)
		GUI->messageBox("FATAL ERROR!\nINVALID NAME OR EXTENSION,\nOR CAN'T OPEN FILE FOR WRITING!");
	else if (*flag == 1) {
		GUI->messageBox("ERROR WRITING FILE...\nTAPE WILL BE CORRUPTED!");
		*flag = 0;
	}
	else {
		delete [] Settings->TapeBrowser->fileName;
		Settings->TapeBrowser->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->TapeBrowser->fileName, fileName);

		int curr = TapeBrowser->currBlockIdx;
		if (TapeBrowser->SetTapeFileName(fileName))
			TapeBrowser->currBlockIdx = curr;
	}
}
//---------------------------------------------------------------------------
void TEmulator::InsertPMD32Disk(char *fileName, BYTE *flag)
{
	*flag = 1;
	switch (pmd32workdrive) {
		case 1:
			delete [] Settings->PMD32->driveA.image;
			Settings->PMD32->driveA.image = new char[strlen(fileName) + 1];
			strcpy(Settings->PMD32->driveA.image, fileName);
			break;
		case 2:
			delete [] Settings->PMD32->driveB.image;
			Settings->PMD32->driveB.image = new char[strlen(fileName) + 1];
			strcpy(Settings->PMD32->driveB.image, fileName);
			break;
		case 3:
			delete [] Settings->PMD32->driveC.image;
			Settings->PMD32->driveC.image = new char[strlen(fileName) + 1];
			strcpy(Settings->PMD32->driveC.image, fileName);
			break;
		case 4:
			delete [] Settings->PMD32->driveD.image;
			Settings->PMD32->driveD.image = new char[strlen(fileName) + 1];
			strcpy(Settings->PMD32->driveD.image, fileName);
			break;
		default:
			*flag = 0xFF;
			break;
	}

	if (*flag == 1)
		ConnectPMD32(false);
}
//---------------------------------------------------------------------------
void TEmulator::ChangeROMFile(char *fileName, BYTE *flag)
{
	char *ptr = (char *) AdaptFilePath(fileName, PathAppConfig);
	if (ptr == fileName) {
		ptr = (char *) AdaptFilePath(fileName, PathResources);
		if (ptr == fileName) {
			char res[MAX_PATH];
			sprintf(res, "%s%c%s", PathApplication, DIR_DELIMITER, "rom");
			ptr = (char *) AdaptFilePath(fileName, res);
			if (ptr == fileName)
				ptr = (char *) AdaptFilePath(fileName);
		}
	}

	if (*flag == 32) {
		delete [] Settings->PMD32->romFile;
		Settings->PMD32->romFile = new char[strlen(ptr) + 1];
		strcpy(Settings->PMD32->romFile, ptr);
		GUI->uiSetChanges |= PS_PERIPHERALS;
	}
	else {
		delete [] Settings->CurrentModel->romFile;
		Settings->CurrentModel->romFile = new char[strlen(ptr) + 1];
		strcpy(Settings->CurrentModel->romFile, ptr);
		GUI->uiSetChanges |= PS_MACHINE;
	}

	romChanged = true;
	*flag = 1;
}
//---------------------------------------------------------------------------
