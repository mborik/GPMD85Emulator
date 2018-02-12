/*	Menu.cpp: Part of GUI rendering class: Menu drawing and handling
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
#include "UserInterface.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
void UserInterface::DrawMenu(void *data)
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

		k = (strlen(ptr->text) * fontWidth) +
			((ptr->type == MI_SUBMENU) ?
				(fontWidth + fontWidth) :
				((ptr->type == MI_DIALOG) ? fontWidth : 0));

		if (widthLeft < k)
			widthLeft = k;

		k = 0;
		if ((wrk = ptr->hotkey) != NULL) {
			if (wrk[0] == '^') {
				k += GUI_CONST_HOTKEYCHAR;
				wrk++;
			}

			k += (strlen(wrk) * fontWidth) +
				((menuStack[menuStackLevel].type != GUI_TYPE_TAPE_POPUP) ?
						GUI_CONST_HOTKEYCHAR : 0);
		}

		if (ptr->detail) {
			wrk = ptr->detail(ptr);
			if (wrk) {
				const char *lastDot = NULL;
				int len = strlen(wrk);

				if (len > 15)
					lastDot = strrchr(wrk, '.');
				int max = SDL_min(15, len);
				if (lastDot)
					max = SDL_min(max, lastDot - wrk);

				k += ++max * fontWidth;
			}
		}

		if (widthRight < k)
			widthRight = k;
	}

	cMenu_count = (ptr - &cMenu_data[1]);

	k = cMenu_leftMargin + widthLeft + fontWidth + widthRight;
	if ((int) (strlen(cMenu_data->text) * fontWidth) > k)
		k = strlen(cMenu_data->text) * fontWidth;

	cMenu_rect->w = (2 * GUI_CONST_BORDER) + k;
	cMenu_rect->h = (3 * GUI_CONST_BORDER) + height;
	cMenu_rect->x = (frameWidth  - cMenu_rect->w) / 2;
	cMenu_rect->y = (frameHeight - cMenu_rect->h) / 2;

	if (menuStack[menuStackLevel].type == GUI_TYPE_TAPE_POPUP)
		cMenu_rect->x = (cMenu_rect->x * 2) - 1;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	DrawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y, cMenu_rect->w, cMenu_rect->h);
	PrintTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1, cMenu_rect->w, GUI_COLOR_BACKGROUND, cMenu_data->text);
	DrawMenuItems(defaultSurface);

	UnlockSurface(defaultTexture, defaultSurface);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawMenuItems(GUI_SURFACE *s)
{
	bool needUnlock = false;
	if (s == NULL) {
		s = LockSurface(defaultTexture);
		needUnlock = true;
	}

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
			DrawLineH(s, r->x + 2, my, r->w - 4, GUI_COLOR_SEPARATOR);
			r->y += GUI_CONST_SEPARATOR;
		}
		else {
			mx = r->x + 7;

			DrawRectangle(s, r->x, r->y, r->w, GUI_CONST_ITEM_SIZE,
				(i == cMenu_hilite) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);
			PrintFormatted(s, mx + cMenu_leftMargin, my,
				(ptr->enabled) ? GUI_COLOR_FOREGROUND : GUI_COLOR_DISABLED,
				"%s%s", ptr->text, ((ptr->type == MI_SUBMENU) ? " \200" :
					((ptr->type == MI_DIALOG) ? "\205" : "")));

			if (ptr->type == MI_CHECKBOX && ptr->action >= 0x8000) {
				PrintCheck(s, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_HOTKEY : GUI_COLOR_DISABLED),
					SCHR_LOCKER, ptr->state);
			}
			else if (ptr->type == MI_CHECKBOX) {
				PrintCheck(s, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_CHECKED : GUI_COLOR_DISABLED),
					SCHR_CHECK, ptr->state);
			}
			else if (ptr->type == MI_RADIO) {
				PrintCheck(s, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_CHECKED : GUI_COLOR_DISABLED),
					SCHR_RADIO, ptr->state);
			}

			mx += r->w - 12;
			if ((wrk = ptr->hotkey) != NULL) {
				BYTE c = (ptr->enabled) ? GUI_COLOR_HOTKEY : GUI_COLOR_DISABLED;
				char hkchr = (menuStack[menuStackLevel].type != GUI_TYPE_TAPE_POPUP) ?
						(char) SCHR_HOTKEY : ' ';

				mx -= GUI_CONST_HOTKEYCHAR + (strlen(wrk) * 6);
				if (wrk[0] == '^') {
					wrk++;
					mx += GUI_CONST_HOTKEYCHAR - (GUI_CONST_HOTKEYCHAR - 6);

					PrintChar(s, mx - GUI_CONST_HOTKEYCHAR, my, c, hkchr);
					PrintChar(s, mx, my, c, SCHR_SHIFT);
				}
				else
					PrintChar(s, mx, my, c, hkchr);

				PrintText(s, mx + GUI_CONST_HOTKEYCHAR, my, c, wrk);
			}
			else if (ptr->detail) {
				wrk = ptr->detail(ptr);
				if (wrk) {
					const char *lastDot = NULL;
					int len = strlen(wrk);

					if (len > 15)
						lastDot = strrchr(wrk, '.');
					int max = SDL_min(15, len);
					if (lastDot)
						max = SDL_min(max, lastDot - wrk);

					mx -= (max + 2) * 6;
					if (len > 15)
						PrintFormatted(s, (mx -= 6), my, GUI_COLOR_DISABLED, "[%.*s\205]", max, wrk);
					else
						PrintFormatted(s, mx, my, GUI_COLOR_DISABLED, "[%s]", wrk);
				}
			}

			r->y += GUI_CONST_ITEM_SIZE;
		}
	}

	if (needUnlock)
		UnlockSurface(defaultTexture, s);

	delete r;
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerMenu(WORD key)
{
	GUI_MENU_ENTRY *ptr = &cMenu_data[cMenu_hilite + 1];
	bool change = false;

	switch (key) {
		case SDL_SCANCODE_F4 | KM_ALT:
			Emulator->ActionExit();
			MenuCloseAll();
			return;

		case SDL_SCANCODE_ESCAPE:
			MenuClose();
			return;

		case SDL_SCANCODE_SPACE:
			if (ptr->enabled && ptr->type == MI_CHECKBOX
			 && ptr->action >= 0x8000)
				ptr->action |= 0x4000; // ... and continue in ENTER section...
			else
				break;

		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			if (ptr->enabled) {
				needRelease = true;

				if (ptr->type == MI_SUBMENU && ptr->submenu)
					MenuOpen(GUI_TYPE_MENU, ptr->submenu);
				else if (ptr->callback) {
					if (ptr->callback(ptr)) {
						if (uiSetChanges & PS_CLOSEALL)
							MenuCloseAll();
						else
							MenuClose();
						return;
					}
					else if (menuStack[menuStackLevel].type == GUI_TYPE_TAPE_POPUP) {
						MenuClose();
						KeyhandlerTapeDialog(ptr->action);
						return;
					}
					else if (cMenu_data)
						change = true;
					else
						return;
				}
			}
			break;

		case SDL_SCANCODE_BACKSPACE:
		case SDL_SCANCODE_LEFT:
			if (menuStackLevel > 0)
				MenuClose();

			needRelease = true;
			return;

		case SDL_SCANCODE_RIGHT:
			if (ptr->enabled && ptr->type == MI_SUBMENU && ptr->submenu)
				MenuOpen(GUI_TYPE_MENU, ptr->submenu);

			needRelease = true;
			return;

		case SDL_SCANCODE_UP:
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

		case SDL_SCANCODE_DOWN:
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

		case SDL_SCANCODE_HOME:
		case SDL_SCANCODE_PAGEUP:
			cMenu_hilite = 0;
			needRelease = true;
			change = true;
			break;

		case SDL_SCANCODE_END:
		case SDL_SCANCODE_PAGEDOWN:
			cMenu_hilite = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			break;
	}

	if (change) {
		DrawMenuItems();
		return;
	}

	for (ptr = &cMenu_data[1]; ptr->type != MENU_END; ptr++) {
		if (ptr->enabled && key == ptr->key) {
			if (ptr->type == MI_SUBMENU && ptr->submenu)
				MenuOpen(GUI_TYPE_MENU, ptr->submenu);
			else if (ptr->callback) {
				if (ptr->callback(ptr)) {
					if (uiSetChanges & PS_CLOSEALL)
						MenuCloseAll();
					else
						MenuClose();
				}
				else if (menuStack[menuStackLevel].type == GUI_TYPE_TAPE_POPUP) {
					MenuClose();
					KeyhandlerTapeDialog(ptr->action);
					return;
				}
				else if (cMenu_data)
					DrawMenuItems();
			}

			break;
		}
	}
}
//-----------------------------------------------------------------------------
