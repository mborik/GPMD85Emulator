/*	Settings.h: Class for reading, handling and saveing settings
	Copyright (c) 2011 Martin Borik <mborik@users.sourceforge.net>

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
#include <libxml/tree.h>
#include <libxml/parser.h>
//-----------------------------------------------------------------------------
#define CONFIGURATION_VERSION "1.1"
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
			bool monitoring;
			bool flash;
			TAutoStopType autoStop;
			char *fileName;
		};
		struct SetScreen {
			BYTE border;
			TDisplayMode size;
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
			char outType[4];
			bool hwBuffer;
			BYTE volume;
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
			DWORD ctrlLeft;
			DWORD ctrlRight;
			DWORD ctrlUp;
			DWORD ctrlDown;
			DWORD ctrlFire;
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

	private:
		xmlDocPtr  xmlDoc;
		xmlNodePtr xmlRoot;

		xmlNodePtr cfgGetChildNode(xmlNodePtr node, const char * name);
		int cfgCountChildNodes(xmlNodePtr node);
		char *cfgGetAttributeToken(xmlNodePtr node, const char *attrName);
		int cfgGetAttributeIntValue(xmlNodePtr node, const char *attrName, int dflt);
		bool cfgGetAttributeBoolValue(xmlNodePtr node, const char *attrName, bool dflt);
		TColor cfgGetAttributeColorValue(xmlNodePtr node, const char *attrName, TColor dflt);
		bool cfgIsAttributeToken(xmlNodePtr node, const char *attrName, const char *token);
		char *cfgConvertXmlString(xmlChar *s);

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

		TSettings();
		virtual ~TSettings();

		SetRomPackage *findROMmodule(char *name);
};
//-----------------------------------------------------------------------------
#endif
