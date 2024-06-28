/*	Emulator.cpp: Core of emulation and interface.
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2018 Martin Borik <mborik@users.sourceforge.net>

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
#include "ArgvParser.h"
#include "CommonUtils.h"
#include "Emulator.h"
#include "UserInterfaceData.h"
//-----------------------------------------------------------------------------
TEmulator *Emulator;
//-----------------------------------------------------------------------------
TEmulator::TEmulator()
{
	cpu = NULL;
	memory = NULL;
	video = NULL;
	systemPIO = NULL;
	ifTimer = NULL;
	ifTape = NULL;
	ifGpio = NULL;
	mif85 = NULL;
	pmd32 = NULL;
	romModule = NULL;
	sound = NULL;
	cpuUsage = 0;

	SDL_zero(keyBuffer);
	SDL_zero(exposeEvent);
	exposeEvent.type = SDL_WINDOWEVENT;
	exposeEvent.window.type = SDL_WINDOWEVENT;
	exposeEvent.window.event = SDL_WINDOWEVENT_EXPOSED;
	exposeEvent.window.windowID = gdc.windowID;

	model = CM_UNKNOWN;
	romChanged = false;
	romSplit8kMode = false;
	compatibilityMode = false;
	ramExpansion256k = false;
	mif85connected = false;
	pmd32connected = false;
	romModuleConnected = false;
	megaModuleEnabled = false;

	Settings = new TSettings(argv_config.any_related ? argv_config.overcfg : true);
	Debugger = new TDebugger();
	TapeBrowser = new TTapeBrowser();
	GUI = new UserInterface();

	isRunning = false;
	isActive = false;
	inmenu = false;

	if (argv_config.any_related)
		ProcessArgvOptions();
}
//-----------------------------------------------------------------------------
TEmulator::~TEmulator()
{
	isRunning = false;

	if (cpu)
		delete cpu;
	cpu = NULL;

	if (video)
		delete video;
	video = NULL;

	if (memory)
		delete memory;
	memory = NULL;

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

	if (mif85)
		delete mif85;
	mif85 = NULL;

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
void TEmulator::ProcessArgvOptions(bool memModifiers)
{
	if (!memModifiers) {
		TComputerModel cm = (TComputerModel) argv_config.model;

		// Machine
		if (cm != CM_UNKNOWN) {
			for (int i = 0; i < Settings->modelsCount; i++) {
				if (Settings->AllModels[i]->type == cm) {
					Settings->CurrentModel = Settings->AllModels[i];
					break;
				}
			}
		}

		Settings->CurrentModel->romModuleInserted = argv_config.rmm;

		// Screen
		if (argv_config.scaler > 0)
			Settings->Screen->size = (TDisplayMode) argv_config.scaler;

		if (argv_config.border > 0)
			Settings->Screen->border = argv_config.border;

		if (argv_config.halfpass >= 0 && argv_config.halfpass <= 4) {
			Settings->Screen->lcdMode = false;
			Settings->Screen->halfPass = (THalfPassMode) argv_config.halfpass;
		}
		else if (argv_config.halfpass == 5) {
			Settings->Screen->lcdMode = true;
			Settings->Screen->halfPass = HP_OFF;
		}

		if (argv_config.color >= 0)
			Settings->Screen->colorProfile = (TColorProfile) argv_config.color;

		// Sound
		if (argv_config.volume > 0) {
			Settings->Sound->mute = false;
			Settings->Sound->volume = argv_config.volume;
		}
		else if (argv_config.volume == 0)
			Settings->Sound->mute = true;

		Settings->Sound->ifMIF85 = argv_config.mif85;

		// PMD 32
		Settings->PMD32->connected = argv_config.pmd32;
		if (argv_config.p32_drvA) {
			if (Settings->PMD32->driveA.image)
				delete [] Settings->PMD32->driveA.image;
			Settings->PMD32->driveA.image = ComposeFilePath(argv_config.p32_drvA);
			Settings->PMD32->driveA.writeProtect = argv_config.p32_drvA_wp;
		}
		if (argv_config.p32_drvB) {
			if (Settings->PMD32->driveD.image)
				delete [] Settings->PMD32->driveD.image;
			Settings->PMD32->driveD.image = ComposeFilePath(argv_config.p32_drvB);
			Settings->PMD32->driveD.writeProtect = argv_config.p32_drvB_wp;
		}
		if (argv_config.p32_drvC) {
			if (Settings->PMD32->driveC.image)
				delete [] Settings->PMD32->driveC.image;
			Settings->PMD32->driveC.image = ComposeFilePath(argv_config.p32_drvC);
			Settings->PMD32->driveC.writeProtect = argv_config.p32_drvC_wp;
		}
		if (argv_config.p32_drvD) {
			if (Settings->PMD32->driveD.image)
				delete [] Settings->PMD32->driveD.image;
			Settings->PMD32->driveD.image = ComposeFilePath(argv_config.p32_drvD);
			Settings->PMD32->driveD.writeProtect = argv_config.p32_drvD_wp;
		}

		// Tape
		Settings->TapeBrowser->flash = argv_config.flashload;
		if (argv_config.tape) {
			char *fileName = ComposeFilePath(argv_config.tape);
			if (TapeBrowser->SetTapeFileName(fileName) == 0xFF) {
				delete [] fileName;
				fileName = NULL;
			}

			if (Settings->TapeBrowser->fileName)
				delete [] Settings->TapeBrowser->fileName;
			Settings->TapeBrowser->fileName = fileName;
		}
	}
	else {
		// Snapshot
		if (argv_config.snap) {
			BYTE flag;
			char *fileName = ComposeFilePath(argv_config.snap);
			ProcessSnapshot(fileName, &flag);

			if (flag != 0) {
				delete [] fileName;
				fileName = NULL;

				if (Settings->Snapshot->fileName)
					delete [] Settings->Snapshot->fileName;
				Settings->Snapshot->fileName = NULL;
			}
		}

		// Memory block
		if (argv_config.memblock) {
			Settings->MemoryBlock->start = argv_config.memstart;
			Settings->MemoryBlock->length = 65535;
			Settings->MemoryBlock->fileName = ComposeFilePath(argv_config.memblock);

			if (!ProcessRawFile(false)) {
				if (Settings->MemoryBlock->fileName)
					delete [] Settings->MemoryBlock->fileName;
				Settings->MemoryBlock->fileName = NULL;
			}
		}
	}
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
		sound = new SoundDriver(Settings->Sound->volume);
	else if (filter & PS_SOUND)
		sound->SetVolume(Settings->Sound->volume);

	if (filter & PS_SOUND) {
		if (Settings->Sound->mute)
			sound->SoundMute();
		else
			sound->SoundOn();
	}

	if (filter & PS_MACHINE) {
		bool machineCfgChanging = false;

		if (model != Settings->CurrentModel->type) {
			model = Settings->CurrentModel->type;
			machineCfgChanging = true;
		}
		if (romSplit8kMode != Settings->CurrentModel->romSplit8kMode) {
			romSplit8kMode = Settings->CurrentModel->romSplit8kMode;
			machineCfgChanging = true;
		}
		if (ramExpansion256k != Settings->CurrentModel->ramExpansion256k) {
			ramExpansion256k = Settings->CurrentModel->ramExpansion256k;
			machineCfgChanging = true;
		}
		if (compatibilityMode != Settings->CurrentModel->compatibilityMode) {
			compatibilityMode = Settings->CurrentModel->compatibilityMode;
			machineCfgChanging = true;
		}
		if (romModuleConnected != Settings->CurrentModel->romModuleInserted) {
			romModuleConnected = Settings->CurrentModel->romModuleInserted;
			machineCfgChanging = true;
			filter |= PS_PERIPHERALS;
		}
		if (megaModuleEnabled != Settings->CurrentModel->megaModuleEnabled) {
			megaModuleEnabled = Settings->CurrentModel->megaModuleEnabled;
			if (romModuleConnected) {
				machineCfgChanging = true;
				filter |= PS_PERIPHERALS;
			}
		}

		if (!romChanged && !machineCfgChanging)
			filter ^= PS_MACHINE;
	}

	if (!isActive || (filter & PS_MACHINE)) {
		SetComputerModel();
		Debugger->SetParams(cpu, memory, model);
		filter |= PS_PERIPHERALS | PS_CONTROLS;
	}

	if (!isActive || (filter & PS_PERIPHERALS)) {
		bool init = (filter & PS_MACHINE) || romChanged;

		ConnectMIF85(init);
		ConnectPMD32(init);

		if (romModuleConnected) {
			if (megaModuleEnabled)
				InsertRomMegaModule(romModuleConnected);
			else
				InsertRomModule(romModuleConnected);
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

	if (!isActive)
		return;

	thisTime = SDL_GetTicks();
	bool fullRedrawExpected = false;

	// blinking toggle
	if (blinkCounter >= 500) {
		blinkCounter = 0;
		fullRedrawExpected = true;
		video->ToggleBlinkStatus();
	}
	else
		blinkCounter += (thisTime - lastTick);

	if (isRunning) {
		if (systemPIO) {
			systemPIO->ScanKeyboard(keyBuffer);

			// detection of Consul 2717 extended screen mode 384x256...
			BYTE flag = systemPIO->width384;
			if ((flag & 1) && (bool)(flag & 0xFE) != video->IsWidth384()) {
				video->SetWidth384(flag & 0xFE);
				systemPIO->width384 = (flag & 0xFE);
			}
		}

		// status bar icon with priority: ifTape > PMD 32 > none
		BYTE icon = (ifTape) ? ifTape->GetTapeIcon() : 0;
		if (!icon && pmd32 && pmd32->diskIcon)
			icon = pmd32->diskIcon;
		GUI->SetIconState(icon);

		video->FillBuffer(
			memory->GetVramPointer(),
			fullRedrawExpected || memory->WasVramModified()
		);
	}

	// status bar FPS and CPU indicators
	if (thisTime >= nextTick) {
		int perc = GUI->InMenu() - 1; // -> false = -1, true = 0 ;)
		if (!Settings->isPaused)
			perc = (cpuUsage * 100.0f) / (float) (thisTime - (nextTick - MEASURE_PERIOD));

		GUI->SetStatusPercentage(perc);
		GUI->SetStatusFPS(frames);

		nextTick = thisTime + MEASURE_PERIOD;
		cpuUsage = 0;
		frames = 0;
	}

	SDL_PushEvent(&exposeEvent);

	lastTick = thisTime;
	frames++;
}
//-----------------------------------------------------------------------------
void TEmulator::CpuTimerCallback()
{
	if (!isActive || !isRunning)
		return;

	DWORD beg = SDL_GetTicks();
	WORD pc, loader = (0x8B6C | ((model == CM_V3) ? 6000 : 0));
	int tci, tcpf = (TCYCLES_PER_FRAME * Settings->emulationSpeed);

	if (sound)
		sound->PrepareBuffer();

	do {
		pc = cpu->GetPC();

		// catch debugger breakpoint
		if (Debugger->CheckBreakPoint(pc)) {
			Debugger->Reset();
			ActionDebugger();
			return;
		}

		// switch PMD 85-3 to compatibility mode
		if (pc == 0xE04C && model == CM_V3 && compatibilityMode)
			cpu->SetPC(0xFFF0);

		// tape flash loading - ROM routine entry-point mapping
		if (pc == loader && model != CM_V1 && model != CM_MATO &&
		          ifTape && ifTape->IsFlashLoadOn()) {

			BYTE byte = 0;
			bool cy = ifTape->GetTapeByte(&byte);

			cpu->SetAF((byte << 8) | (cy ? FLAG_CY : 0));
			cpu->SetPC(loader + 0x2F);
		}

		// back to debugger after RET, Rx instructions
		if (Debugger->flag == 9 && Debugger->CheckDebugRet(&tci)) {
			Debugger->Reset();
			ActionDebugger();
			return;
		}
		else
			tci = cpu->DoInstruction();

		// status bar LED's state
		// (with pull-up delay simulation)
		if (systemPIO)
			GUI->SetLedState(systemPIO->ledState);

	} while (cpu->GetTCycles() < tcpf);

	cpu->SetTCycles(cpu->GetTCycles() - tcpf);
	cpuUsage += (SDL_GetTicks() - beg);
}
//-----------------------------------------------------------------------------
bool TEmulator::TestHotkeys()
{
	WORD i, key = 0;

	// add shift/ctrl/alt flag
	for (i = 256; i >= SDL_SCANCODE_LCTRL; i--) {
		if (keyBuffer[i]) {
			switch (i) {
				case SDL_SCANCODE_LSHIFT:
				case SDL_SCANCODE_RSHIFT:
					key |= KM_SHIFT;
					break;
				case SDL_SCANCODE_LCTRL:
				case SDL_SCANCODE_RCTRL:
					key |= KM_CTRL;
					break;
				case SDL_SCANCODE_LALT:
				case SDL_SCANCODE_RALT:
				case SDL_SCANCODE_LGUI:
				case SDL_SCANCODE_RGUI:
					key |= KM_ALT;
					break;
			}
		}
	}

	// get one pressed key from main part of keymap buffer
	for (; i > 0; i--) {
		if (keyBuffer[i]) {
			key = (key & 0xFE00) | i;
			break;
		}
	}

	// special key replacements
	switch (key) {
		case SDL_SCANCODE_CANCEL:
			key = SDL_SCANCODE_ESCAPE;
			break;

		case SDL_SCANCODE_APPLICATION:
			key = KM_ALT | SDL_SCANCODE_F1;
			break;

		case SDL_SCANCODE_PAUSE:
			key = KM_ALT | SDL_SCANCODE_F3;
			break;

		case SDL_SCANCODE_POWER:
			key = KM_ALT | SDL_SCANCODE_F4;
			break;
	}

	// menu keyboard handling and interaction of result
	// GUI->uiSetChanges is bit-map of setting changes for ProcessSettings()
	if (GUI->InMenu()) {
		GUI->MenuHandleKey(key);

		// special flag that close all menu windows
		if (GUI->uiSetChanges & PS_CLOSEALL)
			key = SDL_SCANCODE_ESCAPE;

		// if we leave menu and uiSetChanges is set, apply settings change
		if (!GUI->InMenu() && key == SDL_SCANCODE_ESCAPE) {
			if (GUI->uiSetChanges) {
				ProcessSettings(GUI->uiSetChanges);
				GUI->uiSetChanges = 0;
			}

			// perform full redraw of the screen...
			video->FillBuffer(memory->GetVramPointer());

			// menu leaving callback was executed
			GUI->uiCallback();
			GUI->uiCallback.disconnect_all();

			// callback can popup new window,
			// so we test again if we are in menu;
			// if not, we run emulation back to previous state
			if (!GUI->InMenu()) {
				ActionPlayPause(!Settings->isPaused, false);
				return true;
			}
		}

		// send flag to main loop, if we need to release all keys
		bool ret = GUI->needRelease;
		GUI->needRelease = false;
		return ret;
	}

	// keyboard handling in emulation (hotkeys)
	if (key & KM_ALT) {
		i = key & 0x01FF;

		switch (i) {
			case SDL_SCANCODE_1:	// SCREEN SIZE 1x1
				ActionSizeChange(1);
				break;

			case SDL_SCANCODE_2:	// SCREEN SIZE 2x2
				ActionSizeChange(2);
				break;

			case SDL_SCANCODE_3:	// SCREEN SIZE 3x3
				ActionSizeChange(3);
				break;

			case SDL_SCANCODE_4:	// SCREEN SIZE 4x4
				ActionSizeChange(4);
				break;

			case SDL_SCANCODE_5:	// SCREEN SIZE 5x5
				ActionSizeChange(5);
				break;

			case SDL_SCANCODE_L:	// SCANLINER: LCD EMULATION
				video->SetLcdMode(true);
				video->SetHalfPassMode(HP_OFF);
				Settings->Screen->lcdMode = true;
				Settings->Screen->halfPass = HP_OFF;
				break;

			case SDL_SCANCODE_6:	// SCANLINER: HALFPASS 0%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_0);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_0;
				break;

			case SDL_SCANCODE_7:	// SCANLINER: HALFPASS 25%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_25);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_25;
				break;

			case SDL_SCANCODE_8:	// SCANLINER: HALFPASS 50%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_50);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_50;
				break;

			case SDL_SCANCODE_9:	// SCANLINER: HALFPASS 75%
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_75);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_75;
				break;

			case SDL_SCANCODE_0:	// SCANLINER: PIXEL PRECISE
				video->SetLcdMode(false);
				video->SetHalfPassMode(HP_OFF);
				Settings->Screen->lcdMode = false;
				Settings->Screen->halfPass = HP_OFF;
				break;

			case SDL_SCANCODE_F:	// FULL-SCREEN
			case SDL_SCANCODE_RETURN:
				if (Settings->Screen->size == DM_FULLSCREEN)
					ActionSizeChange((int) Settings->Screen->realsize);
				else
					ActionSizeChange(0);
				return true;

			case SDL_SCANCODE_M:	// MONO/STANDARD MODES
				if (video->GetColorProfile() == CP_STANDARD) {
					video->SetColorProfile(CP_MONO);
					Settings->Screen->colorProfile = CP_MONO;
				}
				else {
					video->SetColorProfile(CP_STANDARD);
					Settings->Screen->colorProfile = CP_STANDARD;
				}
				// perform full redraw of the screen...
				video->FillBuffer(memory->GetVramPointer());
				break;

			case SDL_SCANCODE_C:	// COLOR MODES
				if (video->GetColorProfile() == CP_COLOR) {
					video->SetColorProfile(CP_COLORACE);
					Settings->Screen->colorProfile = CP_COLORACE;
				}
				else {
					video->SetColorProfile(CP_COLOR);
					Settings->Screen->colorProfile = CP_COLOR;
				}
				// perform full redraw of the screen...
				video->FillBuffer(memory->GetVramPointer());
				break;

			case SDL_SCANCODE_P:	// PLAY/STOP TAPE
				ActionTapePlayStop();
				break;

			case SDL_SCANCODE_T:	// TAPE BROWSER
				ActionTapeBrowser();
				break;

			case SDL_SCANCODE_F1:	// MAIN MENU
				ActionPlayPause(false, false);
				GUI->MenuOpen(UserInterface::GUI_TYPE_MENU);
				break;

			case SDL_SCANCODE_F2:	// LOAD/SAVE TAPE
				if (key & KM_SHIFT)
					ActionTapeSave();
				else
					ActionTapeLoad();
				break;

			case SDL_SCANCODE_F3:	// PLAY/PAUSE
				if (key & KM_SHIFT)
					ActionSpeedChange();
				else
					ActionPlayPause();
				break;

			case SDL_SCANCODE_F4:	// EXIT
				ActionExit();
				break;

			case SDL_SCANCODE_F5:	// RESET
				if (key & KM_SHIFT)
					ActionHardReset();
				else
					ActionReset();
				break;

			case SDL_SCANCODE_F6:	// DISK IMAGES
				ActionPlayPause(false, false);
				GUI->MenuOpen(UserInterface::GUI_TYPE_MENU, gui_p32_images_menu);
				break;

			case SDL_SCANCODE_F7:	// LOAD/SAVE SNAPSHOT
				if (key & KM_SHIFT)
					ActionSnapSave();
				else
					ActionSnapLoad();
				break;

			case SDL_SCANCODE_F8:	// SOUND ON/OFF
				ActionSound((key & KM_SHIFT) ? -1 : Settings->Sound->mute);
				break;

			case SDL_SCANCODE_F9:	// MODEL SELECT/MEMORY MENU
				ActionPlayPause(false, false);
				if (key & KM_SHIFT)
					GUI->MenuOpen(UserInterface::GUI_TYPE_MENU, gui_mem_menu);
				else
					GUI->MenuOpen(UserInterface::GUI_TYPE_MENU, gui_machine_menu);
				break;

			case SDL_SCANCODE_F10:	// PERIPHERALS
				ActionPlayPause(false, false);
				GUI->MenuOpen(UserInterface::GUI_TYPE_MENU, gui_pers_menu);
				break;

			case SDL_SCANCODE_F11:	// MEMORY BLOCK READ/WRITE
				ActionPlayPause(false, false);
				if (key & KM_SHIFT)
					GUI->MenuOpen(UserInterface::GUI_TYPE_MENU, gui_memblock_write_menu);
				else
					GUI->MenuOpen(UserInterface::GUI_TYPE_MENU, gui_memblock_read_menu);
				break;

			case SDL_SCANCODE_F12:	// DEBUGGER
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
		BYTE result = GUI->QueryDialog("SAVE TAPE CHANGES?", true);
		if (result == GUI_QUERY_SAVE) {
			ActionTapeSave();
			return;
		}
		else if (result != GUI_QUERY_DONTSAVE) {
			GUI->MenuCloseAll();
			ActionPlayPause(!Settings->isPaused, false);
			return;
		}
	}

	if (GUI->QueryDialog("REALLY EXIT?", false) == GUI_QUERY_YES)
		isActive = false;

	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionDebugger()
{
	ActionPlayPause(false, false);
	GUI->MenuOpen(UserInterface::GUI_TYPE_DEBUGGER);
}
//---------------------------------------------------------------------------
void TEmulator::ActionTapeBrowser()
{
	ActionPlayPause(false, false);
	GUI->MenuOpen(UserInterface::GUI_TYPE_TAPEBROWSER);
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
		result = GUI->QueryDialog("SAVE TAPE CHANGES?", true);
		if (result == GUI_QUERY_SAVE) {
			GUI->MenuCloseAll();
			ActionTapeSave();
			return;
		}
	}

	if (result == GUI_QUERY_DONTSAVE) {
		TapeBrowser->SetNewTape();
		delete [] Settings->TapeBrowser->fileName;
		Settings->TapeBrowser->fileName = NULL;
		GUI->MenuCloseAll();
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
		BYTE result = GUI->QueryDialog("SAVE TAPE CHANGES?", true);
		if (result == GUI_QUERY_SAVE) {
			ActionTapeSave();
			return;
		}
		else if (result != GUI_QUERY_DONTSAVE) {
			GUI->MenuCloseAll();
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

		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			recentFile = NULL;
	}
	if (!recentFile)
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
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

	char *recentFile = Settings->TapeBrowser->fileName;
	if (recentFile) {
		char *file = ComposeFilePath(recentFile);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			recentFile = NULL;
	}
	if (!recentFile)
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
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

		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			fileName = NULL;
	}
	if (!fileName)
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
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

	char *recentFile = Settings->Snapshot->fileName;
	if (recentFile) {
		char *file = ComposeFilePath(recentFile);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			recentFile = NULL;
	}
	if (!recentFile)
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
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

	char *recentFile = Settings->Snapshot->fileName;
	if (recentFile) {
		char *file = ComposeFilePath(recentFile);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			recentFile = NULL;
	}
	if (!recentFile)
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionROMLoad()
{
	static const char *rom_filter[] = { "rom", NULL };
	char *fileName;

	ActionPlayPause(false, false);

	fileName = Settings->CurrentModel->romFile;

	GUI->fileSelector->tag = 0;
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
		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			fileName = NULL;
	}
	if (!fileName) {
		if (stat(PathAppConfig, &filestat) == 0)
			strcpy(GUI->fileSelector->path, PathAppConfig);
		else if (stat(PathResources, &filestat) == 0)
			strcpy(GUI->fileSelector->path, PathResources);
		else
			strcpy(GUI->fileSelector->path, PathApplication);
	}

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionMegaRomLoad()
{
	static const char *mrm_filter[] = { "mrm", "rmm", NULL };
	char *fileName;

	ActionPlayPause(false, false);

	fileName = Settings->CurrentModel->mrmFile;

	GUI->fileSelector->tag = 0;
	GUI->fileSelector->type = GUI_FS_BASELOAD;
	GUI->fileSelector->title = "SELECT ROM MODULE FILE (*.mrm, *.rmm)";
	GUI->fileSelector->extFilter = (char **) mrm_filter;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::ChangeMegaRomFile);

	if (fileName) {
		char *file = LocateROM(fileName);
		if (file == NULL)
			file = fileName;

		strcpy(GUI->fileSelector->path, file);
		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			fileName = NULL;
	}
	if (!fileName) {
		if (stat(PathAppConfig, &filestat) == 0)
			strcpy(GUI->fileSelector->path, PathAppConfig);
		else if (stat(PathResources, &filestat) == 0)
			strcpy(GUI->fileSelector->path, PathResources);
		else
			strcpy(GUI->fileSelector->path, PathApplication);
	}

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionRawFile(bool save)
{
	ActionPlayPause(false, false);

	GUI->fileSelector->tag = (BYTE) save;
	GUI->fileSelector->type = save ? GUI_FS_BASESAVE : GUI_FS_BASELOAD;
	GUI->fileSelector->title = "SELECT RAW FILE";
	GUI->fileSelector->extFilter = NULL;
	GUI->fileSelector->callback.disconnect_all();
	GUI->fileSelector->callback.connect(this, &TEmulator::SelectRawFile);

	char *recentFile = Settings->MemoryBlock->fileName;
	if (recentFile) {
		char *file = ComposeFilePath(recentFile);
		strcpy(GUI->fileSelector->path, file);
		delete [] file;

		if (!TestDir(GUI->fileSelector->path, (char *) "..", NULL))
			recentFile = NULL;
	}
	if (!recentFile)
		strcpy(GUI->fileSelector->path, PathApplication);

	GUI->MenuOpen(UserInterface::GUI_TYPE_FILESELECTOR);
}
//---------------------------------------------------------------------------
void TEmulator::ActionReset()
{
	cpu->DoReset();
}
//---------------------------------------------------------------------------
void TEmulator::ActionHardReset()
{
	ActionPlayPause(false, false);
	romChanged = true; // force really cold restart
	ProcessSettings(PS_MACHINE | PS_PERIPHERALS);
	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionSound(BYTE action)
{
	if (action == 0) {
		sound->SoundMute();
		Settings->Sound->mute = true;
	}
	else if (action == 1) {
		sound->SoundOn();
		Settings->Sound->mute = false;
	}
	else {
		ActionPlayPause(false, false);

		ccb_snd_volume(NULL);
		if (GUI->uiSetChanges & PS_SOUND) {
			sound->SoundOn();
			Settings->Sound->mute = false;
		}

		ActionPlayPause(!Settings->isPaused, false);
	}
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
	if (mode < 0 || mode > 5)
		newMode = DM_NORMAL;

	if (video->GetDisplayMode() != newMode) {
		video->SetDisplayMode(newMode);
		Settings->Screen->size = newMode;
		Settings->Screen->realsize = (TDisplayMode) video->GetMultiplier();
	}

	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
void TEmulator::ActionSpeedChange()
{
	ActionPlayPause(false, false);
	ccb_emu_speed(NULL);
	ActionPlayPause(!Settings->isPaused, false);
}
//---------------------------------------------------------------------------
int TEmulator::ActionMegaModulePage(bool set, BYTE page)
{
	int result = -1;
	if (!(romModuleConnected && megaModuleEnabled))
		return result;

	RomMegaModule *megaModule = static_cast<RomMegaModule *>(romModule);
	if (megaModule) {
		result = (int) megaModule->GetCurrentPage();
		if (set) {
			ActionPlayPause(false, false);
			megaModule->WriteToDevice(MEGA_MODULE_ADR, page, 0);
			ActionPlayPause(!Settings->isPaused, false);
		}
	}

	return result;
}
//---------------------------------------------------------------------------
void TEmulator::SetComputerModel(bool fromSnap, int snapRomLen, BYTE *snapRom)
{
	int fileSize;
	BYTE romSize;

	if (cpu)
		delete cpu;
	cpu = NULL;
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
			warning("Emulator", "Error reading ROM file!\n%s", romFile);
	}

	fileSize = (fileSize + KB - 1) & (~(KB - 1));
	romSize = (BYTE) (fileSize / KB);
	if (romSize < 4)
		romSize = 4;
	if (romSize > 16)
		romSize = 16;

	switch (model) {
		case CM_UNKNOWN :
			delete[] romBuff;
			return;

		case CM_V1 :    // PMD 85-1
		case CM_ALFA :  // Didaktik Alfa
		case CM_ALFA2 : // Didaktik Alfa 2
		case CM_V2 :    // PMD 85-2
			memory = new ChipMemory12(romSize);    // 48 kB RAM, x kB ROM
			break;

		case CM_V2A : // PMD 85-2A
			if (ramExpansion256k)
				memory = new ChipMemory2AEx(romSize); // 256 kB RAM, x kB ROM
			else
				memory = new ChipMemory2A(romSize);    // 64 kB RAM, x kB ROM
			break;

		case CM_V3 :  // PMD 85-3
			romSize = 8;
			if (ramExpansion256k)
				memory = new ChipMemory3Ex(romSize);  // 256 kB RAM, 8 kB ROM
			else
				memory = new ChipMemory3(romSize);     // 64 kB RAM, 8 kB ROM
			break;

		case CM_MATO :  // Mato
			romSize = 16;
			memory = new ChipMemory12(romSize);    // 48 kB RAM, 16kB ROM
			break;

		case CM_C2717 :  // CONSUL 2717
			romSize = 16;
			memory = new ChipMemoryC2717(romSize); // 64 kB RAM, 16 kB ROM
			memory->SetRemapped(true);
			break;
	}

	// Set split 8kB ROM (on 8000h and A000h)
	if (model <= CM_V2A)
		memory->SetSplit8k(romSplit8kMode);

	// CPU
	cpu = new ChipCpu8080P(memory);

	// System PIO
	systemPIO = new SystemPIO(model, memory);
	systemPIO->PrepareSample.connect(sound, &SoundDriver::PrepareSample);
	cpu->AddDevice(SYSTEM_PIO_ADR, SYSTEM_PIO_MASK, systemPIO, true);

	if (model != CM_MATO) {
		// Sound - 1kHz and 4kHz frequencies
		cpu->TCyclesListeners.connect(systemPIO, &SystemPIO::SoundService);

		// GPIO interface
		ifGpio = new IifGPIO();
		cpu->AddDevice(IIF_GPIO_ADR, IIF_GPIO_MASK, ifGpio, true);

		// Timer interface
		ifTimer = new IifTimer(model, cpu);
		cpu->AddDevice(IIF_TIMER_ADR, IIF_TIMER_MASK, ifTimer, false);
		cpu->TCyclesListeners.connect(ifTimer, &IifTimer::ITimerService);

		// Tape interface
		ifTape = new IifTape(model);
		ifTape->PrepareSample.connect(sound, &SoundDriver::PrepareSample);
		TapeBrowser->SetIfTape(ifTape);

		// set proper tape interface ports in CPU
		if (model == CM_ALFA || model == CM_ALFA2)
			cpu->AddDevice(IIF_TAPE_ADR_A, IIF_TAPE_MASK_A, ifTape, true);
		else
			cpu->AddDevice(IIF_TAPE_ADR, IIF_TAPE_MASK, ifTape, true);

		// pin tape interface signal to timer
		if (model != CM_V1 && model != CM_ALFA)
			ifTimer->Counters[((model == CM_C2717) ? 0 : 1)].OnOutChange.connect(ifTape, &IifTape::TapeClockService23);

		// pin tape interface signal to CPU
		cpu->TCyclesListeners.connect(ifTape, &IifTape::TapeClockService123);

		// registering the extended memory 256k mapper
		if (ramExpansion256k)
			cpu->AddDevice(MM256_REG_ADR, MM256_REG_MASK,
					dynamic_cast<PeripheralDevice *>(memory), true);
	}

	// disable Consul 2717 extended screen mode if was enabled
	if (model != CM_C2717)
		systemPIO->width384 = 1;

	monitorLength = romSize * KB;
	if (fileSize > 0)
		memory->PutRom(romBuff, monitorLength);
	delete[] romBuff;

	// set status bar short model descriptor
	GUI->SetComputerModel(model);
}
//---------------------------------------------------------------------------
void TEmulator::InsertRomModule(bool inserted)
{
	int i, count, sizeKB;
	DWORD romPackSizeKB, kBadr;
	TSettings::SetRomModuleFile **rl;
	BYTE *buff;

	if (romModule) {
		cpu->RemoveDevice(ROM_MODULE_ADR);
		cpu->RemoveDevice(MEGA_MODULE_ADR);
		delete romModule;
		romModule = NULL;
	}

	if (!inserted)
		return;

	romModule = new RomModule();
	cpu->AddDevice(ROM_MODULE_ADR, ROM_MODULE_MASK, romModule, true);
	romModule->RemoveRomPack();
	romPackSizeKB = ROM_PACK_SIZE_KB;

	rl = Settings->CurrentModel->romModule->files;
	count = Settings->CurrentModel->romModule->count;

	kBadr = 0;
	buff = new BYTE[romPackSizeKB * KB];
	for (i = 0; i < count && kBadr < romPackSizeKB; i++) {
		sizeKB = (rl[i]->size + KB - 1) / KB;
		if ((kBadr + sizeKB) > romPackSizeKB)
			sizeKB = romPackSizeKB - kBadr;

		memset(buff, 0xFF, sizeKB * KB);
		if (ReadFromFile(LocateROM(rl[i]->rmmFile), 0, sizeKB * KB, buff) > 0) {
			romModule->InsertRom((BYTE) kBadr, (BYTE) sizeKB, buff);
		}

		kBadr += sizeKB;
	}

	delete[] buff;
}
//---------------------------------------------------------------------------
void TEmulator::InsertRomMegaModule(bool inserted)
{
	long size;
	BYTE *buff;

	if (romModule) {
		cpu->RemoveDevice(ROM_MODULE_ADR);
		cpu->RemoveDevice(MEGA_MODULE_ADR);
		delete romModule;
		romModule = NULL;
	}

	if (!inserted)
		return;

	romModule = new RomMegaModule();
	cpu->AddDevice(ROM_MODULE_ADR, ROM_MODULE_MASK, romModule, true);
	cpu->AddDevice(MEGA_MODULE_ADR, MEGA_MODULE_MASK, romModule, false);

	char *mrmFile = LocateROM(Settings->CurrentModel->mrmFile);
	if (mrmFile == NULL)
		mrmFile = Settings->CurrentModel->mrmFile;
	size = FileSize(mrmFile);
	if (size < 0) {
		warning("Emulator", "MEGAModule ROM file not found: \"%s\"", mrmFile ? mrmFile : "");
		return;
	}
	if (size > 8 * KB * KB) {
		warning("Emulator", "MEGAModule ROM file too large: \"%s\"", mrmFile ? mrmFile : "");
		size = 8 * KB * KB;
	}
	buff = new BYTE[size];
	memset(buff, 0xFF, size);
	if (ReadFromFile(mrmFile, 0, size, buff) > 0)
		static_cast<RomMegaModule *>(romModule)->LoadRom(size, buff);
	else
		warning("Emulator", "Error reading MEGAModule ROM file: \"%s\"", mrmFile ? mrmFile : "");

	delete[] buff;
}
//---------------------------------------------------------------------------
void TEmulator::ConnectMIF85(bool init)
{
	if (init || (mif85connected != Settings->Sound->ifMIF85)) {
		if (ifTimer)
			ifTimer->EnableMIF85(false, NULL);
		if (sound)
			sound->EnableMIF85(false);

		cpu->RemoveDevice(MIF85_ADR);

		if (mif85) {
			delete mif85;
			mif85 = NULL;
		}

		mif85connected = Settings->Sound->ifMIF85;
		if (mif85connected) {
			mif85 = new Mif85();
			cpu->AddDevice(MIF85_ADR, MIF85_MASK, mif85, true);

			if (sound)
				sound->EnableMIF85(true);
			if (ifTimer)
				ifTimer->EnableMIF85(true, mif85);
		}
	}
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

			GUI->SetIconState(0);

			delete pmd32;
			pmd32 = NULL;
		}

		if (cpu && ifGpio && Settings->PMD32->connected) {
			pmd32 = new Pmd32(ifGpio);
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
	BYTE *buf  = new BYTE[SNAP_HEADER_LEN + 1];
	BYTE *src  = new BYTE[SNAP_BLOCK_LEN];
	BYTE *dest = new BYTE[SNAP_BLOCK_LEN];

	TComputerModel oldModel = model, newModel;
	int version, offset, len;

	do {
		*flag = 0xFF;
		if (ReadFromFile(fileName, 0, SNAP_HEADER_LEN, buf) != SNAP_HEADER_LEN)
			break;

		if (memcmp(buf, "PSN", 3) != 0)
			break;

		version = (int) *(buf + 3);
		offset = (int) *((WORD *) (buf + 4));

		// we have only one version now...
		if (version != 1 || offset != SNAP_HEADER_LEN)
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
			len = lenX & 0x7FFF; // bit 15 of lenX is set, if ROM is in pure format
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
			memory->ResetOn();

		Debugger->SetParams(cpu, memory, model);

		if (systemPIO) {
			systemPIO->ResetDevice(0);
			systemPIO->WriteToDevice(SYSTEM_REG_CWR, *(buf + 30), 0);
			systemPIO->WriteToDevice(SYSTEM_REG_C, *(buf + 31), 0);
			systemPIO->WriteToDevice(SYSTEM_REG_A, *(buf + 32), 0);
			systemPIO->ReadFromDevice(SYSTEM_REG_B, 0); // set for C2717 repagination
		}

		if (ifGpio)
			ifGpio->SetChipState(buf + 33);
//		if (ifIms2)
//			ifIms2->SetChipState(buf + 38);
		if (ifTimer)
			ifTimer->SetChipState(buf + 43);
		if (ifTape)
			ifTape->SetChipState(buf + 52);

		// block 0 - 3FFFh
		len = (int) *((WORD *) (buf + 22));
		if (len > 0) {
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			offset += len;
			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN)
				break;

			memory->PutMem(0x0000, dest, SNAP_BLOCK_LEN);
		}

		// block 4000h - 7FFFh
		len = (int) *((WORD *) (buf + 24));
		if (len > 0) {
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			offset += len;
			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN)
				break;

			memory->PutMem(0x4000, dest, SNAP_BLOCK_LEN);
		}

		// block 8000h - BFFFh
		if (model == CM_V2A || model == CM_V3 || model == CM_C2717) {
			len = (int) *((WORD *) (buf + 26));
			if (len > 0) {
				if (ReadFromFile(fileName, offset, len, src) != len)
					break;

				offset += len;
				len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
				if (len < 0 || len != SNAP_BLOCK_LEN)
					break;

				memory->PutMem(0x8000, dest, SNAP_BLOCK_LEN);
			}
		}

		// block C000h - FFFFh
		len = (int) *((WORD *) (buf + 28));
		if (len > 0) {
			if (ReadFromFile(fileName, offset, len, src) != len)
				break;

			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN)
				break;

			memory->PutMem(0xC000, dest, SNAP_BLOCK_LEN);
		}

		*flag = 0;
	} while (false);

	if (*flag == 0xFF)
		GUI->MessageBox("INVALID SNAPHOT FORMAT!");
	else if (*flag == 2) {
		GUI->MessageBox("CORRUPTED SNAPSHOT!");

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
void TEmulator::PrepareSnapshot(char *fileName, BYTE *flag)
{
	BYTE *buf  = new BYTE[SNAP_HEADER_LEN + 1];
	BYTE *src  = new BYTE[SNAP_BLOCK_LEN];
	BYTE *dest = new BYTE[SNAP_BLOCK_LEN];
	int len, offset = SNAP_HEADER_LEN;

	memset(buf, 0, SNAP_HEADER_LEN);
	memcpy(buf, "PSN", 3);
	*(buf + 3) = 1; // we have only one version now...
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
					memory->GetMem(src, 0x8000, monitorLength);
					break;
				case CM_V2A:
				case CM_C2717:
					memory->GetMem(src, 0x8000, monitorLength);
					break;
				case CM_V3:
					memory->GetMem(src, 0xE000, monitorLength);
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
		memory->GetMem(src, 0, SNAP_BLOCK_LEN);
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
		memory->GetMem(src, 16384, SNAP_BLOCK_LEN);
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
			memory->GetMem(src, 32768, SNAP_BLOCK_LEN);
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
		memory->GetMem(src, 49152, SNAP_BLOCK_LEN);
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
		GUI->MessageBox("CAN'T OPEN FILE FOR WRITING!");
	else if (*flag == 1) {
		GUI->MessageBox("ERROR WRITING FILE...\nSNAPSHOT WILL BE CORRUPTED!");
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
		GUI->MessageBox("FATAL ERROR!\nOR CAN'T OPEN FILE!");
	else if (*flag == 1) {
		GUI->MessageBox("CORRUPTED TAPE FORMAT!");
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
		GUI->MessageBox("FATAL ERROR!\nINVALID NAME OR EXTENSION,\nOR CAN'T OPEN FILE FOR WRITING!");
	else if (*flag == 1) {
		GUI->MessageBox("ERROR WRITING FILE...\nTAPE WILL BE CORRUPTED!");
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

	delete [] Settings->CurrentModel->romFile;
	Settings->CurrentModel->romFile = new char[strlen(ptr) + 1];
	strcpy(Settings->CurrentModel->romFile, ptr);
	GUI->uiSetChanges |= PS_MACHINE;

	romChanged = true;
	*flag = 1;
}
//---------------------------------------------------------------------------
void TEmulator::ChangeMegaRomFile(char *fileName, BYTE *flag)
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

	delete [] Settings->CurrentModel->mrmFile;
	Settings->CurrentModel->mrmFile = new char[strlen(ptr) + 1];
	strcpy(Settings->CurrentModel->mrmFile, ptr);
	GUI->uiSetChanges |= PS_MACHINE;

	romChanged = true;
	*flag = 1;
}
//---------------------------------------------------------------------------
void TEmulator::SelectRawFile(char *fileName, BYTE *flag)
{
	long int length = Settings->MemoryBlock->length;
	bool save = (bool) *flag;
	FILE *f = NULL;

	*flag = 0xFF;
	if (save)
		*flag = 1;
	else if ((f = fopen(fileName, "rb"))) {
		if (fseek(f, 0, SEEK_END) == 0) {
			length = ftell(f);
			if (length > 0 && length < 65536)
				*flag = 1;
		}

		fclose(f);
		f = NULL;
	}

	if (*flag == 0xFF)
		GUI->MessageBox("FATAL ERROR!\nINVALID LENGTH OR CAN'T OPEN FILE!");
	else {
		if (Settings->MemoryBlock->fileName)
			delete [] Settings->MemoryBlock->fileName;

		Settings->MemoryBlock->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->MemoryBlock->fileName, fileName);

		Settings->MemoryBlock->length = length;
	}
}
//---------------------------------------------------------------------------
bool TEmulator::ProcessRawFile(bool save)
{
	int length = Settings->MemoryBlock->length,
	    start = Settings->MemoryBlock->start;

	if (start < 0 || start > 65535)
		return false;
	if (length <= 0 || length > 65535)
		return false;
	if ((start + length) > 65535)
		length = 65536 - start;

	char *fn = ComposeFilePath(Settings->MemoryBlock->fileName);
	if (!fn)
		return false;

	BYTE *buff = NULL;
	bool ret = true;

	if (!save) {
		buff = new BYTE[length];
		length = ReadFromFile(fn, 0, length, buff);

		if (length > 0) {
			bool oldState = false;

			if (model == CM_C2717) {
				oldState = memory->IsRemapped();
				memory->SetRemapped(Settings->MemoryBlock->remapping);
			}
			else {
				oldState = memory->IsInReset();
				if (oldState)
					memory->ResetOff();
			}

			for (int i = 0; i < length; i++)
				memory->WriteByte(start + i, *(buff + i));

			if (model == CM_C2717)
				memory->SetRemapped(oldState);
			else if (oldState)
				memory->ResetOn();
		}
		else
			ret = false;
	}
	else {
		int oldPage = -1;
		bool oldState = false;
		buff = new BYTE[length];

		if (model == CM_V2A || model == CM_V3 || model == CM_C2717) {
			oldPage = memory->GetPage();
			memory->SetPage((BYTE) Settings->MemoryBlock->rom);

			if (model == CM_C2717) {
				oldState = memory->IsRemapped();
				memory->SetRemapped(Settings->MemoryBlock->remapping);
			}
		}

		for (int i = 0; i < length; i++)
			*(buff + i) = memory->ReadByte(start + i);

		if (model == CM_V2A || model == CM_V3 || model == CM_C2717) {
			memory->SetPage(oldPage);

			if (model == CM_C2717)
				memory->SetRemapped(oldState);
		}

		if (WriteToFile(fn, 0, length, buff, true) < 0)
			ret = false;
	}

	if (buff)
		delete [] buff;
	delete [] fn;

	return ret;
}
//---------------------------------------------------------------------------
