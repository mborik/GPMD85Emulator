/*	Settings.h: Class for reading, handling and saveing settings
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
#ifndef SETTINGS_H_
#define SETTINGS_H_
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
#define CONFIGURATION_VERSION "2"
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
			SetRomPackage *romModule;
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
		};
		struct SetSound {
			int  volume;
			bool mute;
			bool ifMusica;
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
			int pov;
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
			char *romFile;
			SetPMD32Drive driveA;
			SetPMD32Drive driveB;
			SetPMD32Drive driveC;
			SetPMD32Drive driveD;
			char *sdRoot;
		};
		struct SetStorageRAOM {
			TRaomType type;
			bool inserted;
			SetRomPackage *module;
			char *file;
		};
		struct SetDebugger {
			bool hex;
			bool z80;
			TDebugListType listType;
			TDebugListSource listSource;
			int listOffset;
		};

		enum cfgIniLineType {
			LT_EMPTY, LT_COMMENT, LT_DELIMITER, LT_SECTION, LT_ITEM, LT_LIST,
			LT_STRING, LT_QUOTED, LT_NUMBER, LT_BOOL, LT_RADIX, LT_AUTOSTOP,
			LT_SCR_SIZE, LT_SCR_HP, LT_SCR_COL, LT_SCR_PAL, LT_COLOR,
			LT_NOTATION, LT_DEBUGLIST, LT_DEBUGSRC, LT_MOUSE, LT_JOY, LT_RAOM
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

		BYTE modelsCount;
		BYTE romPackagesCount;

		SetComputerModel **AllModels;
		SetComputerModel *CurrentModel;
		SetRomPackage **RomPackages;
		SetSnapshot *Snapshot;
		SetTapeBrowser *TapeBrowser;
		SetScreen *Screen;
		SetSound *Sound;
		SetKeyboard *Keyboard;
		SetJoystick *Joystick;
		SetMouse *Mouse;
		SetStoragePMD32 *PMD32;
		SetStorageRAOM *RaomModule;
		SetDebugger *Debugger;

		TSettings();
		virtual ~TSettings();
		void storeSettings();

		SetRomPackage *findROMmodule(char *name);
		SetRomModuleFile *checkRMMfile(char *name);
};
//-----------------------------------------------------------------------------
#endif
