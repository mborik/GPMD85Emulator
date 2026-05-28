/*	CommonDialog.cpp: Part of GUI rendering class: Common popup dialogs
	Copyright (c) 2011-2024 Martin Borik <mborik@users.sourceforge.net>

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
#include "UserInterfaceData.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
void UserInterface::AboutDialog()
{
	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	if (menuStackLevel < 0)
		memset(defaultSurface->pixels, 0, frameLength);
	else {
		menuStack[menuStackLevel].frame = new BYTE[frameLength];
		memcpy(menuStack[menuStackLevel].frame, defaultSurface->pixels, frameLength);
	}

	menuStackLevel++;
	menuStack[menuStackLevel].type = GUI_TYPE_ABOUT_DIALOG;
	menuStack[menuStackLevel].data = NULL;
	menuStack[menuStackLevel].frame = NULL;
	menuStack[menuStackLevel].hilite = 0;

	sprintf(
		msgbuffer,
		"%s v%s (c) %s\n\n"
		"Open-source multi-platform\n"
		"emulator of the Tesla PMD 85,\n"
		"an 8-bit personal micro-computer\n"
		"produced in 80s of 20th century\n"
		"in former Czechoslovakia.\n\n"
		"Licensed under GNU/GPL version 3.\n\n"
		"\200 %s\n"
		"\200 pmd85.borik.net",
		PACKAGE_NAME, VERSION, PACKAGE_YEAR, (PACKAGE_URL) + 8
	);

	unsigned i, w = 0, h = fontLineHeight, x, y;
	for (i = 0, x = 0; i < strlen(msgbuffer); i++) {
		if (x > (unsigned) (maxCharsOnScreen - 5)) {
			for (y = i; y <= strlen(msgbuffer); y++)
				msgbuffer[y + 1] = msgbuffer[y];

			msgbuffer[i] = '\n';
		}

		if (msgbuffer[i] == '\n') {
			h += fontLineHeight;
			if (w < (x * fontWidth))
				w = (x * fontWidth);

			x = 0;
			continue;
		}
		else if (msgbuffer[i] == '\a')
			continue;

		x++;
	}

	if (w < (x * fontWidth))
		w = (x * fontWidth);

	w = (4 * GUI_CONST_BORDER) + 32 + w;
	h = (2 * GUI_CONST_BORDER) + h;
	x = (frameWidth  - w) / 2;
	y = (frameHeight - h) / 2;

	DrawOutlineRounded(defaultSurface, x - 1, y - 1, w + 2, h + 2, GUI_COLOR_SHADOW);
	DrawRectangle(defaultSurface, x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BACKGROUND);
	DrawOutlineRounded(defaultSurface, x, y, w, h, GUI_COLOR_BORDER);
	PrintText(defaultSurface, x + (2 * GUI_CONST_BORDER),
			y + GUI_CONST_BORDER + 1, GUI_COLOR_FOREGROUND, msgbuffer);

	GUI_SURFACE *icon = LoadImgToSurface(LocateResource("icon.bmp"));
	if (icon) {
		SDL_Rect dstRect = {
			(int) ((x + w) - GUI_CONST_BORDER - icon->w),
			(int) (y + GUI_CONST_BORDER),
			icon->w, icon->h
		};

		int bpp = defaultSurface->pitch / defaultSurface->w;
		BYTE *ptr1 = defaultSurface->pixels + (dstRect.y * defaultSurface->pitch) + (dstRect.x * bpp);
		BYTE *ptr2 = icon->pixels;

		for (int hh = icon->h; hh > 0; --hh, ptr1 += defaultSurface->pitch, ptr2 += icon->pitch) {
			for (int ii = 0; ii < icon->w; ii++) {
				DWORD color = *((DWORD *) ptr2 + ii);
				if (color == GUI->globalPalette[0])
					color = GUI->globalPalette[GUI_COLOR_BACKGROUND];
				*((DWORD *) ptr1 + ii) = color;
			}
		}

		free(icon->pixels);
		delete icon;
	}
	else
		warning("GUI", "Can't load icon resource file");

	UnlockSurface(defaultTexture, defaultSurface);
}
//-----------------------------------------------------------------------------
BYTE UserInterface::QueryDialog(const char *title, bool save)
{
	void *data = (save) ? gui_query_save : gui_query_confirm;
	char *dialogTitle = new char[strlen(title) + 5];

	sprintf(dialogTitle, "  %s  ", title);
	((GUI_MENU_ENTRY *) data)->text = (const char *) dialogTitle;

	MenuOpen(GUI_TYPE_QUERY_DIALOG, data);

	// TODO FIXME
	return GUI_QUERY_NONE;
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerQueryDialog(WORD key)
{
	GUI_MENU_ENTRY *ptr = &cMenu_data[cMenu_hilite + 1];
	bool change = false;

	switch (key) {
		case SDL_SCANCODE_ESCAPE:
			uiQueryState = GUI_QUERY_CANCEL;
			MenuClose();
			return;

		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			uiQueryState = ptr->action;
			MenuClose();
			return;

		case SDL_SCANCODE_Y:
			if (cMenu_data[2].action != GUI_QUERY_SAVE) {
				uiQueryState = cMenu_data[1].action;
				MenuClose();
			}
			return;

		case SDL_SCANCODE_N:
			if (cMenu_data[2].action != GUI_QUERY_SAVE) {
				uiQueryState = cMenu_data[2].action;
				MenuClose();
			}
			return;

		case SDL_SCANCODE_UP:
			if (cMenu_hilite > 0) {
				cMenu_hilite--;
				change = true;
			}
			break;

		case SDL_SCANCODE_DOWN:
			if (cMenu_hilite < (cMenu_count - 1)) {
				cMenu_hilite++;
				change = true;
			}
			break;

		default:
			break;
	}

	if (change) {
		DrawMenuItems();
		return;
	}

	for (ptr = &cMenu_data[1]; ptr->type != MENU_END; ptr++) {
		if (key == ptr->key) {
			uiQueryState = ptr->action;
			MenuClose();
			return;
		}
	}
}
//-----------------------------------------------------------------------------
void UserInterface::MessageBox(const char *text, ...)
{
	va_list va;
	va_start(va, text);
	vsprintf(msgbuffer, text, va);
	va_end(va);

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	if (menuStackLevel < 0)
		memset(defaultSurface->pixels, 0, frameLength);
	else {
		menuStack[menuStackLevel].frame = new BYTE[frameLength];
		memcpy(menuStack[menuStackLevel].frame, defaultSurface->pixels, frameLength);
	}

	menuStackLevel++;
	menuStack[menuStackLevel].type = GUI_TYPE_MESSAGE_BOX;
	menuStack[menuStackLevel].data = NULL;
	menuStack[menuStackLevel].frame = NULL;
	menuStack[menuStackLevel].hilite = 0;

	unsigned i, w = 0, h = fontLineHeight, x, y;
	for (i = 0, x = 0; i < strlen(msgbuffer); i++) {
		if (x > (unsigned) (maxCharsOnScreen - 5)) {
			for (y = i; y <= strlen(msgbuffer); y++)
				msgbuffer[y + 1] = msgbuffer[y];

			msgbuffer[i] = '\n';
		}

		if (msgbuffer[i] == '\n') {
			h += fontLineHeight;
			if (w < (x * fontWidth))
				w = (x * fontWidth);

			x = 0;
			continue;
		}
		else if (msgbuffer[i] == '\a')
			continue;

		x++;
	}

	if (w < (x * fontWidth))
		w = (x * fontWidth);

	w = (4 * GUI_CONST_BORDER) + w;
	h = (2 * GUI_CONST_BORDER) + h;
	x = (frameWidth  - w) / 2;
	y = (frameHeight - h) / 2;

	DrawOutlineRounded(defaultSurface, x - 1, y - 1, w + 2, h + 2, GUI_COLOR_SHADOW);
	DrawRectangle(defaultSurface, x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BACKGROUND);
	DrawOutlineRounded(defaultSurface, x, y, w, h, GUI_COLOR_BORDER);
	PrintText(defaultSurface, x + (2 * GUI_CONST_BORDER),
			y + GUI_CONST_BORDER + 1, GUI_COLOR_FOREGROUND, msgbuffer);

	UnlockSurface(defaultTexture, defaultSurface);
	needRelease = true;
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerCommonDialog(WORD key)
{
	switch (key) {
		case SDL_SCANCODE_SPACE:
		case SDL_SCANCODE_ESCAPE:
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			MenuClose();
			break;

		default:
			break;
	}
}
//-----------------------------------------------------------------------------
