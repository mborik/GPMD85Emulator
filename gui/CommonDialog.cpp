/*	CommonDialog.cpp: Part of GUI rendering class: Common popup dialogs
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
#include "UserInterfaceData.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
BYTE UserInterface::queryDialog(const char *title, bool save)
{
	void *data = (save) ? gui_query_save : gui_query_confirm;
	char *dialogTitle = new char[strlen(title) + 5];

	sprintf(dialogTitle, "  %s  ", title);
	((GUI_MENU_ENTRY *) data)->text = (const char *) dialogTitle;

	BYTE *bkm_frameSave = new BYTE[frameLength];
	memcpy(bkm_frameSave, defaultSurface->pixels, frameLength);

	GUI_MENU_ENTRY *bkm_data = cMenu_data;
	SDL_Rect *bkm_rect = new SDL_Rect(*cMenu_rect);
	int bkm_leftMargin = cMenu_leftMargin, bkm_count = cMenu_count,
	    bkm_hilite = cMenu_hilite;

	menuStackLevel++;
	menuStack[menuStackLevel].type = GUI_TYPE_MENU;
	menuStack[menuStackLevel].data = data;
	menuStack[menuStackLevel].hilite = cMenu_hilite = (save) ? 2 : 1;

	drawMenu(data);

	bool change;
	DWORD nextTick;
	SDL_Event event;
	GUI_MENU_ENTRY *ptr;
	uiQueryState = GUI_QUERY_NONE;

	SDL_Delay(GPU_TIMER_INTERVAL);
	while (uiQueryState == GUI_QUERY_NONE) {
		nextTick = SDL_GetTicks() + CPU_TIMER_INTERVAL;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					ptr = &cMenu_data[cMenu_hilite + 1];
					change = false;

					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							uiQueryState = GUI_QUERY_CANCEL;
							break;

						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							uiQueryState = ptr->action;
							break;

						case SDLK_UP:
							if (cMenu_hilite > 0) {
								cMenu_hilite--;
								change = true;
							}
							break;

						case SDLK_DOWN:
							if (cMenu_hilite < (cMenu_count - 1)) {
								cMenu_hilite++;
								change = true;
							}
							break;

						default:
							break;
					}

					if (change) {
						drawMenuItems();
						break;
					}

					for (ptr = &cMenu_data[1]; ptr->type != MENU_END; ptr++) {
						if (event.key.keysym.sym == ptr->key) {
							uiQueryState = ptr->action;
							break;
						}
					}
					break;

				default:
					break;
			}
		}

		// have a break, have a tick-tock...
		while (SDL_GetTicks() < nextTick)
			SDL_Delay(1);
	}

	memcpy(defaultSurface->pixels, bkm_frameSave, frameLength);
	needRelease = true;
	needRedraw = true;

	SDL_Delay(GPU_TIMER_INTERVAL);

	((GUI_MENU_ENTRY *) data)->text = NULL;
	delete [] bkm_frameSave;
	delete [] dialogTitle;

	cMenu_rect->x = bkm_rect->x;
	cMenu_rect->y = bkm_rect->y;
	cMenu_rect->w = bkm_rect->w;
	cMenu_rect->h = bkm_rect->h;
	delete bkm_rect;

	cMenu_data = bkm_data;
	cMenu_leftMargin = bkm_leftMargin;
	cMenu_hilite = bkm_hilite;
	cMenu_count = bkm_count;
	menuStackLevel--;

	return uiQueryState;
}
//-----------------------------------------------------------------------------
void UserInterface::messageBox(const char *text, ...)
{
	va_list va;
	va_start(va, text);
	vsprintf(msgbuffer, text, va);
	va_end(va);

	BYTE *bkm_frameSave = new BYTE[frameLength];
	memcpy(bkm_frameSave, defaultSurface->pixels, frameLength);

	menuStackLevel++;

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

	drawOutlineRounded(defaultSurface, x - 1, y - 1, w + 2, h + 2, GUI_COLOR_SHADOW);
	drawRectangle(defaultSurface, x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BACKGROUND);
	drawOutlineRounded(defaultSurface, x, y, w, h, GUI_COLOR_BORDER);
	printText(defaultSurface, x + (2 * GUI_CONST_BORDER),
			y + GUI_CONST_BORDER + 1, GUI_COLOR_FOREGROUND, msgbuffer);

	needRedraw = true;
	SDL_Delay(GPU_TIMER_INTERVAL);

	i = 1;
	DWORD nextTick;
	SDL_Event event;
	while (i) {
		nextTick = SDL_GetTicks() + CPU_TIMER_INTERVAL;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_SPACE:
						case SDLK_ESCAPE:
						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							i = 0;
							break;

						default:
							break;
					}
					break;

				default:
					break;
			}
		}

		// have a break, have a tick-tock...
		while (SDL_GetTicks() < nextTick)
			SDL_Delay(1);
	}

	memcpy(defaultSurface->pixels, bkm_frameSave, frameLength);
	SDL_Delay(GPU_TIMER_INTERVAL);

	delete [] bkm_frameSave;
	needRelease = true;
	needRedraw = true;
	menuStackLevel--;
}
//-----------------------------------------------------------------------------
