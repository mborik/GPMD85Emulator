/*	Settings.cpp: Class for reading, handling and saveing settings
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
#include "CommonUtils.h"
#include "Settings.h"
//-----------------------------------------------------------------------------
TSettings::TSettings()
{
	int i, j, k;
	char *buf, *s;

	debug("[Settings] Configuration parser initialization");

	if ((buf = LocateResource("default.conf", true)) == NULL)
		error("[Settings] Configuration file not found!");

	cfgReadFile(buf);
	if (cfgRoot == NULL)
		error("[Settings] Couldn't parse configuration file: %s", buf);

	debug("[Settings] Configuration XML parsed, reading elements");

	if (strcmp(cfgRoot->value, "GNU/GPL PMD 85 Emulator Configuration File") != 0)
		warning("[Settings] Invalid header of configuration file!");

	if (!cfgHasKeyValue(cfgRoot->next, "config-version", CONFIGURATION_VERSION))
		warning("[Settings] Incompatible configuration file version (required %s)!", CONFIGURATION_VERSION);

	cfgIniLine *m = NULL, *n = cfgFindSection(cfgRoot, "General");

	pauseOnFocusLost = cfgGetBoolValue(n, "pause-on-focus-lost", false, &pauseOnFocusLost);
	showHiddenFiles = cfgGetBoolValue(n, "show-hidden-files", false, &showHiddenFiles);
	autosaveSettings = cfgGetBoolValue(n, "autosave-setting", false, &autosaveSettings);
	buf = cfgGetStringValue(n, "current-model");

	isPaused = false;

	n = cfgFindSection(cfgRoot, "RomModulePackages");
	romPackagesCount = cfgCountChildAttributes(n);
	RomPackages = new SetRomPackage *[romPackagesCount];

	n = n->next;
	for (i = 0; (n != NULL && n->type != LT_SECTION) || i < romPackagesCount; n = n->next) {
		if (n->type == LT_LIST) {
			k = 1;
			s = n->value;
			strccnt(s, '|', k);

			RomPackages[i] = new SetRomPackage;
			RomPackages[i]->name = n->key;
			RomPackages[i]->count = k;
			RomPackages[i]->files = new SetRomModuleFile *[k];

			j = 0;
			checkRMMfile(NULL);
			s = strtok(n->value, "|");
			while (s != NULL) {
				while (*s == ' ' || *s == '\t')
					s++;
				while (*(s + strlen(s) - 1) == ' ' || *(s + strlen(s) - 1) == '\t')
					*(s + strlen(s) - 1) = '\0';

				RomPackages[i]->files[j] = checkRMMfile(s);
				s = strtok(NULL, "|");
				j++;
			}

			i++;
		}
	}

	k = 8;
	n = cfgRoot;
	CurrentModel = NULL;
	SetComputerModel *model = NULL;
	AllModels = new SetComputerModel *[k];

	for (modelsCount = 0; n != NULL; n = n->next) {
		if (n->type != LT_SECTION)
			continue;
		if (strncmp(n->key, "Model-", 6) != 0)
			continue;

		model = new SetComputerModel;

		if (strcmp(n->key, "Model-1") == 0)
			model->type = CM_V1;
		else if (strcmp(n->key, "Model-2") == 0)
			model->type = CM_V2;
		else if (strcmp(n->key, "Model-2A") == 0)
			model->type = CM_V2A;
		else if (strcmp(n->key, "Model-3") == 0)
			model->type = CM_V3;
		else if (strcmp(n->key, "Model-Mato") == 0)
			model->type = CM_MATO;
		else if (strcmp(n->key, "Model-Alfa") == 0)
			model->type = CM_ALFA;
		else if (strcmp(n->key, "Model-Alfa2") == 0)
			model->type = CM_ALFA2;
		else if (strcmp(n->key, "Model-C2717") == 0)
			model->type = CM_C2717;
		else {
			warning("[Settings] Unknown model '%s' definition!", n->key);
			delete model;
			continue;
		}

		model->romFile = cfgGetStringValue(n, "rom", &(model->romFile));
		if (model->romFile == NULL) {
			warning("[Settings] ROM file not defined for %s!", n->key);
			delete model;
			continue;
		}

		model->compatibilityMode = cfgGetBoolValue(n, "compmode", false, &(model->compatibilityMode));
		model->romModuleInserted = cfgGetBoolValue(n, "rmm-inserted", false, &(model->romModuleInserted));

		s = cfgGetStringValue(n, "rmm-name");
		if ((model->romModule = findROMmodule(s)) == NULL)
			model->romModuleInserted = false;

		if (s)
			delete [] s;
		s = NULL;

		if (strcmp(n->key, buf) == 0)
			CurrentModel = model;

		AllModels[modelsCount] = model;
		modelsCount++;

		if (modelsCount > k) {
			warning("[Settings] Too much computer models defined!");
			break;
		}
	}

	if (!modelsCount)
		error("[Settings] No computer models defined!");

	if (CurrentModel == NULL) {
		warning("[Settings] Current model '%s' not found!", buf);
		CurrentModel = AllModels[0];
	}

	if (buf)
		delete [] buf;
	buf = NULL;

	n = cfgFindSection(cfgRoot, "Snapshot");

	Snapshot = new SetSnapshot;
	Snapshot->saveWithMonitor = cfgGetBoolValue(n, "save-snapshot-with-monitor", false, &(Snapshot->saveWithMonitor));
	Snapshot->saveCompressed = cfgGetBoolValue(n, "save-snapshot-compressed", true, &(Snapshot->saveCompressed));
	Snapshot->dontRunOnLoad = cfgGetBoolValue(n, "dont-run-snapshot-on-load", false, &(Snapshot->dontRunOnLoad));
	Snapshot->fileName = cfgGetStringValue(n, "recent-file", &(Snapshot->fileName));

	n = cfgFindSection(cfgRoot, "TapeBrowser");

	TapeBrowser = new SetTapeBrowser;
	TapeBrowser->monitoring = cfgGetBoolValue(n, "monitoring", false, &(TapeBrowser->monitoring));
	TapeBrowser->flash = cfgGetBoolValue(n, "flash", false, &(TapeBrowser->flash));
	TapeBrowser->fileName = cfgGetStringValue(n, "recent-file", &(TapeBrowser->fileName));

	TapeBrowser->autoStop = AS_NEXTHEAD;
	if ((m = cfgGetLine(n, "auto-stop")) != NULL) {
		if (strcmp(m->value, "next-head") == 0)
			TapeBrowser->autoStop = AS_NEXTHEAD;
		else if (strcmp(m->value, "cursor") == 0)
			TapeBrowser->autoStop = AS_CURSOR;
		else if (strcmp(m->value, "off") == 0)
			TapeBrowser->autoStop = AS_OFF;

		// TODO: extend line types to handle all enumerators in config!
		m->type = LT_ENUM;
		m->ptr = (void *) &(TapeBrowser->autoStop);
	}
	else
		cfgInsertNewLine(n->next, "auto-stop", LT_ENUM, (void *) &(TapeBrowser->autoStop));

	n = cfgFindSection(cfgRoot, "Screen");

	Screen = new SetScreen;
	cfgGetIntValue(n, "border", 0, &(Screen->border));

	Screen->size = DM_NORMAL;
	if ((m = cfgGetLine(n, "size")) != NULL) {
		if (strcmp(m->value, "double") == 0)
			Screen->size = DM_DOUBLESIZE;
		else if (strcmp(m->value, "triple") == 0)
			Screen->size = DM_TRIPLESIZE;
		else if (strcmp(m->value, "quadruple") == 0)
			Screen->size = DM_QUADRUPLESIZE;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Screen->size);
	}
	else
		cfgInsertNewLine(n->next, "size", LT_ENUM, (void *) &(Screen->size));

	Screen->lcdMode = false;
	Screen->halfPass = HP_OFF;
	if ((m = cfgGetLine(n, "half-pass")) != NULL) {
		if (strcmp(m->value, "lcd") == 0)
			Screen->lcdMode = true;
		else if (strcmp(m->value, "b75") == 0)
			Screen->halfPass = HP_75;
		else if (strcmp(m->value, "b50") == 0)
			Screen->halfPass = HP_50;
		else if (strcmp(m->value, "b25") == 0)
			Screen->halfPass = HP_25;
		else if (strcmp(m->value, "b0") == 0)
			Screen->halfPass = HP_0;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Screen->halfPass);
	}
	else
		cfgInsertNewLine(n->next, "half-pass", LT_ENUM, (void *) &(Screen->halfPass));

	Screen->colorProfile = CP_STANDARD;
	if ((m = cfgGetLine(n, "color-profile")) != NULL) {
		if (strcmp(m->value, "mono") == 0)
			Screen->colorProfile = CP_MONO;
		else if (strcmp(m->value, "color") == 0)
			Screen->colorProfile = CP_COLOR;
		else if (strcmp(m->value, "multicolor") == 0)
			Screen->colorProfile = CP_MULTICOLOR;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Screen->colorProfile);
	}
	else
		cfgInsertNewLine(n->next, "color-profile", LT_ENUM, (void *) &(Screen->colorProfile));

	Screen->colorPalette = CL_RGB;
	if ((m = cfgGetLine(n, "color-pallette")) != NULL) {
		if (strcmp(m->value, "video") == 0)
			Screen->colorPalette = CL_VIDEO;
		else if (strcmp(m->value, "defined") == 0)
			Screen->colorPalette = CL_DEFINED;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Screen->colorPalette);
	}
	else
		cfgInsertNewLine(n->next, "color-pallette", LT_ENUM, (void *) &(Screen->colorPalette));

	Screen->attr00 = cfgGetColorValue(n, "attr00", WHITE, &(Screen->attr00));
	Screen->attr01 = cfgGetColorValue(n, "attr01", GREEN, &(Screen->attr01));
	Screen->attr10 = cfgGetColorValue(n, "attr10", AQUA, &(Screen->attr10));
	Screen->attr11 = cfgGetColorValue(n, "attr11", YELLOW, &(Screen->attr11));

	n = cfgFindSection(cfgRoot, "Sound");

	Sound = new SetSound;
	Sound->mute = cfgGetBoolValue(n, "mute", false, &(Sound->mute));
	Sound->volume = cfgGetIntValue(n, "volume", 64, &(Sound->volume));
	Sound->ifMusica = cfgGetBoolValue(n, "if-musica", false, &(Sound->ifMusica));

	n = cfgFindSection(cfgRoot, "Keyboard");

	Keyboard = new SetKeyboard;
	Keyboard->changeZY = cfgGetBoolValue(n, "change-zy", false, &(Keyboard->changeZY));
	Keyboard->useNumpad = cfgGetBoolValue(n, "use-numpad", false, &(Keyboard->useNumpad));
	Keyboard->useMatoCtrl = cfgGetBoolValue(n, "mato-ctrl", false, &(Keyboard->useMatoCtrl));

	n = cfgFindSection(cfgRoot, "Mouse");

	Mouse = new SetMouse;
	Mouse->hideCursor = cfgGetBoolValue(n, "hide-cursor", true, &(Mouse->hideCursor));

	Mouse->type = MT_NONE;
	if ((m = cfgGetLine(n, "type")) != NULL) {
		if (strcmp(m->value, "m602") == 0)
			Mouse->type = MT_M602;
		else if (strcmp(m->value, "poly8") == 0)
			Mouse->type = MT_POLY8;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Mouse->type);
	}
	else
		cfgInsertNewLine(n->next, "type", LT_ENUM, (void *) &(Mouse->type));

	n = cfgFindSection(cfgRoot, "Joystick-GPIO0");

	Joystick = new SetJoystick;
	Joystick->GPIO0 = new SetJoystickGPIO;
	Joystick->GPIO1 = new SetJoystickGPIO;

	Joystick->GPIO0->connected = cfgGetBoolValue(n, "connected", false, &(Joystick->GPIO0->connected));
	Joystick->GPIO0->guid = cfgGetStringValue(n, "guid", &(Joystick->GPIO0->guid));
	Joystick->GPIO0->ctrlLeft = cfgGetIntValue(n, "left", SDLK_UNKNOWN, &(Joystick->GPIO0->ctrlLeft));
	Joystick->GPIO0->ctrlRight = cfgGetIntValue(n, "right", SDLK_UNKNOWN, &(Joystick->GPIO0->ctrlRight));
	Joystick->GPIO0->ctrlUp = cfgGetIntValue(n, "up", SDLK_UNKNOWN, &(Joystick->GPIO0->ctrlUp));
	Joystick->GPIO0->ctrlDown = cfgGetIntValue(n, "down", SDLK_UNKNOWN, &(Joystick->GPIO0->ctrlDown));
	Joystick->GPIO0->ctrlFire = cfgGetIntValue(n, "fire", SDLK_UNKNOWN, &(Joystick->GPIO0->ctrlFire));
	Joystick->GPIO0->sensitivity = cfgGetIntValue(n, "sensitivity", -1, &(Joystick->GPIO0->sensitivity));
	Joystick->GPIO0->pov = cfgGetIntValue(n, "pov", -1, &(Joystick->GPIO0->pov));

	Joystick->GPIO0->type = JT_NONE;
	if ((m = cfgGetLine(n, "type")) != NULL) {
		if (strcmp(m->value, "keys") == 0)
			Joystick->GPIO0->type = JT_KEYS;
		else if (strcmp(m->value, "axes") == 0)
			Joystick->GPIO0->type = JT_AXES;
		else if (strcmp(m->value, "pov") == 0)
			Joystick->GPIO0->type = JT_POV;
		else if (strcmp(m->value, "buttons") == 0)
			Joystick->GPIO0->type = JT_BUTTONS;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Joystick->GPIO0->type);
	}
	else
		cfgInsertNewLine(n->next, "type", LT_ENUM, (void *) &(Joystick->GPIO0->type));

	n = cfgFindSection(cfgRoot, "Joystick-GPIO1");

	Joystick->GPIO1->connected = cfgGetBoolValue(n, "connected", false, &(Joystick->GPIO1->connected));
	Joystick->GPIO1->guid = cfgGetStringValue(n, "guid", &(Joystick->GPIO1->guid));
	Joystick->GPIO1->ctrlLeft = cfgGetIntValue(n, "left", SDLK_UNKNOWN, &(Joystick->GPIO1->ctrlLeft));
	Joystick->GPIO1->ctrlRight = cfgGetIntValue(n, "right", SDLK_UNKNOWN, &(Joystick->GPIO1->ctrlRight));
	Joystick->GPIO1->ctrlUp = cfgGetIntValue(n, "up", SDLK_UNKNOWN, &(Joystick->GPIO1->ctrlUp));
	Joystick->GPIO1->ctrlDown = cfgGetIntValue(n, "down", SDLK_UNKNOWN, &(Joystick->GPIO1->ctrlDown));
	Joystick->GPIO1->ctrlFire = cfgGetIntValue(n, "fire", SDLK_UNKNOWN, &(Joystick->GPIO1->ctrlFire));
	Joystick->GPIO1->sensitivity = cfgGetIntValue(n, "sensitivity", -1, &(Joystick->GPIO1->sensitivity));
	Joystick->GPIO1->pov = cfgGetIntValue(n, "pov", -1, &(Joystick->GPIO1->pov));

	Joystick->GPIO1->type = JT_NONE;
	if ((m = cfgGetLine(n, "type")) != NULL) {
		if (strcmp(m->value, "keys") == 0)
			Joystick->GPIO1->type = JT_KEYS;
		else if (strcmp(m->value, "axes") == 0)
			Joystick->GPIO1->type = JT_AXES;
		else if (strcmp(m->value, "pov") == 0)
			Joystick->GPIO1->type = JT_POV;
		else if (strcmp(m->value, "buttons") == 0)
			Joystick->GPIO1->type = JT_BUTTONS;

		m->type = LT_ENUM;
		m->ptr = (void *) &(Joystick->GPIO1->type);
	}
	else
		cfgInsertNewLine(n->next, "type", LT_ENUM, (void *) &(Joystick->GPIO1->type));

	n = cfgFindSection(cfgRoot, "PMD-32");

	PMD32 = new SetStoragePMD32;
	PMD32->connected = cfgGetBoolValue(n, "connected", false, &(PMD32->connected));
	PMD32->extraCommands = cfgGetBoolValue(n, "extra-commands", false, &(PMD32->extraCommands));
	PMD32->romFile = cfgGetStringValue(n, "rom", &(PMD32->romFile));
	PMD32->sdRoot = cfgGetStringValue(n, "sd-root", &(PMD32->sdRoot));
	PMD32->driveA.image = cfgGetStringValue(n, "drive-a-file", &(PMD32->driveA.image));
	PMD32->driveA.writeProtect = cfgGetBoolValue(n, "drive-a-wp", false, &(PMD32->driveA.writeProtect));
	PMD32->driveB.image = cfgGetStringValue(n, "drive-b-file", &(PMD32->driveB.image));
	PMD32->driveB.writeProtect = cfgGetBoolValue(n, "drive-b-wp", false, &(PMD32->driveB.writeProtect));
	PMD32->driveC.image = cfgGetStringValue(n, "drive-c-file", &(PMD32->driveC.image));
	PMD32->driveC.writeProtect = cfgGetBoolValue(n, "drive-c-wp", false, &(PMD32->driveC.writeProtect));
	PMD32->driveD.image = cfgGetStringValue(n, "drive-d-file", &(PMD32->driveD.image));
	PMD32->driveD.writeProtect = cfgGetBoolValue(n, "drive-d-wp", false, &(PMD32->driveD.writeProtect));

	n = cfgFindSection(cfgRoot, "RaomModule");

	RaomModule = new SetStorageRAOM;
	RaomModule->inserted = cfgGetBoolValue(n, "inserted", false, &(RaomModule->inserted));
	RaomModule->file = cfgGetStringValue(n, "recent-file", &(RaomModule->file));
	RaomModule->type = RT_CHTF;
	if ((m = cfgGetLine(n, "hw-version")) != NULL) {
		if (strcmp(m->value, "chtf") == 0)
			RaomModule->type = RT_CHTF;
		else if (strcmp(m->value, "kuvi") == 0)
			RaomModule->type = RT_KUVI;

		m->type = LT_ENUM;
		m->ptr = (void *) &(RaomModule->type);
	}
	else
		cfgInsertNewLine(n->next, "hw-version", LT_ENUM, (void *) &(RaomModule->type));

	s = cfgGetStringValue(n, "rmm-name");
	if ((RaomModule->module = findROMmodule(s)) == NULL)
		RaomModule->inserted = false;

	if (s)
		delete [] s;
	s = NULL;

	debug("[Settings] Configuration loaded");
}
//-----------------------------------------------------------------------------
TSettings::~TSettings()
{
	int i, j;

	debug("[Settings] Freeing all structures");

	if (AllModels) {
		for (i = 0; i < modelsCount; i++) {
			if (AllModels[i] == NULL)
				continue;

			if (AllModels[i]->romFile) {
				delete [] AllModels[i]->romFile;
				AllModels[i]->romFile = NULL;
			}

			delete AllModels[i];
			AllModels[i] = NULL;
		}

		delete [] AllModels;
		AllModels = NULL;
		CurrentModel = NULL;
	}

	if (RomPackages) {
		for (i = 0; i < romPackagesCount; i++) {
			if (RomPackages[i] == NULL)
				continue;

			if (RomPackages[i]->name) {
				delete [] RomPackages[i]->name;
				RomPackages[i]->name = NULL;
			}

			if (RomPackages[i]->files) {
				for (j = 0; j < RomPackages[i]->count; j++) {
					delete [] RomPackages[i]->files[j]->rmmFile;
					RomPackages[i]->files[j]->rmmFile = NULL;
				}

				delete [] RomPackages[i]->files;
			}

			delete RomPackages[i];
			RomPackages[i] = NULL;
		}

		delete [] RomPackages;
		RomPackages = NULL;
	}

	if (Snapshot) {
		if (Snapshot->fileName) {
			delete [] Snapshot->fileName;
			Snapshot->fileName = NULL;
		}

		delete Snapshot;
		Snapshot = NULL;
	}

	if (Joystick) {
		if (Joystick->GPIO0) {
			if (Joystick->GPIO0->guid) {
				delete [] Joystick->GPIO0->guid;
				Joystick->GPIO0->guid = NULL;
			}

			delete Joystick->GPIO0;
			Joystick->GPIO0 = NULL;
		}

		if (Joystick->GPIO1) {
			if (Joystick->GPIO1->guid) {
				delete [] Joystick->GPIO1->guid;
				Joystick->GPIO1->guid = NULL;
			}

			delete Joystick->GPIO1;
			Joystick->GPIO1 = NULL;
		}

		delete Joystick;
		Joystick = NULL;
	}

	if (Screen)
		delete Screen;
	Screen = NULL;

	if (Sound)
		delete Sound;
	Sound = NULL;

	if (Keyboard)
		delete Keyboard;
	Keyboard = NULL;

	if (Mouse)
		delete Mouse;
	Mouse = NULL;

	if (PMD32) {
		if (PMD32->romFile)
			delete [] PMD32->romFile;
		PMD32->romFile = NULL;

		if (PMD32->sdRoot)
			delete [] PMD32->sdRoot;
		PMD32->sdRoot = NULL;

		if (PMD32->driveA.image)
			delete [] PMD32->driveA.image;
		PMD32->driveA.image = NULL;

		if (PMD32->driveB.image)
			delete [] PMD32->driveB.image;
		PMD32->driveB.image = NULL;

		if (PMD32->driveC.image)
			delete [] PMD32->driveC.image;
		PMD32->driveC.image = NULL;

		if (PMD32->driveD.image)
			delete [] PMD32->driveD.image;
		PMD32->driveD.image = NULL;

		delete PMD32;
	}
	PMD32 = NULL;

	cfgIniLine *n = cfgRoot;
	while (n) {
		if (n->key) {
			delete [] n->key;
			n->key = NULL;
		}
		if (n->value) {
			delete [] n->value;
			n->value = NULL;
		}

		if (n->next) {
			n = n->next;
			delete n->prev;
			n->prev = NULL;
		}
		else {
			delete n;
			n = NULL;
		}
	}
}
//-----------------------------------------------------------------------------
TSettings::SetRomPackage *TSettings::findROMmodule(char *name)
{
	if (name)
		for (int i = 0; i < romPackagesCount; i++)
			if (strcmp(RomPackages[i]->name, name) == 0)
				return RomPackages[i];

	return NULL;
}
//-----------------------------------------------------------------------------
TSettings::SetRomModuleFile *TSettings::checkRMMfile(char *name)
{
	static int k = 0;

	if (name == NULL) {
		k = 0;
		return NULL;
	}

	SetRomModuleFile *rmf = new SetRomModuleFile;
	rmf->rmmFile = new char[strlen(name) + 1];
	strcpy(rmf->rmmFile, name);

	rmf->err = 1;
	rmf->size = 0;
	if (strcmp(strrchr(rmf->rmmFile, '.'), ".rmm") == 0) {
		if (FILE *f = fopen(LocateROM(rmf->rmmFile), "rb")) {
			if (fseek(f, 0, SEEK_END) == 0)
				rmf->size = ftell(f);
			fclose(f);

			k += (rmf->size + KB - 1) / KB;

			if (rmf->size < KB
			 || rmf->size > (32 * KB)
			 || (rmf->size & 0x3FF) > 0)
				rmf->err = 2;
			else if (k > 32)
				rmf->err = 3;
			else if (k > 64)
				rmf->err = 4;
			else if (k > 256)
				rmf->err = 5;
			else
				rmf->err = 0;
		}
	}

	return rmf;
}
//-----------------------------------------------------------------------------
void TSettings::cfgReadFile(char *fileName)
{
	int i, fileSize = 0;
	char lineBuffer[256];
	char *buffer = NULL, *ptr, *ptr2;
	cfgRoot = NULL;

	if (FILE *fn = fopen(fileName, "rb")) {
		fseek(fn, 0, SEEK_END);
		fileSize = ftell(fn);
		if (fileSize > 0) {
			fseek(fn, 0, SEEK_SET);
			buffer = new char[fileSize];
			fileSize = fread(buffer, sizeof(char), fileSize, fn);
		}
		fclose(fn);
	}

	if (buffer != NULL && fileSize > 0) {
		cfgIniLine *entry = (cfgRoot = new cfgIniLine);
		entry->prev = NULL;
		ptr = buffer;

		do {
			i = 0;
			while (*ptr != '\n' && i < 256)
				lineBuffer[i++] = *ptr++;
			lineBuffer[i] = '\0';
			ptr++;

			ptr2 = lineBuffer + (i - 1);
			while (*ptr2 == ' ' || *ptr2 == '\t' || *ptr2 == '\r') {
				*ptr2 = '\0';
				ptr2--;
			}

			entry->key = NULL;
			entry->value = NULL;
			entry->ptr = NULL;

			// empty line
			if (i == 0)
				entry->type = LT_EMPTY;

			// comment
			else if (lineBuffer[0] == '#') {
				ptr2 = lineBuffer + 1;
				if (*ptr2 == ' ' || *ptr2 == '\t')
					ptr2++;

				unsigned int q = 0;
				strccnt(ptr2, '-', q);
				if (q == strlen(ptr2))
					entry->type = LT_DELIMITER;
				else {
					entry->type = LT_COMMENT;
					entry->value = new char[i];
					strcpy(entry->value, ptr2);
				}
			}

			// section
			else if (lineBuffer[0] == '[' && *ptr2 == ']') {
				entry->type = LT_SECTION;
				entry->key = new char[i];

				*ptr2 = '\0';
				strcpy(entry->key, lineBuffer + 1);
			}

			// list
			else if (lineBuffer[0] == '"') {
				entry->type = LT_LIST;

				ptr2 = strchr(lineBuffer + 1, '"');
				*ptr2 = '\0';
				entry->key = new char[strlen(lineBuffer + 1) + 1];
				strcpy(entry->key, lineBuffer + 1);
				*ptr2 = '"';

				ptr2 = strchr(ptr2 + 1, '=');
				if (ptr2 == NULL) {
					entry->type = LT_COMMENT;
					delete [] entry->key;
					entry->key = NULL;
					entry->value = new char[strlen(lineBuffer) + 1];
					strcpy(entry->value, lineBuffer);
				}
				else {
					ptr2++;
					while (*ptr2 == ' ' || *ptr2 == '\t')
						ptr2++;

					entry->value = new char[strlen(ptr2) + 1];
					strcpy(entry->value, ptr2);
				}
			}

			// item
			else if ((ptr2 = strchr(lineBuffer, '=')) != NULL) {
				*ptr2 = '\0';
				entry->type = LT_ITEM;

				ptr2++;
				while (*ptr2 == ' ' || *ptr2 == '\t')
					ptr2++;

				if (*ptr2 == '"' && *(ptr2 + strlen(ptr2) - 1) == '"') {
					entry->type = LT_QUOTED;
					*(ptr2 + strlen(ptr2) - 1) = '\0';
					entry->value = new char[strlen(++ptr2) + 1];
					strcpy(entry->value, ptr2);
				}
				else {
					entry->value = new char[strlen(ptr2) + 1];
					strcpy(entry->value, ptr2);
				}

				ptr2 = lineBuffer + strlen(lineBuffer) - 1;
				while (*ptr2 == ' ' || *ptr2 == '\t') {
					*ptr2 = '\0';
					ptr2--;
				}

				ptr2 = lineBuffer;
				while (*ptr2 == ' ' || *ptr2 == '\t')
					ptr2++;

				entry->key = new char[strlen(ptr2) + 1];
				strcpy(entry->key, ptr2);
			}
			else {
				entry->type = LT_COMMENT;
				entry->value = new char[i];
				strcpy(entry->value, lineBuffer);
			}

			entry->next = new cfgIniLine;
			entry->next->prev = entry;
			entry = entry->next;

		} while (ptr < (buffer + fileSize - 1));

		entry->type  = LT_EMPTY;
		entry->key   = NULL;
		entry->value = NULL;
		entry->ptr   = NULL;
		entry->next  = NULL;
	}
}
//-----------------------------------------------------------------------------
TSettings::cfgIniLine *TSettings::cfgFindSection(cfgIniLine *node, const char * name)
{
	debug("[Settings] Parsing %s section...", name);

	while (node != NULL) {
		if (node->type == LT_SECTION && strcmp(node->key, name) == 0)
			break;

		node = node->next;
	}

	if (node == NULL)
		error("[Settings] %s section missing", name);

	return node;
}
//-----------------------------------------------------------------------------
int TSettings::cfgCountChildAttributes(cfgIniLine *node)
{
	int count = 0;

	if (node != NULL)
		node = node->next;

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER && node->type != LT_COMMENT)
			count++;

		node = node->next;
	}

	return count;
}
//-----------------------------------------------------------------------------
char * TSettings::cfgGetStringValue(cfgIniLine *node, const char *key, char **target)
{
	char *val = NULL;
	cfgIniLine *backup = NULL;

	if (node != NULL)
		backup = (node = node->next);

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER &&
				node->type != LT_COMMENT && strcmp(node->key, key) == 0) {

			if (strlen(node->value)) {
				val = new char[strlen(node->value) + 1];
				strcpy(val, node->value);
			}

			if (target)
				node->ptr = (void *) target;

			if (node->type != LT_QUOTED)
				node->type = LT_STRING;

			break;
		}

		node = node->next;
	}

	if (node == NULL && backup)
		cfgInsertNewLine(backup, node->key, LT_QUOTED, (void *) target);

	return val;
}
//-----------------------------------------------------------------------------
int TSettings::cfgGetIntValue(cfgIniLine *node, const char *key, int dflt, int *target)
{
	int val = dflt;
	cfgIniLine *backup = NULL;

	if (node != NULL)
		backup = (node = node->next);

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER &&
				node->type != LT_COMMENT && strcmp(node->key, key) == 0) {

			char *t;
			val = strtol(node->value, &t, 10);
			if (*t)
				val = dflt;

			if (target) {
				*target = val;
				node->ptr = (void *) target;
				node->type = LT_NUMBER;
			}
			break;
		}

		node = node->next;
	}

	if (node == NULL && backup && target)
		cfgInsertNewLine(backup, node->key, LT_NUMBER, (void *) target);

	return val;
}
//-----------------------------------------------------------------------------
bool TSettings::cfgGetBoolValue(cfgIniLine *node, const char *key, bool dflt, bool *target)
{
	bool val = dflt;
	cfgIniLine *backup = NULL;

	if (node != NULL)
		backup = (node = node->next);

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER &&
				node->type != LT_COMMENT && strcmp(node->key, key) == 0) {

			if (strcmp(node->value, "true") == 0 || strcmp(node->value, "1") == 0)
				val = true;
			else if (strcmp(node->value, "false") == 0 || strcmp(node->value, "0") == 0)
				val = false;

			if (target) {
				*target = val;
				node->ptr = (void *) target;
				node->type = LT_BOOL;
			}
			break;
		}

		node = node->next;
	}

	if (node == NULL && backup && target)
		cfgInsertNewLine(backup, node->key, LT_BOOL, (void *) target);

	return val;
}
//-----------------------------------------------------------------------------
TColor TSettings::cfgGetColorValue(cfgIniLine *node, const char *key, TColor dflt, TColor *target)
{
	TColor val = dflt;
	cfgIniLine *backup = NULL;

	if (node != NULL)
		backup = (node = node->next);

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER &&
				node->type != LT_COMMENT && strcmp(node->key, key) == 0) {

			if (strcmp(node->value, "black") == 0)
				val = BLACK;
			else if (strcmp(node->value, "maroon") == 0)
				val = MAROON;
			else if (strcmp(node->value, "green") == 0)
				val = GREEN;
			else if (strcmp(node->value, "olive") == 0)
				val = OLIVE;
			else if (strcmp(node->value, "navy") == 0)
				val = NAVY;
			else if (strcmp(node->value, "purple") == 0)
				val = PURPLE;
			else if (strcmp(node->value, "teal") == 0)
				val = TEAL;
			else if (strcmp(node->value, "gray") == 0)
				val = GRAY;
			else if (strcmp(node->value, "silver") == 0)
				val = SILVER;
			else if (strcmp(node->value, "red") == 0)
				val = RED;
			else if (strcmp(node->value, "lime") == 0)
				val = LIME;
			else if (strcmp(node->value, "yellow") == 0)
				val = YELLOW;
			else if (strcmp(node->value, "blue") == 0)
				val = BLUE;
			else if (strcmp(node->value, "fuchsia") == 0)
				val = FUCHSIA;
			else if (strcmp(node->value, "aqua") == 0)
				val = AQUA;
			else if (strcmp(node->value, "white") == 0)
				val = WHITE;

			if (target) {
				*target = val;
				node->ptr = (void *) target;
				node->type = LT_COLOR;
			}
			break;
		}

		node = node->next;
	}

	if (node == NULL && backup && target)
		cfgInsertNewLine(backup, node->key, LT_COLOR, (void *) target);

	return val;
}
//-----------------------------------------------------------------------------
TSettings::cfgIniLine *TSettings::cfgGetLine(cfgIniLine *node, const char *key)
{
	cfgIniLine *val = NULL;

	if (node != NULL)
		node = node->next;

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER &&
				node->type != LT_COMMENT && strcmp(node->key, key) == 0) {

			val = node;
			break;
		}

		node = node->next;
	}

	return val;
}
//-----------------------------------------------------------------------------
bool TSettings::cfgHasKeyValue(cfgIniLine *node, const char *key, const char *value)
{
	bool val = false;

	if (node != NULL)
		node = node->next;

	while (node != NULL) {
		if (node->type == LT_SECTION)
			break;
		else if (node->type != LT_EMPTY && node->type != LT_DELIMITER &&
				node->type != LT_COMMENT && strcmp(node->key, key) == 0 &&
				strcmp(node->value, value) == 0) {

			val = true;
			break;
		}

		node = node->next;
	}

	return val;
}
//-----------------------------------------------------------------------------
void TSettings::cfgInsertNewLine(cfgIniLine *node, const char *key, cfgIniLineType type, void *ptr)
{
	cfgIniLine *next = node->next;
	node->next = new cfgIniLine;
	node->next->prev = node->next;

	node = node->next;
	node->type = type;
	node->key = new char[strlen(key) + 1];
	strcpy(node->key, key);
	node->value = NULL;
	node->ptr = ptr;
	node->next = next;
}
//-----------------------------------------------------------------------------
