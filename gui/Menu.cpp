/*	Menu.cpp: Part of GUI rendering class: Menu drawing and handling
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
#include "UserInterface.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
void UserInterface::drawMenu(void *data)
{
	GUI_MENU_ENTRY *ptr;
	int widthLeft = 0, widthRight = 0, height = 0, i, k;
	const char *wrk;

	cMenu_leftMargin = 0;
	cMenu_data = (GUI_MENU_ENTRY *) data;

	for (i = 0, ptr = &cMenu_data[1]; ptr->type != MENU_END; i++, ptr++) {
		if (i == cMenu_hilite && (ptr->type == MI_SEPARATOR || ptr->type == MI_FIXED))
			cMenu_hilite++;

		if (ptr->type == MI_SEPARATOR) {
			height += GUI_CONST_SEPARATOR;
			continue;
		}
		else if (ptr->type == MI_CHECKBOX || ptr->type == MI_RADIO)
			cMenu_leftMargin = GUI_CONST_CHK_MARGIN;

		height += GUI_CONST_ITEM_SIZE;

		k = (strlen(ptr->text) * fontWidth)
		  + ((ptr->type == MI_SUBMENU) ? (fontWidth + fontWidth) :
		  + ((ptr->type == MI_DIALOG)  ? fontWidth : 0));

		if (widthLeft < k)
			widthLeft = k;

		k = 0;
		if ((wrk = ptr->hotkey) != NULL) {
			if (wrk[0] == '^') {
				k += GUI_CONST_HOTKEYCHAR;
				wrk++;
			}

			k += (strlen(wrk) * fontWidth) +
				(menuStack[menuStackLevel].type != GUI_TYPE_TAPE_POPUP) ? GUI_CONST_HOTKEYCHAR : 0;
		}

		if (ptr->detail) {
			wrk = ptr->detail(ptr);
			if (wrk)
				k += ((strlen(wrk) + 2) * fontWidth);
		}

		if (widthRight < k)
			widthRight = k;
	}

	cMenu_count = (ptr - &cMenu_data[1]);

	k = cMenu_leftMargin + widthLeft + fontWidth + widthRight;
	if ((int) (strlen(cMenu_data->text) * fontWidth) > k)
		k = strlen(cMenu_data->text) * fontWidth;

	cMenu_rect->w = GUI_CONST_BORDER + k + GUI_CONST_BORDER;
	cMenu_rect->h = (2 * GUI_CONST_BORDER) + height + GUI_CONST_BORDER;
	cMenu_rect->x = (defaultSurface->w - cMenu_rect->w) / 2;
	cMenu_rect->y = (defaultSurface->h - cMenu_rect->h) / 2;

	if (menuStack[menuStackLevel].type == GUI_TYPE_TAPE_POPUP)
		cMenu_rect->x *= 2;

	drawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y, cMenu_rect->w, cMenu_rect->h);
	printTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1, cMenu_rect->w, GUI_COLOR_BACKGROUND, cMenu_data->text);
	drawMenuItems();
}
//-----------------------------------------------------------------------------
void UserInterface::drawMenuItems()
{
	GUI_MENU_ENTRY *ptr;
	SDL_Rect *r = new SDL_Rect(*cMenu_rect);
	const char *wrk;

	r->x++;
	r->y += (2 * GUI_CONST_BORDER);
	r->w -= 2;

	for (int mx, my, i = 0; i < cMenu_count; i++) {
		ptr = &cMenu_data[i + 1];
		my = r->y + 2;

		if (ptr->type == MI_SEPARATOR) {
			drawLineH(defaultSurface, r->x + 2, my, r->w - 4, GUI_COLOR_SEPARATOR);
			r->y += GUI_CONST_SEPARATOR;
		}
		else {
			mx = r->x + 7;

			drawRectangle(defaultSurface, r->x, r->y, r->w, GUI_CONST_ITEM_SIZE,
				(i == cMenu_hilite) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);
			printFormatted(defaultSurface, mx + cMenu_leftMargin, my,
				(ptr->enabled) ? GUI_COLOR_FOREGROUND : GUI_COLOR_DISABLED,
				"%s%s", ptr->text, ((ptr->type == MI_SUBMENU) ? " \200" :
					((ptr->type == MI_DIALOG) ? "\205" : "")));

			if (ptr->type == MI_CHECKBOX && ptr->action >= 0x8000) {
				printCheck(defaultSurface, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_HOTKEY : GUI_COLOR_DISABLED),
					SCHR_LOCKER, ptr->state);
			}
			else if (ptr->type == MI_CHECKBOX) {
				printCheck(defaultSurface, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_CHECKED : GUI_COLOR_DISABLED),
					SCHR_CHECK, ptr->state);
			}
			else if (ptr->type == MI_RADIO) {
				printCheck(defaultSurface, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_CHECKED : GUI_COLOR_DISABLED),
					SCHR_RADIO, ptr->state);
			}

			mx += r->w - 12;
			if ((wrk = ptr->hotkey) != NULL) {
				BYTE c = (ptr->enabled) ? GUI_COLOR_HOTKEY : GUI_COLOR_DISABLED;
				char hkchr = (menuStack[menuStackLevel].type != GUI_TYPE_TAPE_POPUP) ? SCHR_HOTKEY : ' ';

				mx -= GUI_CONST_HOTKEYCHAR + (strlen(wrk) * 6);
				if (wrk[0] == '^') {
					wrk++;
					mx += GUI_CONST_HOTKEYCHAR - (GUI_CONST_HOTKEYCHAR - 6);

					printChar(defaultSurface, mx - GUI_CONST_HOTKEYCHAR, my, c, hkchr);
					printChar(defaultSurface, mx, my, c, SCHR_SHIFT);
				}
				else
					printChar(defaultSurface, mx, my, c, hkchr);

				printText(defaultSurface, mx + GUI_CONST_HOTKEYCHAR, my, c, wrk);
			}
			else if (ptr->detail) {
				wrk = ptr->detail(ptr);
				if (wrk) {
					mx -= (strlen(wrk) * 6) + 12;
					printFormatted(defaultSurface, mx, my, GUI_COLOR_DISABLED, "[%s]", wrk);
				}
			}

			r->y += GUI_CONST_ITEM_SIZE;
		}
	}

	delete r;
	needRedraw = true;
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerMenu(WORD key)
{
	GUI_MENU_ENTRY *ptr = &cMenu_data[cMenu_hilite + 1];
	bool change = false;

	switch (key) {
		case SDLK_F4 | KM_ALT:
			Emulator->ActionExit();
			menuCloseAll();
			needRelease = true;
			return;

		case SDLK_ESCAPE:
			menuClose();
			needRelease = true;
			return;

		case SDLK_SPACE:
			if (ptr->enabled && ptr->type == MI_CHECKBOX
			 && ptr->action >= 0x8000)
				ptr->action |= 0x4000; // ... and continue in ENTER section...
			else
				break;

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (ptr->enabled) {
				needRelease = true;

				if (ptr->type == MI_SUBMENU && ptr->submenu)
					menuOpen(GUI_TYPE_MENU, ptr->submenu);
				else if (ptr->callback) {
					if (ptr->callback(ptr)) {
						if (uiSetChanges & PS_CLOSEALL)
							menuCloseAll();
						else
							menuClose();
						return;
					}
					else if (menuStack[menuStackLevel].type == GUI_TYPE_TAPE_POPUP) {
						menuClose();
						keyhandlerTapeDialog(ptr->action);
						return;
					}
					else if (cMenu_data)
						change = true;
					else
						return;
				}
			}
			break;

		case SDLK_BACKSPACE:
		case SDLK_LEFT:
			if (menuStackLevel > 0)
				menuClose();

			needRelease = true;
			return;

		case SDLK_RIGHT:
			if (ptr->enabled && ptr->type == MI_SUBMENU && ptr->submenu)
				menuOpen(GUI_TYPE_MENU, ptr->submenu);

			needRelease = true;
			return;

		case SDLK_UP:
			cMenu_hilite--;
			if (cMenu_hilite < 0)
				cMenu_hilite += cMenu_count;

			while ((&cMenu_data[cMenu_hilite + 1])->type & (MI_FIXED | MI_SEPARATOR)) {
				cMenu_hilite--;
				if (cMenu_hilite < 0)
					cMenu_hilite += cMenu_count;
			}

			change = true;
			break;

		case SDLK_DOWN:
			cMenu_hilite++;
			if (cMenu_hilite >= cMenu_count)
				cMenu_hilite -= cMenu_count;

			while ((&cMenu_data[cMenu_hilite + 1])->type & (MI_FIXED | MI_SEPARATOR)) {
				cMenu_hilite++;
				if (cMenu_hilite >= cMenu_count)
					cMenu_hilite -= cMenu_count;
			}

			change = true;
			break;

		case SDLK_HOME:
		case SDLK_PAGEUP:
			cMenu_hilite = 0;
			needRelease = true;
			change = true;
			break;

		case SDLK_END:
		case SDLK_PAGEDOWN:
			cMenu_hilite = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			break;
	}

	if (change) {
		drawMenuItems();
		return;
	}

	for (ptr = &cMenu_data[1]; ptr->type != MENU_END; ptr++) {
		if (ptr->enabled && key == ptr->key) {
			if (ptr->type == MI_SUBMENU && ptr->submenu)
				menuOpen(GUI_TYPE_MENU, ptr->submenu);
			else if (ptr->callback) {
				if (ptr->callback(ptr)) {
					if (uiSetChanges & PS_CLOSEALL)
						menuCloseAll();
					else
						menuClose();
				}
				else if (menuStack[menuStackLevel].type == GUI_TYPE_TAPE_POPUP) {
					menuClose();
					keyhandlerTapeDialog(ptr->action);
					return;
				}
				else
					drawMenuItems();
			}

			break;
		}
	}
}
//-----------------------------------------------------------------------------
