/*	Settings.h: Class for reading, handling and saveing settings
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
#ifndef SETTINGS_H_
#define SETTINGS_H_
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
#define CONFIGURATION_VERSION "7"
//-----------------------------------------------------------------------------
class TSettings
{
	public:
		struct SetRomModuleFile {
			char *rmmFile;
			DWORD size;
			BYTE err;
		};
		struct SetRomPackage {
			char *name;
			BYTE count;
			SetRomModuleFile **files;
		};
		struct SetComputerModel {
			TComputerModel type;
			char *romFile;
			bool compatibilityMode;
			bool romModuleInserted;
			bool romSplit8kMode;
			bool ramExpansion256k;
			bool matoAllRAM64k;
			TTapeIfType tapeIfType;
			SetRomPackage *romModule;
			bool megaModuleEnabled;
			char *mrmFile;
		};
		struct SetSnapshot {
			bool saveCompressed;
			bool saveWithMonitor;
			bool dontRunOnLoad;
			char *fileName;
		};
		struct SetTapeBrowser {
			bool hex;
			bool flash;
			bool monitoring;
			TAutoStopType autoStop;
			char *fileName;
		};
		struct SetUserInterface {
			SDL_Point position;
			SDL_Point windowSize;
			int fontScale;
			bool dialogTapeBrowserOpened;
			bool dialogDiskImagesOpened;
			bool dialogMemEditOpened;
			bool dialogDebugOpened;
			bool memEditAscii;
			int memEditColumns;
		};
		struct SetScreen {
			int border;
			TDisplayMode size;
			TDisplayMode realsize;
			THalfPassMode halfPass;
			bool lcdMode;
			TColorProfile colorProfile;
			TColorPalette colorPalette;
			TColor attr00;
			TColor attr01;
			TColor attr10;
			TColor attr11;
			bool videoInterrupt;
		};
		struct SetSound {
			int  volume;
			bool mute;
			bool ifMIF85;
		};
		struct SetKeyboard {
			bool changeZY;
			bool useNumpad;
			bool useMatoCtrl;
		};
		struct SetJoystickGPIO {
			bool connected;
			TJoyType type;
			char *guid;
			int ctrlLeft;
			int ctrlRight;
			int ctrlUp;
			int ctrlDown;
			int ctrlFire;
			int sensitivity;
			int axis;
		};
		struct SetJoystick {
			SetJoystickGPIO *GPIO0;
			SetJoystickGPIO *GPIO1;
		};
		struct SetMouse {
			TMouseType type;
			bool hideCursor;
		};
		struct SetPMD32Drive {
			char *image;
			bool writeProtect;
		};
		struct SetStoragePMD32 {
			bool connected;
			bool extraCommands;
			SetPMD32Drive driveA;
			SetPMD32Drive driveB;
			SetPMD32Drive driveC;
			SetPMD32Drive driveD;
			char *sdRoot;
		};
		struct SetDebugger {
			bool hex;
			bool z80;
			TDebugListType listType;
			TDebugListSource listSource;
			int listOffset;
		};
		struct SetMemoryBlock {
			int start;
			int length;
			bool autorun;
			int autorunAddr;
			int ex256pg;
			bool remapping;
			bool rom;
			bool hex;
			char *fileName;
		};

		enum cfgIniLineType {
			LT_EMPTY, LT_COMMENT, LT_DELIMITER, LT_SECTION, LT_ITEM, LT_LIST,
			LT_STRING, LT_QUOTED, LT_NUMBER, LT_BOOL, LT_RADIX, LT_AUTOSTOP,
			LT_SCR_SIZE, LT_SCR_HP, LT_SCR_COL, LT_SCR_PAL, LT_COLOR, LT_ROM,
			LT_MOUSE, LT_JOY, LT_TAPEIF, LT_NOTATION, LT_DEBUGLIST, LT_DEBUGSRC
		};
		typedef struct cfgIniLine {
			cfgIniLineType type;
			char *key;
			char *value;
			void *ptr;
			cfgIniLine *prev;
			cfgIniLine *next;
		} cfgIniLine;
		cfgIniLine *cfgRoot;

	private:
		void cfgReadFile(char *fileName);
		cfgIniLine *cfgFindSection(cfgIniLine *node, const char *name);
		int cfgCountChildAttributes(cfgIniLine *node);
		char *cfgGetStringValue(cfgIniLine *node, const char *key, char **target = NULL);
		int cfgGetIntValue(cfgIniLine *node, const char *key, int dflt, int *target = NULL);
		bool cfgGetBoolValue(cfgIniLine *node, const char *key, bool dflt, bool *target = NULL);
		TColor cfgGetColorValue(cfgIniLine *node, const char *key, TColor dflt, TColor *target = NULL);
		cfgIniLine *cfgGetLine(cfgIniLine *node, const char *key);
		bool cfgHasKeyValue(cfgIniLine *node, const char *key, const char *value);
		void cfgInsertNewLine(cfgIniLine *node, const char *key, cfgIniLineType type, void *ptr);

	public:
		bool isPaused;
		bool pauseOnFocusLost;
		bool showHiddenFiles;
		bool autosaveSettings;
		bool fixedSettings;

		double emulationSpeed;

		BYTE modelsCount;
		BYTE romPackagesCount;

		SetUserInterface *GUI;
		SetComputerModel **AllModels;
		SetComputerModel *CurrentModel;
		SetRomPackage **RomPackages;
		SetScreen *Screen;
		SetSound *Sound;
		SetKeyboard *Keyboard;
		SetSnapshot *Snapshot;
		SetTapeBrowser *TapeBrowser;
		SetDebugger *Debugger;
		SetMemoryBlock *MemoryBlock;
		SetJoystick *Joystick;
		SetMouse *Mouse;
		SetStoragePMD32 *PMD32;

		TSettings(bool userCfg);
		virtual ~TSettings();
		void storeSettings();

		SetRomPackage *findROMmodule(char *name);
		SetRomModuleFile *checkRMMfile(char *name);
};
//-----------------------------------------------------------------------------
#endif
