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
	int i = 0, j, k;
	char *buf;

	if (LocateResource("GPMD85Emu.dtd", true) == NULL)
		warning("Configuration scheme not found!");
	else
		i = XML_PARSE_DTDVALID;

	if ((buf = LocateResource("GPMD85Emu.xml", true)) == NULL)
		error("Configuration file not found!");

	xmlDoc = xmlReadFile(buf, "UTF-8", i);
	if (xmlDoc == NULL)
		error("Couldn't parse configuration file:\n> %s", buf);

	xmlRoot = xmlDocGetRootElement(xmlDoc);
	if (xmlStrcmp(xmlRoot->name, (const xmlChar *) "GPMD85Emulator") != 0)
		error("Invalid configuration file!");

	if (!cfgIsAttributeToken(xmlRoot, "version", CONFIGURATION_VERSION))
		warning("Incompatible configuration file version (required %s)!", CONFIGURATION_VERSION);

	xmlChar *s;
	xmlNodePtr m, n = cfgGetChildNode(xmlRoot, "General");
	pauseOnFocusLost = cfgGetAttributeBoolValue(n, "pause-on-focus-lost", false);
	isPaused = false;

	n = cfgGetChildNode(xmlRoot, "RomPackages");
	romPackagesCount = cfgCountChildNodes(n);
	RomPackages = new SetRomPackage *[romPackagesCount];

	n = n->children;
	SetRomModuleFile *rmf = NULL;
	for (i = 0; n != NULL || i < romPackagesCount; n = n->next) {
		if (xmlStrcmp(n->name, (const xmlChar *) "RomPackage") == 0 && n->type == XML_ELEMENT_NODE) {
			RomPackages[i] = new SetRomPackage;
			RomPackages[i]->name = cfgGetAttributeToken(n, "name");
			RomPackages[i]->count = cfgCountChildNodes(n);
			RomPackages[i]->files = new SetRomModuleFile *[RomPackages[i]->count];

			k = 0;
			m = n->children;
			for (j = 0; m != NULL || j < RomPackages[i]->count; m = m->next) {
				if (xmlStrcmp(m->name, (const xmlChar *) "RomFile") == 0 && m->type == XML_ELEMENT_NODE) {
					s = xmlNodeGetContent(m);
					if (s) {
						rmf = new SetRomModuleFile;
						rmf->rmmFile = cfgConvertXmlString(s);

						rmf->err = 1;
						rmf->size = 0;
						if (strcmp(strrchr(rmf->rmmFile, '.'), ".rmm") == 0) {
							FILE *f = fopen(LocateROM(rmf->rmmFile), "rb");
							if (f) {
								if (fseek(f, 0, SEEK_END) == 0)
									rmf->size = ftell(f);
								fclose(f);

								k += (rmf->size + KBYTE - 1) / KBYTE;

								if (rmf->size < KBYTE
								 || rmf->size > (32 * KBYTE)
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

						RomPackages[i]->files[j] = rmf;
					}

					j++;
				}
			}

			i++;
		}
	}

	n = cfgGetChildNode(xmlRoot, "Model");
	buf = cfgGetAttributeToken(n, "current");

	modelsCount = cfgCountChildNodes(n);
	AllModels = new SetComputerModel *[modelsCount];
	CurrentModel = NULL;

	n = n->children;
	for (i = 0; n != NULL || i < modelsCount; n = n->next) {
		if (n->type != XML_ELEMENT_NODE)
			continue;

		SetComputerModel *model = new SetComputerModel;

		if (xmlStrcmp(n->name, (const xmlChar *) "Model-1") == 0)
			model->type = CM_V1;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-2") == 0)
			model->type = CM_V2;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-2A") == 0)
			model->type = CM_V2A;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-3") == 0)
			model->type = CM_V3;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-Mato") == 0)
			model->type = CM_MATO;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-Alfa") == 0)
			model->type = CM_ALFA;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-Alfa2") == 0)
			model->type = CM_ALFA2;
		else if (xmlStrcmp(n->name, (const xmlChar *) "Model-C2717") == 0)
			model->type = CM_C2717;

		model->compatibilityMode = cfgGetAttributeBoolValue(n, "compatibility-mode", false);
		model->romFile = cfgConvertXmlString(xmlNodeGetContent(cfgGetChildNode(n, "RomFile")));

		if (model->romFile == NULL) {
			warning("[Settings] RomFile not defined for %s, will be ignored...", n->name);
			modelsCount--;
			delete model;
			continue;
		}

		m = cfgGetChildNode(n, "RomModule");
		model->romModuleInserted = cfgGetAttributeBoolValue(m, "inserted", false);
		s = xmlGetProp(m, (const xmlChar *) "name");
		if ((model->romModule = findROMmodule((char *) s)) == NULL)
			model->romModuleInserted = false;

		xmlFree(s);

		if (xmlStrcmp(n->name, (const xmlChar *) buf) == 0)
			CurrentModel = model;

		AllModels[i] = model;
		i++;
	}

	if (CurrentModel == NULL) {
		warning("[Settings] Current model '%s' not found!", buf);
		CurrentModel = AllModels[0];
	}

	if (buf)
		delete [] buf;
	buf = NULL;

	n = cfgGetChildNode(xmlRoot, "Snapshot");

	Snapshot = new SetSnapshot;
	Snapshot->saveCompressed = cfgGetAttributeBoolValue(n, "save-snapshot-compressed", true);
	Snapshot->saveWithMonitor = cfgGetAttributeBoolValue(n, "save-snapshot-with-monitor", false);
	Snapshot->dontRunOnLoad = cfgGetAttributeBoolValue(n, "dont-run-snapshot-on-load", false);
	Snapshot->fileName = cfgConvertXmlString(xmlNodeGetContent(n));

	n = cfgGetChildNode(xmlRoot, "TapeBrowser");

	TapeBrowser = new SetTapeBrowser;
	TapeBrowser->monitoring = cfgGetAttributeBoolValue(n, "monitoring", false);
	TapeBrowser->flash = cfgGetAttributeBoolValue(n, "flash", false);
	TapeBrowser->fileName = cfgConvertXmlString(xmlNodeGetContent(n));

	if (cfgIsAttributeToken(n, "auto-stop", "next-head"))
		TapeBrowser->autoStop = AS_NEXTHEAD;
	else if (cfgIsAttributeToken(n, "auto-stop", "cursor"))
		TapeBrowser->autoStop = AS_CURSOR;
	else
		TapeBrowser->autoStop = AS_OFF;

	n = cfgGetChildNode(xmlRoot, "Screen");

	Screen = new SetScreen;
	Screen->border = cfgGetAttributeIntValue(n, "border", 0);

	if (cfgIsAttributeToken(n, "size", "double"))
		Screen->size = DM_DOUBLESIZE;
	else if (cfgIsAttributeToken(n, "size", "triple"))
		Screen->size = DM_TRIPLESIZE;
	else
		Screen->size = DM_NORMAL;

	Screen->lcdMode = false;
	Screen->halfPass = HP_OFF;
	if (cfgIsAttributeToken(n, "half-pass", "lcd"))
		Screen->lcdMode = true;
	else if (cfgIsAttributeToken(n, "half-pass", "b75"))
		Screen->halfPass = HP_75;
	else if (cfgIsAttributeToken(n, "half-pass", "b50"))
		Screen->halfPass = HP_50;
	else if (cfgIsAttributeToken(n, "half-pass", "b25"))
		Screen->halfPass = HP_25;
	else if (cfgIsAttributeToken(n, "half-pass", "b0"))
		Screen->halfPass = HP_0;

	if (cfgIsAttributeToken(n, "color-profile", "mono"))
		Screen->colorProfile = CP_MONO;
	else if (cfgIsAttributeToken(n, "color-profile", "color"))
		Screen->colorProfile = CP_COLOR;
	else if (cfgIsAttributeToken(n, "color-profile", "multicolor"))
		Screen->colorProfile = CP_MULTICOLOR;
	else
		Screen->colorProfile = CP_STANDARD;

	if (cfgIsAttributeToken(n, "color-pallette", "rgb"))
		Screen->colorPalette = CL_RGB;
	else if (cfgIsAttributeToken(n, "color-pallette", "video"))
		Screen->colorPalette = CL_VIDEO;
	else
		Screen->colorPalette = CL_DEFINED;

	Screen->attr00 = cfgGetAttributeColorValue(n, "attr00", WHITE);
	Screen->attr01 = cfgGetAttributeColorValue(n, "attr01", GREEN);
	Screen->attr10 = cfgGetAttributeColorValue(n, "attr10", AQUA);
	Screen->attr11 = cfgGetAttributeColorValue(n, "attr11", YELLOW);

	n = cfgGetChildNode(xmlRoot, "Sound");

	Sound = new SetSound;
	k = cfgGetAttributeIntValue(n, "volume", 64);
	if (k < 0 || k > 127)
		k = 64;

	Sound->hwBuffer = cfgGetAttributeBoolValue(n, "hw-buff", true);
	Sound->ifMusica = cfgGetAttributeBoolValue(n, "if-musica", false);
	Sound->mute = cfgGetAttributeBoolValue(n, "mute", false);
	Sound->volume = (BYTE) k;

	if ((buf = cfgGetAttributeToken(n, "out-type")) != NULL) {
		strncpy(Sound->outType, buf, sizeof(char) * 3);
		delete [] buf;
		buf = NULL;
	}

	m = cfgGetChildNode(xmlRoot, "Controls");
	n = cfgGetChildNode(m, "Keyboard");

	Keyboard = new SetKeyboard;
	Keyboard->changeZY = cfgGetAttributeBoolValue(n, "change-zy", false);
	Keyboard->useNumpad = cfgGetAttributeBoolValue(n, "use-numpad", false);
	Keyboard->useMatoCtrl = cfgGetAttributeBoolValue(n, "mato-ctrl", false);

	n = cfgGetChildNode(m, "Mouse");

	Mouse = new SetMouse;
	Mouse->hideCursor = cfgGetAttributeBoolValue(n, "hide-cursor", true);
	if (cfgIsAttributeToken(n, "type", "m602"))
		Mouse->type = MT_M602;
	else if (cfgIsAttributeToken(n, "type", "poly8"))
		Mouse->type = MT_POLY8;
	else
		Mouse->type = MT_NONE;

	n = cfgGetChildNode(m, "Joystick");
	Joystick = new SetJoystick;

	m = cfgGetChildNode(n, "Gpio0");
	Joystick->GPIO0 = new SetJoystickGPIO;
	Joystick->GPIO0->connected = cfgGetAttributeBoolValue(m, "connected", false);
	Joystick->GPIO0->guid = cfgGetAttributeToken(m, "guid");
	Joystick->GPIO0->ctrlLeft = cfgGetAttributeIntValue(m, "left", -1);
	Joystick->GPIO0->ctrlRight = cfgGetAttributeIntValue(m, "right", -1);
	Joystick->GPIO0->ctrlUp = cfgGetAttributeIntValue(m, "up", -1);
	Joystick->GPIO0->ctrlDown = cfgGetAttributeIntValue(m, "down", -1);
	Joystick->GPIO0->ctrlFire = cfgGetAttributeIntValue(m, "fire", -1);
	Joystick->GPIO0->sensitivity = cfgGetAttributeIntValue(m, "sensitivity", 0);
	Joystick->GPIO0->pov = cfgGetAttributeIntValue(m, "pov", -1);

	if (cfgIsAttributeToken(m, "type", "keys"))
		Joystick->GPIO0->type = JT_KEYS;
	else if (cfgIsAttributeToken(m, "type", "axes"))
		Joystick->GPIO0->type = JT_AXES;
	else if (cfgIsAttributeToken(m, "type", "pov"))
		Joystick->GPIO0->type = JT_POV;
	else if (cfgIsAttributeToken(m, "type", "buttons"))
		Joystick->GPIO0->type = JT_BUTTONS;
	else
		Joystick->GPIO0->type = JT_NONE;

	m = cfgGetChildNode(n, "Gpio1");
	Joystick->GPIO1 = new SetJoystickGPIO;
	Joystick->GPIO1->connected = cfgGetAttributeBoolValue(m, "connected", false);
	Joystick->GPIO1->guid = cfgGetAttributeToken(m, "guid");
	Joystick->GPIO1->ctrlLeft = cfgGetAttributeIntValue(m, "left", -1);
	Joystick->GPIO1->ctrlRight = cfgGetAttributeIntValue(m, "right", -1);
	Joystick->GPIO1->ctrlUp = cfgGetAttributeIntValue(m, "up", -1);
	Joystick->GPIO1->ctrlDown = cfgGetAttributeIntValue(m, "down", -1);
	Joystick->GPIO1->ctrlFire = cfgGetAttributeIntValue(m, "fire", -1);
	Joystick->GPIO1->sensitivity = cfgGetAttributeIntValue(m, "sensitivity", 0);
	Joystick->GPIO1->pov = cfgGetAttributeIntValue(m, "pov", -1);

	if (cfgIsAttributeToken(m, "type", "keys"))
		Joystick->GPIO1->type = JT_KEYS;
	else if (cfgIsAttributeToken(m, "type", "axes"))
		Joystick->GPIO1->type = JT_AXES;
	else if (cfgIsAttributeToken(m, "type", "pov"))
		Joystick->GPIO1->type = JT_POV;
	else if (cfgIsAttributeToken(m, "type", "buttons"))
		Joystick->GPIO1->type = JT_BUTTONS;
	else
		Joystick->GPIO1->type = JT_NONE;

	n = cfgGetChildNode(cfgGetChildNode(xmlRoot, "Storages"), "PMD-32");

	PMD32 = new SetStoragePMD32;
	PMD32->connected = cfgGetAttributeBoolValue(n, "connected", false);
	PMD32->extraCommands = cfgGetAttributeBoolValue(n, "extra-commands", false);
	PMD32->romFile = cfgConvertXmlString(xmlNodeGetContent(cfgGetChildNode(n, "RomFile")));
	PMD32->sdRoot = cfgConvertXmlString(xmlNodeGetContent(cfgGetChildNode(n, "SD-Root")));

	m = cfgGetChildNode(n, "DriveA");
	PMD32->driveA.image = cfgConvertXmlString(xmlNodeGetContent(m));
	PMD32->driveA.writeProtect = cfgGetAttributeBoolValue(m, "write-protect", false);

	m = cfgGetChildNode(n, "DriveB");
	PMD32->driveB.image = cfgConvertXmlString(xmlNodeGetContent(m));
	PMD32->driveB.writeProtect = cfgGetAttributeBoolValue(m, "write-protect", false);

	m = cfgGetChildNode(n, "DriveC");
	PMD32->driveC.image = cfgConvertXmlString(xmlNodeGetContent(m));
	PMD32->driveC.writeProtect = cfgGetAttributeBoolValue(m, "write-protect", false);

	m = cfgGetChildNode(n, "DriveD");
	PMD32->driveD.image = cfgConvertXmlString(xmlNodeGetContent(m));
	PMD32->driveD.writeProtect = cfgGetAttributeBoolValue(m, "write-protect", false);

	n = cfgGetChildNode(cfgGetChildNode(xmlRoot, "Storages"), "RaomModule");

	RaomModule = new SetStorageRAOM;
	RaomModule->inserted = cfgGetAttributeBoolValue(n, "inserted", false);
	RaomModule->type = (cfgIsAttributeToken(n, "hw-version", "chtf")) ? RT_CHTF : RT_KUVI;
	RaomModule->file = cfgConvertXmlString(xmlNodeGetContent(n));

	s = xmlGetProp(n, (const xmlChar *) "name");
	if ((RaomModule->module = findROMmodule((char *) s)) == NULL)
		RaomModule->inserted = false;

	xmlFree(s);
}
//-----------------------------------------------------------------------------
TSettings::~TSettings()
{
	int i, j;

	xmlFreeDoc(xmlDoc);

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
//-- libxml2 extension methods for GPMD85emu ----------------------------------
//-----------------------------------------------------------------------------
xmlNodePtr TSettings::cfgGetChildNode(xmlNodePtr node, const char * name)
{
	xmlNodePtr cur = node->children;

	while (cur != NULL) {
		if (xmlStrcmp(cur->name, (const xmlChar *) name) == 0 && cur->type == XML_ELEMENT_NODE)
			break;

		cur = cur->next;
	}

	return cur;
}
//-----------------------------------------------------------------------------
int TSettings::cfgCountChildNodes(xmlNodePtr node)
{
	int count = 0;
	xmlNodePtr cur = node->children;

	while (cur != NULL) {
		if (cur->type == XML_ELEMENT_NODE)
			count++;

		cur = cur->next;
	}

	return count;
}
//-----------------------------------------------------------------------------
char * TSettings::cfgGetAttributeToken(xmlNodePtr node, const char *attrName)
{
	char *val = NULL;

	xmlChar *s = xmlGetProp(node, (const xmlChar *) attrName);
	if (s) {
		if (xmlStrlen(s) > 0) {
			val = new char[(xmlStrlen(s) + 1)];
			strcpy(val, (const char *) s);
		}

		xmlFree(s);
	}

	return val;
}
//-----------------------------------------------------------------------------
int TSettings::cfgGetAttributeIntValue(xmlNodePtr node, const char *attrName, int dflt)
{
	int val = dflt;

	char *t, *s = (char *) xmlGetProp(node, (const xmlChar *) attrName);
	if (s) {
		val = strtol(s, &t, 10);
		if (*t)
			val = dflt;

		xmlFree(s);
	}

	return val;
}
//-----------------------------------------------------------------------------
bool TSettings::cfgGetAttributeBoolValue(xmlNodePtr node, const char *attrName, bool dflt)
{
	bool val = dflt;

	xmlChar *s = xmlGetProp(node, (const xmlChar *) attrName);
	if (s) {
		if (xmlStrcmp(s, (const xmlChar *) "true") == 0)
			val = true;
		else if (xmlStrcmp(s, (const xmlChar *) "false") == 0)
			val = false;

		xmlFree(s);
	}

	return val;
}
//-----------------------------------------------------------------------------
TColor TSettings::cfgGetAttributeColorValue(xmlNodePtr node, const char *attrName, TColor dflt)
{
	TColor val = dflt;

	xmlChar *s = xmlGetProp(node, (const xmlChar *) attrName);
	if (s) {
		if (xmlStrcmp(s, (const xmlChar *) "black") == 0)
			val = BLACK;
		else if (xmlStrcmp(s, (const xmlChar *) "maroon") == 0)
			val = MAROON;
		else if (xmlStrcmp(s, (const xmlChar *) "green") == 0)
			val = GREEN;
		else if (xmlStrcmp(s, (const xmlChar *) "olive") == 0)
			val = OLIVE;
		else if (xmlStrcmp(s, (const xmlChar *) "navy") == 0)
			val = NAVY;
		else if (xmlStrcmp(s, (const xmlChar *) "purple") == 0)
			val = PURPLE;
		else if (xmlStrcmp(s, (const xmlChar *) "teal") == 0)
			val = TEAL;
		else if (xmlStrcmp(s, (const xmlChar *) "gray") == 0)
			val = GRAY;
		else if (xmlStrcmp(s, (const xmlChar *) "silver") == 0)
			val = SILVER;
		else if (xmlStrcmp(s, (const xmlChar *) "red") == 0)
			val = RED;
		else if (xmlStrcmp(s, (const xmlChar *) "lime") == 0)
			val = LIME;
		else if (xmlStrcmp(s, (const xmlChar *) "yellow") == 0)
			val = YELLOW;
		else if (xmlStrcmp(s, (const xmlChar *) "blue") == 0)
			val = BLUE;
		else if (xmlStrcmp(s, (const xmlChar *) "fuchsia") == 0)
			val = FUCHSIA;
		else if (xmlStrcmp(s, (const xmlChar *) "aqua") == 0)
			val = AQUA;
		else if (xmlStrcmp(s, (const xmlChar *) "white") == 0)
			val = WHITE;

		xmlFree(s);
	}

	return val;
}
//-----------------------------------------------------------------------------
bool TSettings::cfgIsAttributeToken(xmlNodePtr node, const char *attrName, const char *token)
{
	bool val = false;

	xmlChar *s = xmlGetProp(node, (const xmlChar *) attrName);
	if (s) {
		if (xmlStrcmp(s, (const xmlChar *) token) == 0)
			val = true;

		xmlFree(s);
	}

	return val;
}
//-----------------------------------------------------------------------------
char *TSettings::cfgConvertXmlString(xmlChar *s)
{
	char *out = NULL;

	if (s) {
		if (xmlStrlen(s) > 0) {
			out = new char[(xmlStrlen(s) + 1)];
			strcpy(out, (const char *) s);
		}

		xmlFree(s);
	}

	return out;
}
//-----------------------------------------------------------------------------
