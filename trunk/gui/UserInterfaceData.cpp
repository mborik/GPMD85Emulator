/*	UserInterfaceData.h
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
#include "UserInterfaceData.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
const char *dcb_tape_save_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = TapeBrowser->tapeChanged;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_tape_noempty_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = (TapeBrowser->totalBlocks > 0);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_tape_contblk_state(GUI_MENU_ENTRY *ptr)
{
	if (TapeBrowser->totalBlocks > 0) {
		int f = TapeBrowser->Selection->first,
			l = TapeBrowser->Selection->last,
			i = (ptr->action & 0x1ff);

		ptr->enabled = TapeBrowser->Selection->continuity;
		if (TapeBrowser->Selection->total == 0) {
			ptr->enabled = true;
			f = l = GUI->tapeDialog->popup.hilite;
		}

		if ((i == SDLK_UP && f == 0) ||
			(i == SDLK_DOWN && l == (TapeBrowser->totalBlocks - 1)))
			ptr->enabled = false;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_view_size_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = (gvi.wm) ? true : false;
	ptr->state = (Settings->Screen->size == (TDisplayMode) ptr->action);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_view_cmod_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = (Settings->Screen->colorProfile == (TColorProfile) ptr->action);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_view_cpal_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = (Settings->Screen->colorPalette == (TColorPalette) ptr->action);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_view_ccol_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = (Settings->Screen->colorPalette == CL_DEFINED);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_view_sclr_state(GUI_MENU_ENTRY *ptr)
{
	if (ptr->action == (WORD) -1)
		ptr->state = Settings->Screen->lcdMode;
	else if (!Settings->Screen->lcdMode)
		ptr->state = (Settings->Screen->halfPass == (THalfPassMode) ptr->action);
	else
		ptr->state = false;

	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_view_brdr_state(GUI_MENU_ENTRY *ptr)
{
	if (Settings->Screen->size == DM_FULLSCREEN) {
		ptr->enabled = false;
		return NULL;
	}
	else {
		ptr->action = Settings->Screen->border;
		sprintf((char *) uicch, "%dx", ptr->action);
		return uicch;
	}
}
//-----------------------------------------------------------------------------
const char *dcb_snd_mute_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->Sound->mute;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_snd_volume_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = !Settings->Sound->mute;
	ptr->action = Settings->Sound->volume;
	sprintf((char *) uicch, "%d", ptr->action);
	return uicch;
}
//-----------------------------------------------------------------------------
const char *dcb_kbd_xchg_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->Keyboard->changeZY;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_kbd_nums_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->Keyboard->useNumpad;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_kbd_mato_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->Keyboard->useMatoCtrl;
	ptr->enabled = (Settings->CurrentModel->type == CM_MATO);
	return "for Ma\213o";
}
//-----------------------------------------------------------------------------
const char *dcb_emu_m3cmp_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = (Settings->CurrentModel->type == CM_V3);
	ptr->state = Settings->CurrentModel->compatibilityMode;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_emu_pause_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->isPaused;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_emu_focus_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->pauseOnFocusLost;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_emu_asave_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->autosaveSettings;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_machine_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = (Settings->CurrentModel->type == (TComputerModel) ptr->action);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_mem_file_state(GUI_MENU_ENTRY *ptr)
{
	return ExtractFileName(Settings->CurrentModel->romFile);
}
//-----------------------------------------------------------------------------
const char *dcb_mem_rmod_state(GUI_MENU_ENTRY *ptr)
{
	switch (Settings->CurrentModel->type) {
		case CM_V1:
		case CM_V2:
		case CM_V2A:
		case CM_V3:
			ptr->enabled = true;
			ptr->state = Settings->CurrentModel->romModuleInserted;
			break;

		default:
			ptr->enabled = ptr->state = false;
			break;
	}

	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_mem_rpkg_state(GUI_MENU_ENTRY *ptr)
{
	if (gui_rom_packages == NULL) {
		int c = Settings->romPackagesCount, i;
		if (c > 20)
			c = 20;

		gui_rom_packages = new GUI_MENU_ENTRY[c + 2];
		gui_rom_packages[0].type = MI_TITLE;
		gui_rom_packages[0].text = "ROM PACKAGES";

		for (i = 0; i < c; i++) {
			gui_rom_packages[i + 1].type = MI_RADIO;
			gui_rom_packages[i + 1].text = Settings->RomPackages[i]->name;
			gui_rom_packages[i + 1].hotkey = NULL;
			gui_rom_packages[i + 1].submenu = NULL;
			gui_rom_packages[i + 1].key = SDLK_LAST;
			gui_rom_packages[i + 1].enabled = true;
			gui_rom_packages[i + 1].state = false;
			gui_rom_packages[i + 1].action = i;
			gui_rom_packages[i + 1].detail = dcb_rom_pckg_state;
			gui_rom_packages[i + 1].callback = ccb_rom_pckg;
		}

		gui_rom_packages[i + 1].type = MENU_END;
		ptr->submenu = gui_rom_packages;
	}

	switch (Settings->CurrentModel->type) {
		case CM_V1:
		case CM_V2:
		case CM_V2A:
		case CM_V3:
			ptr->enabled = true;
			return ExtractFileName(Settings->CurrentModel->romModule->name);

		default:
			ptr->enabled = false;
			return NULL;
	}
}
//-----------------------------------------------------------------------------
const char *dcb_rom_pckg_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = (Settings->CurrentModel->romModule->name == ptr->text);
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_p32_conn_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->PMD32->connected;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_p32_file_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = Settings->PMD32->connected;
	return ExtractFileName(Settings->PMD32->romFile);
}
//-----------------------------------------------------------------------------
const char *dcb_p32_imgs_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = Settings->PMD32->connected;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_p32_extc_state(GUI_MENU_ENTRY *ptr)
{
	ptr->state = Settings->PMD32->extraCommands;
	ptr->enabled = Settings->PMD32->connected;
	return NULL;
}
//-----------------------------------------------------------------------------
const char *dcb_p32_sdcd_state(GUI_MENU_ENTRY *ptr)
{
	ptr->enabled = Settings->PMD32->connected && Settings->PMD32->extraCommands;
	return ExtractFileName(Settings->PMD32->sdRoot);
}
//-----------------------------------------------------------------------------
const char *dcb_p32_imgd_state(GUI_MENU_ENTRY *ptr)
{
	const char *ret = NULL;

	switch (ptr->action & 0x3FFF) {
		case 1:
			ptr->state = Settings->PMD32->driveA.writeProtect;
			ret = ExtractFileName(Settings->PMD32->driveA.image);
			break;
		case 2:
			ptr->state = Settings->PMD32->driveB.writeProtect;
			ret = ExtractFileName(Settings->PMD32->driveB.image);
			break;
		case 3:
			ptr->state = Settings->PMD32->driveC.writeProtect;
			ret = ExtractFileName(Settings->PMD32->driveC.image);
			break;
		case 4:
			ptr->state = Settings->PMD32->driveD.writeProtect;
			ret = ExtractFileName(Settings->PMD32->driveD.image);
			break;
		default:
			break;
	}

	return ret;
}
//-----------------------------------------------------------------------------
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//-----------------------------------------------------------------------------
bool ccb_tape_command(GUI_MENU_ENTRY *ptr) { return false; }
//-----------------------------------------------------------------------------
bool ccb_tape_new(GUI_MENU_ENTRY *ptr)
{
	Emulator->ActionTapeNew();
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_fileselector(GUI_MENU_ENTRY *ptr)
{
	switch (ptr->action) {
		case 1:
			Emulator->ActionTapeLoad();
			break;

		case 2:
			Emulator->ActionTapeSave();
			break;

		case 3:
			Emulator->ActionSnapLoad();
			break;

		case 4:
			Emulator->ActionSnapSave();
			break;

		case 5:
			Emulator->ActionROMLoad(0);
			break;

		case 6:
			Emulator->ActionROMLoad(32);
			break;

		case 8:
			Emulator->ActionTapeLoad(true);
			break;

		default:
			break;
	}

	return false;
}
//-----------------------------------------------------------------------------
bool ccb_view_size(GUI_MENU_ENTRY *ptr)
{
	Settings->Screen->size = (TDisplayMode) ptr->action;
	GUI->uiSetChanges |= PS_SCREEN_SIZE;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_view_brdr(GUI_MENU_ENTRY *ptr)
{
	sprintf(msgbuffer, "%d", ptr->action);
	if (GUI->editBox("CHANGE BORDER SIZE:", msgbuffer, 1, true) == 1) {
		long int value = strtol(msgbuffer, NULL, 10);
		if (value == 0 && msgbuffer[0] != '0')
			value = -1;

		if (value >= 0 && value <= 9) {
			Settings->Screen->border = (BYTE) value;
			GUI->uiSetChanges |= PS_SCREEN_SIZE;
			ptr->action = value;
		}
	}

	return false;
}
//-----------------------------------------------------------------------------
bool ccb_view_cmod(GUI_MENU_ENTRY *ptr)
{
	Settings->Screen->colorProfile = (TColorProfile) ptr->action;
	GUI->uiSetChanges |= PS_SCREEN_MODE;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_view_cpal(GUI_MENU_ENTRY *ptr)
{
	Settings->Screen->colorPalette = (TColorPalette) ptr->action;
	GUI->uiSetChanges |= PS_SCREEN_MODE;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_view_sclr(GUI_MENU_ENTRY *ptr)
{
	if (ptr->action == (WORD) -1) {
		Settings->Screen->lcdMode = true;
		Settings->Screen->halfPass = HP_OFF;
	}
	else {
		Settings->Screen->lcdMode = false;
		Settings->Screen->halfPass = (THalfPassMode) ptr->action;
	}

	GUI->uiSetChanges |= PS_SCREEN_MODE;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_snd_mute(GUI_MENU_ENTRY *ptr)
{
	Settings->Sound->mute = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_SOUND;

	ptr++;
	ptr->enabled = !Settings->Sound->mute;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_snd_volume(GUI_MENU_ENTRY *ptr)
{
	sprintf(msgbuffer, "%d", ptr->action);
	GUI->editBox("CHANGE VOLUME:", msgbuffer, 3, true);

	long int value = strtol(msgbuffer, NULL, 10);
	if (value > 0 && value < 128) {
		Settings->Sound->volume = (BYTE) value;
		GUI->uiSetChanges |= PS_SOUND;
	}

	return false;
}
//-----------------------------------------------------------------------------
bool ccb_kbd_xchg(GUI_MENU_ENTRY *ptr)
{
	Settings->Keyboard->changeZY = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_CONTROLS;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_kbd_nums(GUI_MENU_ENTRY *ptr)
{
	Settings->Keyboard->useNumpad = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_CONTROLS;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_kbd_mato(GUI_MENU_ENTRY *ptr)
{
	Settings->Keyboard->useMatoCtrl = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_CONTROLS;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_emu_pause(GUI_MENU_ENTRY *ptr)
{
	Settings->isPaused = (ptr->state = !ptr->state);
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_emu_reset(GUI_MENU_ENTRY *ptr)
{
	GUI->uiCallback.connect(Emulator, &TEmulator::ActionReset);
	GUI->uiSetChanges |= PS_CLOSEALL;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_emu_hardreset(GUI_MENU_ENTRY *ptr)
{
	GUI->uiCallback.connect(Emulator, &TEmulator::ActionHardReset);
	GUI->uiSetChanges |= PS_CLOSEALL;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_emu_m3cmp(GUI_MENU_ENTRY *ptr)
{
	Settings->CurrentModel->compatibilityMode = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_MACHINE;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_emu_focus(GUI_MENU_ENTRY *ptr)
{
	Settings->pauseOnFocusLost = (ptr->state = !ptr->state);
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_emu_asave(GUI_MENU_ENTRY *ptr)
{
	Settings->autosaveSettings = (ptr->state = !ptr->state);
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_emu_saves(GUI_MENU_ENTRY *ptr)
{
	Settings->storeSettings();
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_machine(GUI_MENU_ENTRY *ptr)
{
	for (int i = 0; i < Settings->modelsCount; i++) {
		if (Settings->AllModels[i]->type == (TComputerModel) ptr->action) {
			Settings->CurrentModel = Settings->AllModels[i];
			break;
		}
	}

	GUI->uiSetChanges |= PS_MACHINE | PS_CLOSEALL;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_mem_rmod(GUI_MENU_ENTRY *ptr)
{
	Settings->CurrentModel->romModuleInserted = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_MACHINE | PS_PERIPHERALS;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_rom_pckg(GUI_MENU_ENTRY *ptr)
{
	Settings->CurrentModel->romModule = Settings->RomPackages[ptr->action];
	GUI->uiSetChanges |= PS_MACHINE | PS_PERIPHERALS;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_p32_imgd(GUI_MENU_ENTRY *ptr)
{
	if (ptr->action >= 0xC000) {
		ptr->action ^= 0x4000;
		ptr->state = !ptr->state;

		switch (ptr->action & 0xF) {
			case 1:
				Settings->PMD32->driveA.writeProtect = ptr->state;
				break;
			case 2:
				Settings->PMD32->driveB.writeProtect = ptr->state;
				break;
			case 3:
				Settings->PMD32->driveC.writeProtect = ptr->state;
				break;
			case 4:
				Settings->PMD32->driveD.writeProtect = ptr->state;
				break;
			default:
				break;
		}

		GUI->uiSetChanges |= PS_PERIPHERALS;
		return false;
	}
	else
		Emulator->ActionPMD32LoadDisk(ptr->action & 0xF);

	return false;
}
//-----------------------------------------------------------------------------
bool ccb_p32_conn(GUI_MENU_ENTRY *ptr)
{
	Settings->PMD32->connected = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_PERIPHERALS;

	while ((++ptr)->type != MENU_END)
		if (ptr->detail)
			ptr->detail(ptr);

	return false;
}
//-----------------------------------------------------------------------------
bool ccb_p32_extc(GUI_MENU_ENTRY *ptr)
{
	Settings->PMD32->extraCommands = (ptr->state = !ptr->state);
	GUI->uiSetChanges |= PS_PERIPHERALS;

	ptr++;
	ptr->enabled = Settings->PMD32->extraCommands;
	return false;
}
//-----------------------------------------------------------------------------
bool ccb_tapebrowser(GUI_MENU_ENTRY *ptr)
{
	GUI->uiCallback.connect(Emulator, &TEmulator::ActionTapeBrowser);
	GUI->uiSetChanges |= PS_CLOSEALL;
	return true;
}
//-----------------------------------------------------------------------------
bool ccb_exit(GUI_MENU_ENTRY *ptr)
{
	Emulator->ActionExit();
	return true;
}
//-----------------------------------------------------------------------------
