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
static const char *uicch = new char[32];
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
const char *dcb_emu_m3cmp_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_pause_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_speed_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_focus_state(GUI_MENU_ENTRY *ptr);
const char *dcb_emu_asave_state(GUI_MENU_ENTRY *ptr);
const char *dcb_machine_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_file_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_rmod_state(GUI_MENU_ENTRY *ptr);
const char *dcb_mem_rpkg_state(GUI_MENU_ENTRY *ptr);
const char *dcb_rom_pckg_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_conn_state(GUI_MENU_ENTRY *ptr);
const char *dcb_p32_file_state(GUI_MENU_ENTRY *ptr);
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
bool ccb_emu_m3cmp(GUI_MENU_ENTRY *ptr);
bool ccb_emu_focus(GUI_MENU_ENTRY *ptr);
bool ccb_emu_asave(GUI_MENU_ENTRY *ptr);
bool ccb_emu_saves(GUI_MENU_ENTRY *ptr);
bool ccb_machine(GUI_MENU_ENTRY *ptr);
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
	{ MI_STANDARD, "\aYES", NULL, SDLK_y, NULL, NULL, NULL, true, true, GUI_QUERY_YES },
	{ MI_STANDARD, "\aNO", NULL, SDLK_n, NULL, NULL, NULL, true, true, (WORD) GUI_QUERY_CANCEL },
	{ MENU_END }
};
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_query_save[] = {
	{ MI_TITLE },
	{ MI_STANDARD, "DO\aN'T SAVE", NULL, SDLK_n, NULL, NULL, NULL, true, true, GUI_QUERY_DONTSAVE },
	{ MI_STANDARD, "\aSAVE", NULL, SDLK_s, NULL, NULL, NULL, true, true, GUI_QUERY_SAVE },
	{ MI_STANDARD, "\aCANCEL", NULL, SDLK_c, NULL, NULL, NULL, true, true, (WORD) GUI_QUERY_CANCEL },
	{ MENU_END }
};
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_tapebrowser_popup[] = {
	{ MI_TITLE, "TAPE BLOCK OPTIONS" },
	{ MI_STANDARD, "SET \aCURSOR", "SPACE", SDLK_c, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDLK_SPACE },
	{ MI_STANDARD, "SET \aSTOP-CURSOR", "^END", SDLK_s, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDLK_END | KM_SHIFT },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "TO\aGGLE SELECT", "INS", SDLK_g, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDLK_INSERT },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "MOVE BLOCK \aUP", "^\201", SDLK_u, NULL, ccb_tape_command, dcb_tape_contblk_state, false, false, SDLK_UP | KM_SHIFT },
	{ MI_STANDARD, "MOVE BLOCK DOW\aN", "^\202", SDLK_n, NULL, ccb_tape_command, dcb_tape_contblk_state, false, false, SDLK_DOWN | KM_SHIFT },
	{ MI_STANDARD, "\aDELETE BLOCK(S)", "^DEL", SDLK_d, NULL, ccb_tape_command, dcb_tape_noempty_state, false, false, SDLK_DELETE | KM_SHIFT },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "EDIT \aHEADER", "TAB", SDLK_h, NULL, ccb_tape_command, NULL, false, false, SDLK_TAB },
	{ MI_STANDARD, "\aFIX HEADER CRC", NULL, SDLK_f, NULL, NULL, NULL, false },
	{ MI_DIALOG, "MAKE HEAD(\aLESS)", NULL, SDLK_l, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "\aIMPORT TAPE", NULL, SDLK_i, NULL, ccb_fileselector, NULL, true, false, 8 },
	{ MI_DIALOG, "IMPORT \aRAW DATA", NULL, SDLK_r, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "E\aXPORT TO TAPE", NULL, SDLK_x, NULL, NULL, NULL, false /* dcb_tape_contblk_state, false, false, 0 */ },
	{ MI_DIALOG, "EXPORT RA\aW DATA", NULL, SDLK_w, NULL, NULL, NULL, false /* dcb_tape_noempty_state */ },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_p32_images_menu[] = {
	{ MI_TITLE, "PMD 32 DISK IMAGES" },
	{ MI_CHECKBOX, "DRIVE \aA:", NULL, SDLK_a, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8001 },
	{ MI_CHECKBOX, "DRIVE \aB:", NULL, SDLK_b, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8002 },
	{ MI_CHECKBOX, "DRIVE \aC:", NULL, SDLK_c, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8003 },
	{ MI_CHECKBOX, "DRIVE \aD:", NULL, SDLK_d, NULL, ccb_p32_imgd, dcb_p32_imgd_state, true, false, 0x8004 },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_memblock_read_menu[] = {
	{ MI_TITLE, "READ MEMORY BLOCK" },
	{ MI_DIALOG, "\aFILE", NULL, SDLK_f, NULL, ccb_fileselector, dcb_blk_file_state, true, false, 7 },
	{ MI_SEPARATOR },
	{ MI_VALUE, "\aSTART ADDRESS", NULL, SDLK_s, NULL, ccb_blk_strt, dcb_blk_strt_state, true },
	{ MI_VALUE, "\aLENGTH", NULL, SDLK_l, NULL, ccb_blk_leng, dcb_blk_leng_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aHEX VALUES", NULL, SDLK_h, NULL, ccb_blk_hexa, dcb_blk_hexa_state, true },
	{ MI_CHECKBOX, "\aC\215\216\217 REMAP", NULL, SDLK_b, NULL, ccb_blk_rmap, dcb_blk_rmap_state },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "\aOK", NULL, SDLK_o, NULL, ccb_blk_exec, NULL, true, false },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_memblock_write_menu[] = {
	{ MI_TITLE, "WRITE MEMORY BLOCK" },
	{ MI_DIALOG, "\aFILE", NULL, SDLK_f, NULL, ccb_fileselector, dcb_blk_file_state, true, true, 7 },
	{ MI_SEPARATOR },
	{ MI_VALUE, "\aSTART ADDRESS", NULL, SDLK_s, NULL, ccb_blk_strt, dcb_blk_strt_state, true },
	{ MI_VALUE, "\aLENGTH", NULL, SDLK_l, NULL, ccb_blk_leng, dcb_blk_leng_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aHEX VALUES", NULL, SDLK_h, NULL, ccb_blk_hexa, dcb_blk_hexa_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aROM ACCESS", NULL, SDLK_r, NULL, ccb_blk_roma, dcb_blk_roma_state },
	{ MI_CHECKBOX, "\aC\215\216\217 REMAP", NULL, SDLK_b, NULL, ccb_blk_rmap, dcb_blk_rmap_state },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "\aOK", NULL, SDLK_o, NULL, ccb_blk_exec, NULL, true, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_file_menu[] = {
	{ MI_TITLE, "FILE" },
	{ MI_STANDARD, "\aNEW TAPE", NULL, SDLK_n, NULL, ccb_tape_new, NULL, true, false, 0 },
	{ MI_DIALOG, "OPEN \aTAPE", "F2", SDLK_t, NULL, ccb_fileselector, NULL, true, false, 1 },
	{ MI_DIALOG, "SAVE T\aAPE", "^F2", SDLK_a, NULL, ccb_fileselector, dcb_tape_save_state, false, false, 2 },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "OPEN \aSNAPSHOT", "F7", SDLK_s, NULL, ccb_fileselector, NULL, true, false, 3 },
	{ MI_DIALOG, "\aCREATE SNAPSHOT", "^F7", SDLK_c, NULL, ccb_fileselector, NULL, true, false, 4 },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aDISK IMAGES", "F6", SDLK_d, gui_p32_images_menu, NULL, dcb_p32_imgs_state, false },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aLOAD MEMORY", "F11", SDLK_l, gui_memblock_read_menu, NULL, NULL, true },
	{ MI_SUBMENU, "SAVE \aMEMORY", "^F11", SDLK_m, gui_memblock_write_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "SAVE SCREENS\aHOT", NULL, SDLK_h, NULL, NULL, NULL, false },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_size_menu[] = {
	{ MI_TITLE, "SCREEN SIZE" },
	{ MI_RADIO, "\a100%", "1", SDLK_1, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_NORMAL },
	{ MI_RADIO, "\a200%", "2", SDLK_2, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_DOUBLESIZE },
	{ MI_RADIO, "\a300%", "3", SDLK_3, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_TRIPLESIZE },
	{ MI_RADIO, "\a400%", "4", SDLK_4, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_QUADRUPLESIZE },
	{ MI_RADIO, "\aFULLSCREEN", "F", SDLK_f, NULL, ccb_view_size, dcb_view_size_state, true, false, DM_FULLSCREEN },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_cmode_menu[] = {
	{ MI_TITLE, "COLOR MODE" },
	{ MI_RADIO, "\aMONOCHROMATIC", "M", SDLK_m, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_MONO },
	{ MI_RADIO, "\aSTANDARD", "M", SDLK_s, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_STANDARD },
	{ MI_RADIO, "\aCOLOR", "C", SDLK_c, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_COLOR },
	{ MI_RADIO, "COLOR\aACE\220", "C", SDLK_a, NULL, ccb_view_cmod, dcb_view_cmod_state, true, false, CP_MULTICOLOR },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_cpal_menu[] = {
	{ MI_TITLE, "COLOR PALETTE" },
	{ MI_RADIO, "\aVIDEO", NULL, SDLK_v, NULL, ccb_view_cpal, dcb_view_cpal_state, true, false, CL_VIDEO },
	{ MI_RADIO, "\aRGB", NULL, SDLK_r, NULL, ccb_view_cpal, dcb_view_cpal_state, true, false, CL_RGB },
	{ MI_RADIO, "\aCUSTOM", NULL, SDLK_c, NULL, ccb_view_cpal, dcb_view_cpal_state, true, false, CL_DEFINED },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_view_colors_menu[] = {
	{ MI_TITLE, "CUSTOM COLORS" },
	{ MI_DIALOG, "ATTRIBUTE 00", NULL, SDLK_1, NULL, NULL, NULL, true },
	{ MI_DIALOG, "ATTRIBUTE 01", NULL, SDLK_2, NULL, NULL, NULL, true },
	{ MI_DIALOG, "ATTRIBUTE 10", NULL, SDLK_3, NULL, NULL, NULL, true },
	{ MI_DIALOG, "ATTRIBUTE 10", NULL, SDLK_4, NULL, NULL, NULL, true },
	{ MENU_END }
};
#ifndef OPENGL
static GUI_MENU_ENTRY gui_view_scaler_menu[] = {
	{ MI_TITLE, "SCALER" },
	{ MI_RADIO, "\aLCD EMULATION\220", "5", SDLK_l, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, (WORD) -1 },
	{ MI_RADIO, "HALFPASS \a0%", "6", SDLK_0, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_0 },
	{ MI_RADIO, "HALFPASS \a25%", "7", SDLK_2, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_25 },
	{ MI_RADIO, "HALFPASS \a50%", "8", SDLK_5, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_50 },
	{ MI_RADIO, "HALFPASS \a75%", "9", SDLK_7, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_75 },
	{ MI_RADIO, "\aPIXEL PRECISE", "0", SDLK_p, NULL, ccb_view_sclr, dcb_view_sclr_state, true, false, HP_OFF },
	{ MENU_END }
};
#endif
static GUI_MENU_ENTRY gui_view_menu[] = {
	{ MI_TITLE, "DISPLAY" },
	{ MI_SUBMENU, "\aSCREEN SIZE", NULL, SDLK_s, gui_view_size_menu, NULL, NULL, true },
	{ MI_VALUE, "\aBORDER SIZE", NULL, SDLK_b, NULL, ccb_view_brdr, dcb_view_brdr_state, true },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "COLOR \aMODE", NULL, SDLK_m, gui_view_cmode_menu, NULL, NULL, true },
	{ MI_SUBMENU, "COLOR \aPALETTE", NULL, SDLK_p, gui_view_cpal_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aCUSTOM COLORS", NULL, SDLK_c, gui_view_colors_menu, NULL, dcb_view_ccol_state },
#ifndef OPENGL
	{ MI_SUBMENU, "SC\aALER", NULL, SDLK_a, gui_view_scaler_menu, NULL, NULL, true },
#endif
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_emu_sound_menu[] = {
	{ MI_TITLE, "SOUND" },
	{ MI_CHECKBOX, "\aMUTE", "F8", SDLK_m, NULL, ccb_snd_mute, dcb_snd_mute_state, true },
	{ MI_VALUE, "\aVOLUME", NULL, SDLK_v, NULL, ccb_snd_volume, dcb_snd_volume_state },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_emu_kbd_menu[] = {
	{ MI_TITLE, "KEYBOARD" },
	{ MI_CHECKBOX, "E\aXCHANGE Z/Y KEYS", NULL, SDLK_x, NULL, ccb_kbd_xchg, dcb_kbd_xchg_state, true },
	{ MI_CHECKBOX, "USE \aNUMERIC KEYPAD", NULL, SDLK_n, NULL, ccb_kbd_nums, dcb_kbd_nums_state, true },
	{ MI_CHECKBOX, "EXTENDED \aCTRL KEYS", NULL, SDLK_c, NULL, ccb_kbd_mato, dcb_kbd_mato_state, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_emu_menu[] = {
	{ MI_TITLE, "EMULATOR" },
	{ MI_VALUE, "\aEMULATION SPEED", NULL, SDLK_e, NULL, ccb_emu_speed, dcb_emu_speed_state, true },
	{ MI_CHECKBOX, "\aPAUSE", "F3", SDLK_p, NULL, ccb_emu_pause, dcb_emu_pause_state, true },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "\aRESET", "F5", SDLK_r, NULL, ccb_emu_reset, NULL, true },
	{ MI_STANDARD, "\aCOLD RESTART", "^F5", SDLK_c, NULL, ccb_emu_hardreset, NULL, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "PMD 85-\a3 COMPATIBILITY MODE", NULL, SDLK_3, NULL, ccb_emu_m3cmp, dcb_emu_m3cmp_state },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aSOUND", NULL, SDLK_s, gui_emu_sound_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aKEYBOARD", NULL, SDLK_k, gui_emu_kbd_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "PAUSE ON LOST \aFOCUS", NULL, SDLK_f, NULL, ccb_emu_focus, dcb_emu_focus_state, true },
	{ MI_CHECKBOX, "\aAUTO-SAVE SETTINGS", NULL, SDLK_a, NULL, ccb_emu_asave, dcb_emu_asave_state, true },
	{ MI_SEPARATOR },
	{ MI_STANDARD, "SA\aVE SETTINGS", NULL, SDLK_v, NULL, ccb_emu_saves, NULL, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_machine_menu[] = {
	{ MI_TITLE, "MACHINE" },
	{ MI_RADIO, "PMD 85-\a1", NULL, SDLK_1, NULL, ccb_machine, dcb_machine_state, true, false, CM_V1 },
	{ MI_RADIO, "PMD 85-\a2", NULL, SDLK_2, NULL, ccb_machine, dcb_machine_state, true, false, CM_V2 },
	{ MI_RADIO, "PMD 85-2\aA", NULL, SDLK_a, NULL, ccb_machine, dcb_machine_state, true, false, CM_V2A },
	{ MI_RADIO, "PMD 85-\a3", NULL, SDLK_3, NULL, ccb_machine, dcb_machine_state, true, false, CM_V3 },
	{ MI_SEPARATOR },
	{ MI_RADIO, "\aDidaktik Alfa", NULL, SDLK_d, NULL, ccb_machine, dcb_machine_state, true, false, CM_ALFA },
	{ MI_RADIO, "D\aidaktik Alfa 2", NULL, SDLK_i, NULL, ccb_machine, dcb_machine_state, true, false, CM_ALFA2 },
	{ MI_SEPARATOR },
	{ MI_RADIO, "\aConsul 2717", NULL, SDLK_c, NULL, ccb_machine, dcb_machine_state, true, false, CM_C2717 },
	{ MI_SEPARATOR },
	{ MI_RADIO, "\aMa\213o", NULL, SDLK_m, NULL, ccb_machine, dcb_machine_state, true, false, CM_MATO },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_mem_menu[] = {
	{ MI_TITLE, "MEMORY" },
	{ MI_DIALOG, "\aMONITOR FILE", NULL, SDLK_m, NULL, ccb_fileselector, dcb_mem_file_state, true, false, 5 },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aROM MODULE", NULL, SDLK_r, NULL, ccb_mem_rmod, dcb_mem_rmod_state, true },
	{ MI_SUBMENU, "ROM \aPACKAGE", NULL, SDLK_p, NULL, NULL, dcb_mem_rpkg_state, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_p32_menu[] = {
	{ MI_TITLE, "DISK DRIVE PMD 32" },
	{ MI_CHECKBOX, "\aCONNECTED", NULL, SDLK_c, NULL, ccb_p32_conn, dcb_p32_conn_state, true },
	{ MI_DIALOG, "\aROM FILE", NULL, SDLK_r, NULL, ccb_fileselector, dcb_p32_file_state, true, false, 6 },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "\aDISK IMAGES", "F6", SDLK_d, gui_p32_images_menu, NULL, dcb_p32_imgs_state, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "\aEXTENDED COMMANDS", NULL, SDLK_e, NULL, ccb_p32_extc, dcb_p32_extc_state, true },
	{ MI_DIALOG, "VIRTUAL \aSD-CARD", NULL, SDLK_s, NULL, NULL, dcb_p32_sdcd_state, true },
	{ MENU_END }
};
static GUI_MENU_ENTRY gui_pers_menu[] = {
	{ MI_TITLE, "PERIPHERALS" },
	{ MI_SUBMENU, "\aJOYSTICK 4004/482", NULL, SDLK_j, NULL, NULL, NULL, false },
	{ MI_SUBMENU, "\aMOUSE", NULL, SDLK_m, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_SUBMENU, "PMD \a32", NULL, SDLK_3, gui_p32_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_CHECKBOX, "M\aIF 85", NULL, SDLK_i, NULL, NULL, NULL, false },
	{ MENU_END }
};
static GUI_MENU_ENTRY UNUSED_VARIABLE gui_main_menu[] = {
	{ MI_TITLE, "MAIN MENU" },
	{ MI_SUBMENU, "\aFILE", NULL, SDLK_f, gui_file_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aDISPLAY", NULL, SDLK_d, gui_view_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aEMULATOR", NULL, SDLK_e, gui_emu_menu, NULL, NULL, true },
	{ MI_SUBMENU, "MA\aCHINE", "F9", SDLK_c, gui_machine_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aMEMORY", "^F9", SDLK_m, gui_mem_menu, NULL, NULL, true },
	{ MI_SUBMENU, "\aPERIPHERALS", "F10", SDLK_p, gui_pers_menu, NULL, NULL, true },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "\aTAPE BROWSER", "T", SDLK_t, NULL, ccb_tapebrowser, NULL, true },
	{ MI_DIALOG, "DEBU\aGGER", "F12", SDLK_g, NULL, ccb_debugger, NULL, true },
	{ MI_DIALOG, "P\aOKE", NULL, SDLK_o, NULL, NULL, NULL, false },
	{ MI_SEPARATOR },
	{ MI_DIALOG, "\aABOUT", "^F1", SDLK_a, NULL, NULL, NULL, false },
	{ MI_STANDARD, "E\aXIT", "F4", SDLK_x, NULL, ccb_exit, NULL, true },
	{ MENU_END }
};
//-----------------------------------------------------------------------------
#endif
