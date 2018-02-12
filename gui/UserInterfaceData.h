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
#ifndef USERINTERFACEDATA_H_
#define USERINTERFACEDATA_H_
//-----------------------------------------------------------------------------
#include "globals.h"
#include "UserInterface.h"
//-----------------------------------------------------------------------------
static const char UNUSED_VARIABLE *uicch = new char[32];
//-----------------------------------------------------------------------------
const char *dcb_tape_save_state(GUI_MENU_ENTRY *ptr);
const char *dcb_tape_noempty_state(GUI_MENU_ENTRY *ptr);
const char *dcb_tape_contblk_state(GUI_MENU_ENTRY *ptr);
const char *dcb_view_size_state(GUI_MENU_ENTRY *ptr);
const char *dcb_view_cmod_state(GUI_MENU_ENTRY *ptr);
const char *dcb_view_cpal_state(GUI_MENU_ENTRY *ptr);
const char *dcb_view_ccol_state(GUI_MENU_ENTRY *ptr);
const char *dcb_view_sclr_state(GUI_MENU_ENTRY *ptr);
const char *dcb_view_brdr_state(GUI_MENU_ENTRY *ptr);
const char *dcb_snd_mute_state(GUI_MENU_ENTRY *ptr);
const char *dcb_snd_volume_state(GUI_MENU_ENTRY *ptr);
const char *dcb_kbd_xchg_state(GUI_MENU_ENTRY *ptr);
const char *dcb_kbd_nums_state(GUI_MENU_ENTRY *ptr);
const char *dcb_kbd_mato_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_pause_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_speed_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_focus_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_asave_state(GUI_MENU_ENTRY *ptr);
const char *dcb_machine_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_file_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_rmod_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_rpkg_state(GUI_MENU_ENTRY *ptr);
const char *dcb_rom_pckg_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_x256k_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_m3cmp_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_spl8k_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_conn_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_imgs_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_extc_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_sdcd_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_imgd_state(GUI_MENU_ENTRY *ptr);
const char *dcb_blk_file_state(GUI_MENU_ENTRY *ptr);
const char *dcb_blk_strt_state(GUI_MENU_ENTRY *ptr);
const char *dcb_blk_leng_state(GUI_MENU_ENTRY *ptr);
const char *dcb_blk_hexa_state(GUI_MENU_ENTRY *ptr);
const char *dcb_blk_roma_state(GUI_MENU_ENTRY *ptr);
const char *dcb_blk_rmap_state(GUI_MENU_ENTRY *ptr);
//-----------------------------------------------------------------------------
bool ccb_tape_command(GUI_MENU_ENTRY *ptr);
bool ccb_tape_new(GUI_MENU_ENTRY *ptr);
bool ccb_fileselector(GUI_MENU_ENTRY *ptr);
bool ccb_view_size(GUI_MENU_ENTRY *ptr);
bool ccb_view_brdr(GUI_MENU_ENTRY *ptr);
bool ccb_view_cmod(GUI_MENU_ENTRY *ptr);
bool ccb_view_cpal(GUI_MENU_ENTRY *ptr);
bool ccb_view_sclr(GUI_MENU_ENTRY *ptr);
bool ccb_snd_mute(GUI_MENU_ENTRY *ptr);
bool ccb_snd_volume(GUI_MENU_ENTRY *ptr);
bool ccb_kbd_xchg(GUI_MENU_ENTRY *ptr);
bool ccb_kbd_nums(GUI_MENU_ENTRY *ptr);
bool ccb_kbd_mato(GUI_MENU_ENTRY *ptr);
bool ccb_emu_pause(GUI_MENU_ENTRY *ptr);
bool ccb_emu_speed(GUI_MENU_ENTRY *ptr);
bool ccb_emu_reset(GUI_MENU_ENTRY *ptr);
bool ccb_emu_hardreset(GUI_MENU_ENTRY *ptr);
bool ccb_emu_focus(GUI_MENU_ENTRY *ptr);
bool ccb_emu_asave(GUI_MENU_ENTRY *ptr);
bool ccb_emu_saves(GUI_MENU_ENTRY *ptr);
bool ccb_machine(GUI_MENU_ENTRY *ptr);
bool ccb_mem_x256k(GUI_MENU_ENTRY *ptr);
bool ccb_mem_m3cmp(GUI_MENU_ENTRY *ptr);
bool ccb_mem_spl8k(GUI_MENU_ENTRY *ptr);
bool ccb_mem_rmod(GUI_MENU_ENTRY *ptr);
bool ccb_rom_pckg(GUI_MENU_ENTRY *ptr);
bool ccb_p32_imgd(GUI_MENU_ENTRY *ptr);
bool ccb_p32_conn(GUI_MENU_ENTRY *ptr);
bool ccb_p32_extc(GUI_MENU_ENTRY *ptr);
bool ccb_blk_strt(GUI_MENU_ENTRY *ptr);
bool ccb_blk_leng(GUI_MENU_ENTRY *ptr);
bool ccb_blk_hexa(GUI_MENU_ENTRY *ptr);
bool ccb_blk_roma(GUI_MENU_ENTRY *ptr);
bool ccb_blk_rmap(GUI_MENU_ENTRY *ptr);
bool ccb_blk_exec(GUI_MENU_ENTRY *ptr);
bool ccb_tapebrowser(GUI_MENU_ENTRY *ptr);
bool ccb_debugger(GUI_MENU_ENTRY *ptr);
bool ccb_exit(GUI_MENU_ENTRY *ptr);
//-----------------------------------------------------------------------------
static GUI_MENU_ENTRY UNUSED_VARIABLE *gui_rom_packages = NULL;
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_query_confirm[] = {
	{ MI_TITLE },
	{ MI_STANDARD, "\aYES", NULL, SDL_SCANCODE_Y, NULL, NULL, NULL, true, true, GUI_QUERY_YES },
	{ MI_STANDARD, "\aNO", NULL, SDL_SCANCODE_N, NULL, NULL, NULL, true, true, (WORD) GUI_QUERY_CANCEL },
	{ MENU_END }
};
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_query_save[] = {
	{ MI_TITLE },
	{ MI_STANDARD, "DO\aN'T SAVE", NULL, SDL_SCANCODE_N, NULL, NULL, NULL, true, true, GUI_QUERY_DONTSAVE },
	{ MI_STANDARD, "\aSAVE", NULL, SDL_SCANCODE_S, NULL, NULL, NULL, true, true, GUI_QUERY_SAVE },
	{ MI_STANDARD, "\aCANCEL", NULL, SDL_SCANCODE_C, NULL, NULL, NULL, true, true, (WORD) GUI_QUERY_CANCEL },
	{ MENU_END }
};
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_tapebrowser_popup[] = {
	{ MI_TITLE, "TAPE BLOCK OPTIONS" },
	{ MI_STANDARD, "SET \aCURSOR", "SPACE", SDL_SCANCODE_C, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDL_SCANCODE_SPACE },
	{ MI_STANDARD, "SET \aSTOP-CURSOR", "^END", SDL_SCANCODE_S, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDL_SCANCODE_END | KM_SHIFT },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "TO\aGGLE SELECT", "INS", SDL_SCANCODE_G, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDL_SCANCODE_INSERT },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "MOVE BLOCK \aUP", "^\201", SDL_SCANCODE_U, NULL, ccb_tape_command, dcb_tape_contblk_state, false, false, SDL_SCANCODE_UP | KM_SHIFT },
	{ MI_STANDARD, "MOVE BLOCK DOW\aN", "^\202", SDL_SCANCODE_N, NULL, ccb_tape_command, dcb_tape_contblk_state, false, false, SDL_SCANCODE_DOWN | KM_SHIFT },
	{ MI_STANDARD, "\aDELETE BLOCK(S)", "^DEL", SDL_SCANCODE_D, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDL_SCANCODE_DELETE | KM_SHIFT },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "EDIT \aHEADER", "TAB", SDL_SCANCODE_H, NULL, ccb_tape_command, NULL, false, false, SDL_SCANCODE_TAB },
	{ MI_STANDARD, "\aFIX HEADER CRC", NULL, SDL_SCANCODE_F, NULL, NULL, NULL, false },
	{ MI_DIALOG, "MAKE HEAD(\aLESS)", NULL, SDL_SCANCODE_L, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "\aIMPORT TAPE", NULL, SDL_SCANCODE_I, NULL, ccb_fileselector, NULL, true, false, 8 },
	{ MI_DIALOG, "IMPORT \aRAW DATA", NULL, SDL_SCANCODE_R, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "E\aXPORT TO TAPE", NULL, SDL_SCANCODE_X, NULL, NULL, NULL, false /* dcb_tape_contblk_state, false, false, 0 */ },
	{ MI_DIALOG, "EXPORT RA\aW DATA", NULL, SDL_SCANCODE_W, NULL, NULL, NULL, false /* dcb_tape_noempty_state */ },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_p32_images_menu[] = {
	{ MI_TITLE, "PMD 32 DISK IMAGES" },
	{ MI_CHECKBOX, "DRIVE \aA:", NULL, SDL_SCANCODE_A, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8001 },
	{ MI_CHECKBOX, "DRIVE \aB:", NULL, SDL_SCANCODE_B, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8002 },
	{ MI_CHECKBOX, "DRIVE \aC:", NULL, SDL_SCANCODE_C, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8003 },
	{ MI_CHECKBOX, "DRIVE \aD:", NULL, SDL_SCANCODE_D, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8004 },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_memblock_read_menu[] = {
	{ MI_TITLE, "READ MEMORY BLOCK" },
	{ MI_DIALOG, "\aFILE", NULL, SDL_SCANCODE_F, NULL, ccb_fileselector, dcb_blk_file_state, true, false, 7 },
	{ MI_SEPARATOR },
	{ MI_VALUE, "\aSTART ADDRESS", NULL, SDL_SCANCODE_S, NULL, ccb_blk_strt, dcb_blk_strt_state, true },
	{ MI_VALUE, "\aLENGTH", NULL, SDL_SCANCODE_L, NULL, ccb_blk_leng, dcb_blk_leng_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aHEX VALUES", NULL, SDL_SCANCODE_H, NULL, ccb_blk_hexa, dcb_blk_hexa_state, true },
	{ MI_CHECKBOX, "\aC\215\216\217 REMAP", NULL, SDL_SCANCODE_B, NULL, ccb_blk_rmap, dcb_blk_rmap_state },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "\aOK", NULL, SDL_SCANCODE_O, NULL, ccb_blk_exec, NULL, true, false },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_memblock_write_menu[] = {
	{ MI_TITLE, "WRITE MEMORY BLOCK" },
	{ MI_DIALOG, "\aFILE", NULL, SDL_SCANCODE_F, NULL, ccb_fileselector, dcb_blk_file_state, true, true, 7 },
	{ MI_SEPARATOR },
	{ MI_VALUE, "\aSTART ADDRESS", NULL, SDL_SCANCODE_S, NULL, ccb_blk_strt, dcb_blk_strt_state, true },
	{ MI_VALUE, "\aLENGTH", NULL, SDL_SCANCODE_L, NULL, ccb_blk_leng, dcb_blk_leng_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aHEX VALUES", NULL, SDL_SCANCODE_H, NULL, ccb_blk_hexa, dcb_blk_hexa_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aROM ACCESS", NULL, SDL_SCANCODE_R, NULL, ccb_blk_roma, dcb_blk_roma_state },
	{ MI_CHECKBOX, "\aC\215\216\217 REMAP", NULL, SDL_SCANCODE_B, NULL, ccb_blk_rmap, dcb_blk_rmap_state },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "\aOK", NULL, SDL_SCANCODE_O, NULL, ccb_blk_exec, NULL, true, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_file_menu[] = {
	{ MI_TITLE, "FILE" },
	{ MI_STANDARD, "\aNEW TAPE", NULL, SDL_SCANCODE_N, NULL, ccb_tape_new, NULL, true, false, 0 },
	{ MI_DIALOG, "OPEN \aTAPE", "F2", SDL_SCANCODE_T, NULL, ccb_fileselector, NULL, true, false, 1 },
	{ MI_DIALOG, "SAVE T\aAPE", "^F2", SDL_SCANCODE_A, NULL, ccb_fileselector, dcb_tape_save_state, false, false, 2 },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "OPEN \aSNAPSHOT", "F7", SDL_SCANCODE_S, NULL, ccb_fileselector, NULL, true, false, 3 },
	{ MI_DIALOG, "\aCREATE SNAPSHOT", "^F7", SDL_SCANCODE_C, NULL, ccb_fileselector, NULL, true, false, 4 },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aDISK IMAGES", "F6", SDL_SCANCODE_D, gui_p32_images_menu, NULL, dcb_p32_imgs_state, false },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aLOAD MEMORY", "F11", SDL_SCANCODE_L, gui_memblock_read_menu, NULL, NULL, true },
	{ MI_SUBMENU, "SAVE \aMEMORY", "^F11", SDL_SCANCODE_M, gui_memblock_write_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "SAVE SCREENS\aHOT", NULL, SDL_SCANCODE_H, NULL, NULL, NULL, false },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_size_menu[] = {
	{ MI_TITLE, "SCREEN SIZE" },
	{ MI_RADIO, "\a100%", "1", SDL_SCANCODE_1, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_NORMAL },
	{ MI_RADIO, "\a200%", "2", SDL_SCANCODE_2, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_DOUBLESIZE },
	{ MI_RADIO, "\a300%", "3", SDL_SCANCODE_3, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_TRIPLESIZE },
	{ MI_RADIO, "\a400%", "4", SDL_SCANCODE_4, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_QUADRUPLESIZE },
	{ MI_RADIO, "\aFULLSCREEN", "F", SDL_SCANCODE_F, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_FULLSCREEN },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_cmode_menu[] = {
	{ MI_TITLE, "COLOR MODE" },
	{ MI_RADIO, "\aMONOCHROMATIC", "M", SDL_SCANCODE_M, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_MONO },
	{ MI_RADIO, "\aSTANDARD", "M", SDL_SCANCODE_S, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_STANDARD },
	{ MI_RADIO, "\aCOLOR", "C", SDL_SCANCODE_C, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_COLOR },
	{ MI_RADIO, "COLOR\aACE\220", "C", SDL_SCANCODE_A, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_COLORACE },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_cpal_menu[] = {
	{ MI_TITLE, "COLOR PALETTE" },
	{ MI_RADIO, "\aVIDEO", NULL, SDL_SCANCODE_V, NULL, ccb_view_cpal, dcb_view_cpal_state, true, false, CL_VIDEO },
	{ MI_RADIO, "\aRGB", NULL, SDL_SCANCODE_R, NULL, ccb_view_cpal, dcb_view_cpal_state, true, false, CL_RGB },
	{ MI_RADIO, "\aCUSTOM", NULL, SDL_SCANCODE_C, NULL, ccb_view_cpal, dcb_view_cpal_state, true, false, CL_DEFINED },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_colors_menu[] = {
	{ MI_TITLE, "CUSTOM COLORS" },
	{ MI_DIALOG, "ATTRIBUTE 00", NULL, SDL_SCANCODE_1, NULL, NULL, NULL, true },
	{ MI_DIALOG, "ATTRIBUTE 01", NULL, SDL_SCANCODE_2, NULL, NULL, NULL, true },
	{ MI_DIALOG, "ATTRIBUTE 10", NULL, SDL_SCANCODE_3, NULL, NULL, NULL, true },
	{ MI_DIALOG, "ATTRIBUTE 10", NULL, SDL_SCANCODE_4, NULL, NULL, NULL, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_scanliner_menu[] = {
	{ MI_TITLE, "SCANLINER" },
	{ MI_RADIO, "\aLCD EMULATION\220", "5", SDL_SCANCODE_L, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, (WORD) -1 },
	{ MI_RADIO, "HALFPASS \a0%", "6", SDL_SCANCODE_0, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_0 },
	{ MI_RADIO, "HALFPASS \a25%", "7", SDL_SCANCODE_2, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_25 },
	{ MI_RADIO, "HALFPASS \a50%", "8", SDL_SCANCODE_5, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_50 },
	{ MI_RADIO, "HALFPASS \a75%", "9", SDL_SCANCODE_7, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_75 },
	{ MI_RADIO, "\aPIXEL PRECISE", "0", SDL_SCANCODE_P, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_OFF },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_menu[] = {
	{ MI_TITLE, "DISPLAY" },
	{ MI_SUBMENU, "\aSCREEN SIZE", NULL, SDL_SCANCODE_S, gui_view_size_menu, NULL, NULL, true },
	{ MI_VALUE, "\aBORDER SIZE", NULL, SDL_SCANCODE_B, NULL, ccb_view_brdr, dcb_view_brdr_state, true },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "COLOR \aMODE", NULL, SDL_SCANCODE_M, gui_view_cmode_menu, NULL, NULL, true },
	{ MI_SUBMENU, "COLOR \aPALETTE", NULL, SDL_SCANCODE_P, gui_view_cpal_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aCUSTOM COLORS", NULL, SDL_SCANCODE_C, gui_view_colors_menu, NULL, dcb_view_ccol_state },
	{ MI_SUBMENU, "SC\aANLINER", NULL, SDL_SCANCODE_A, gui_view_scanliner_menu, NULL, NULL, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_emu_sound_menu[] = {
	{ MI_TITLE, "SOUND" },
	{ MI_CHECKBOX, "\aMUTE", "F8", SDL_SCANCODE_M, NULL, ccb_snd_mute, dcb_snd_mute_state, true },
	{ MI_VALUE, "\aVOLUME", NULL, SDL_SCANCODE_V, NULL, ccb_snd_volume, dcb_snd_volume_state },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_emu_kbd_menu[] = {
	{ MI_TITLE, "KEYBOARD" },
	{ MI_CHECKBOX, "E\aXCHANGE Z/Y KEYS", NULL, SDL_SCANCODE_X, NULL, ccb_kbd_xchg, dcb_kbd_xchg_state, true },
	{ MI_CHECKBOX, "USE \aNUMERIC KEYPAD", NULL, SDL_SCANCODE_N, NULL, ccb_kbd_nums, dcb_kbd_nums_state, true },
	{ MI_CHECKBOX, "EXTENDED \aCTRL KEYS", NULL, SDL_SCANCODE_C, NULL, ccb_kbd_mato, dcb_kbd_mato_state, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_emu_menu[] = {
	{ MI_TITLE, "EMULATOR" },
	{ MI_VALUE, "\aEMULATION SPEED", NULL, SDL_SCANCODE_E, NULL, ccb_emu_speed, dcb_emu_speed_state, true },
	{ MI_CHECKBOX, "\aPAUSE", "F3", SDL_SCANCODE_P, NULL, ccb_emu_pause, dcb_emu_pause_state, true },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "\aRESET", "F5", SDL_SCANCODE_R, NULL, ccb_emu_reset, NULL, true },
	{ MI_STANDARD, "\aCOLD RESTART", "^F5", SDL_SCANCODE_C, NULL, ccb_emu_hardreset, NULL, true },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aSOUND", NULL, SDL_SCANCODE_S, gui_emu_sound_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aKEYBOARD", NULL, SDL_SCANCODE_K, gui_emu_kbd_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "PAUSE ON LOST \aFOCUS", NULL, SDL_SCANCODE_F, NULL, ccb_emu_focus, dcb_emu_focus_state, true },
	{ MI_CHECKBOX, "\aAUTO-SAVE SETTINGS", NULL, SDL_SCANCODE_A, NULL, ccb_emu_asave, dcb_emu_asave_state, true },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "SA\aVE SETTINGS", NULL, SDL_SCANCODE_V, NULL, ccb_emu_saves, NULL, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_machine_menu[] = {
	{ MI_TITLE, "MACHINE" },
	{ MI_RADIO, "PMD 85-\a1", NULL, SDL_SCANCODE_1, NULL, ccb_machine, dcb_machine_state, true, false, CM_V1 },
	{ MI_RADIO, "PMD 85-\a2", NULL, SDL_SCANCODE_2, NULL, ccb_machine, dcb_machine_state, true, false, CM_V2 },
	{ MI_RADIO, "PMD 85-2\aA", NULL, SDL_SCANCODE_A, NULL, ccb_machine, dcb_machine_state, true, false, CM_V2A },
	{ MI_RADIO, "PMD 85-\a3", NULL, SDL_SCANCODE_3, NULL, ccb_machine, dcb_machine_state, true, false, CM_V3 },
	{ MI_SEPARATOR },
	{ MI_RADIO, "\aDidaktik Alfa", NULL, SDL_SCANCODE_D, NULL, ccb_machine, dcb_machine_state, true, false, CM_ALFA },
	{ MI_RADIO, "D\aidaktik Alfa 2", NULL, SDL_SCANCODE_I, NULL, ccb_machine, dcb_machine_state, true, false, CM_ALFA2 },
	{ MI_SEPARATOR },
	{ MI_RADIO, "\aConsul 2717", NULL, SDL_SCANCODE_C, NULL, ccb_machine, dcb_machine_state, true, false, CM_C2717 },
	{ MI_SEPARATOR },
	{ MI_RADIO, "\aMa\213o", NULL, SDL_SCANCODE_M, NULL, ccb_machine, dcb_machine_state, true, false, CM_MATO },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_mem_menu[] = {
	{ MI_TITLE, "MEMORY" },
	{ MI_DIALOG, "\aMONITOR FILE", NULL, SDL_SCANCODE_M, NULL, ccb_fileselector, dcb_mem_file_state, true, false, 5 },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aROM MODULE", NULL, SDL_SCANCODE_R, NULL, ccb_mem_rmod, dcb_mem_rmod_state, true },
	{ MI_SUBMENU, "ROM \aPACKAGE", NULL, SDL_SCANCODE_P, NULL, NULL, dcb_mem_rpkg_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "256k MEMORY E\aXPANSION", NULL, SDL_SCANCODE_X, NULL, ccb_mem_x256k, dcb_mem_x256k_state },
	{ MI_CHECKBOX, "\aCOMPATIBILITY MODE", NULL, SDL_SCANCODE_C, NULL, ccb_mem_m3cmp, dcb_mem_m3cmp_state },
	{ MI_CHECKBOX, "\aSPLIT 8kB ROM", NULL, SDL_SCANCODE_S, NULL, ccb_mem_spl8k, dcb_mem_spl8k_state },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_p32_menu[] = {
	{ MI_TITLE, "DISK DRIVE PMD 32" },
	{ MI_CHECKBOX, "\aCONNECTED", NULL, SDL_SCANCODE_C, NULL, ccb_p32_conn, dcb_p32_conn_state, true },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aDISK IMAGES", "F6", SDL_SCANCODE_D, gui_p32_images_menu, NULL, dcb_p32_imgs_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aEXTENDED COMMANDS", NULL, SDL_SCANCODE_E, NULL, ccb_p32_extc, dcb_p32_extc_state, true },
	{ MI_DIALOG, "VIRTUAL \aSD-CARD", NULL, SDL_SCANCODE_S, NULL, NULL, dcb_p32_sdcd_state, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_pers_menu[] = {
	{ MI_TITLE, "PERIPHERALS" },
	{ MI_SUBMENU, "\aJOYSTICK 4004/482", NULL, SDL_SCANCODE_J, NULL, NULL, NULL, false },
	{ MI_SUBMENU, "\aMOUSE", NULL, SDL_SCANCODE_M, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "PMD \a32", NULL, SDL_SCANCODE_3, gui_p32_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "M\aIF 85", NULL, SDL_SCANCODE_I, NULL, NULL, NULL, false },
	{ MENU_END }
};
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_main_menu[] = {
	{ MI_TITLE, "MAIN MENU" },
	{ MI_SUBMENU, "\aFILE", NULL, SDL_SCANCODE_F, gui_file_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aDISPLAY", NULL, SDL_SCANCODE_D, gui_view_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aEMULATOR", NULL, SDL_SCANCODE_E, gui_emu_menu, NULL, NULL, true },
	{ MI_SUBMENU, "MA\aCHINE", "F9", SDL_SCANCODE_C, gui_machine_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aMEMORY", "^F9", SDL_SCANCODE_M, gui_mem_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aPERIPHERALS", "F10", SDL_SCANCODE_P, gui_pers_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "\aTAPE BROWSER", "T", SDL_SCANCODE_T, NULL, ccb_tapebrowser, NULL, true },
	{ MI_DIALOG, "DEBU\aGGER", "F12", SDL_SCANCODE_G, NULL, ccb_debugger, NULL, true },
	{ MI_DIALOG, "P\aOKE", NULL, SDL_SCANCODE_O, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "\aABOUT", "^F1", SDL_SCANCODE_A, NULL, NULL, NULL, false },
	{ MI_STANDARD, "E\aXIT", "F4", SDL_SCANCODE_X, NULL, ccb_exit, NULL, true },
	{ MENU_END }
};
//-----------------------------------------------------------------------------
#endif
