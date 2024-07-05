/*	EditBox.cpp: Part of GUI rendering class: Editable input dialogs
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
#include "Emulator.h"
//-----------------------------------------------------------------------------
BYTE UserInterface::EditBox(const char *title, const char *description, char *buffer, BYTE maxLength, bool decimal)
{
	if (maxLength <= 0 || maxLength > (maxCharsOnScreen - 2))
		maxLength = maxCharsOnScreen - 2;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	if (menuStackLevel < 0)
		memset(defaultSurface->pixels, 0, frameLength);
	else {
		menuStack[menuStackLevel].frame = new BYTE[frameLength];
		memcpy(menuStack[menuStackLevel].frame, defaultSurface->pixels, frameLength);
	}

	menuStackLevel++;
	menuStack[menuStackLevel].type = GUI_TYPE_MENU;
	menuStack[menuStackLevel].hilite = 0;
	menuStack[menuStackLevel].data = NULL;
	menuStack[menuStackLevel].frame = NULL;

	WORD i, len = strlen(buffer), cursor = len,
		w = (strlen(title) + 4) * fontWidth, h, x, y;

	if (description != NULL && w < ((strlen(description) + 4) * fontWidth))
		w = (strlen(description) + 4) * fontWidth;
	if (w < ((maxLength + 1) * fontWidth))
		w = ((maxLength + 1) * fontWidth);

	w = (2 * GUI_CONST_BORDER) + w;
	h = (4 * GUI_CONST_BORDER);
	x = (frameWidth  - w) / 2;
	y = (frameHeight - h) / 2;

	if (description != NULL)
		h += (2 * GUI_CONST_BORDER);

	DrawDialogWithBorder(defaultSurface, x, y, w, h);
	PrintTitle(defaultSurface, x, y + 1, w, GUI_COLOR_BACKGROUND, title);

	x += GUI_CONST_BORDER;
	y += (2 * GUI_CONST_BORDER);
	w -= (2 * GUI_CONST_BORDER);

	if (description != NULL) {
		PrintText(defaultSurface, x, y, GUI_COLOR_DISABLED, description);
		y += (2 * GUI_CONST_BORDER);
	}

	UnlockSurface(defaultTexture, defaultSurface);

	DWORD nextTick;
	BYTE result = -1;
	SDL_Event event;
	bool atTheEnd = true, change = true;

	SDL_Delay(GPU_TIMER_INTERVAL);
	while (result == (BYTE) -1) {
		nextTick = SDL_GetTicks() + CPU_TIMER_INTERVAL;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.scancode) {
						case SDL_SCANCODE_ESCAPE:
							result = 0;
							break;

						case SDL_SCANCODE_RETURN:
						case SDL_SCANCODE_KP_ENTER:
							// trim left
							cursor = 0;
							while (buffer[cursor] == ' ')
								cursor++;

							if (cursor > 0)
								for (i = 0; cursor <= len; i++, cursor++)
									buffer[i] = buffer[cursor];

							// trim right
							cursor = (len = strlen(buffer)) - 1;
							while (buffer[cursor] == ' ')
								cursor--;

							if (++cursor < len)
								buffer[cursor] = '\0';

							result = 1;
							break;

						case SDL_SCANCODE_LEFT:
							if (cursor > 0) {
								cursor--;
								change = true;
							}
							break;

						case SDL_SCANCODE_RIGHT:
							if (!atTheEnd) {
								cursor++;
								change = true;
							}
							break;

						case SDL_SCANCODE_BACKSPACE:
							if (cursor > 0) {
								cursor--;
								for (i = cursor; i < len; i++)
									buffer[i] = buffer[i + 1];
								change = true;
							}
							break;

						case SDL_SCANCODE_INSERT:
							if (!atTheEnd && len < maxLength) {
								for (i = len + 1; i > cursor; i--)
									buffer[i] = buffer[i - 1];
								buffer[cursor] = ' ';
								change = true;
							}
							break;

						case SDL_SCANCODE_DELETE:
							if (!atTheEnd) {
								for (i = cursor; i < len; i++)
									buffer[i] = buffer[i + 1];
								change = true;
							}
							break;

						case SDL_SCANCODE_HOME:
							if (cursor > 0) {
								cursor = 0;
								change = true;
							}
							break;

						case SDL_SCANCODE_END:
							if (!atTheEnd) {
								cursor = len;
								change = true;
							}
							break;

						default:
							if (event.key.keysym.sym < SDLK_SPACE
							 || event.key.keysym.sym > 0x007E)
								break;

							char c = (char) event.key.keysym.sym & 0x7f;
							if (cursor < maxLength) {
								if (decimal && (c < '0' || c > '9'))
									break;

								buffer[cursor] = c;
								cursor++;
								if (atTheEnd)
									buffer[cursor] = '\0';

								change = true;
							}
							break;
					}
					break;

				case SDL_WINDOWEVENT:
					if (event.window.windowID == gdc.windowID &&
						event.window.event == SDL_WINDOWEVENT_EXPOSED) {

						Emulator->RefreshDisplay();
					}
					break;

				default:
					break;
			}

			if (change) {
				defaultSurface = LockSurface(defaultTexture);

				DrawRectangle(defaultSurface, x, y - 1, w, fontHeight + 1,
					GUI_COLOR_BACKGROUND);
				DrawRectangle(defaultSurface, x + (cursor * fontWidth), y - 1,
					fontWidth, fontHeight + 1, GUI_COLOR_HIGHLIGHT);
				PrintText(defaultSurface, x, y, GUI_COLOR_FOREGROUND, buffer);

				len = strlen(buffer);
				atTheEnd = (cursor == len);

				UnlockSurface(defaultTexture, defaultSurface);
				change = false;
				break;
			}
		}

		while (SDL_GetTicks() < nextTick)
			SDL_Delay(1);
	}

	defaultSurface = LockSurface(defaultTexture);

	menuStackLevel--;
	if (menuStackLevel >= 0) {
		memcpy(defaultSurface->pixels, menuStack[menuStackLevel].frame, frameLength);
		delete [] menuStack[menuStackLevel].frame;
		menuStack[menuStackLevel].frame = NULL;
	}
	else
		memset(defaultSurface->pixels, 0, frameLength);

	UnlockSurface(defaultTexture, defaultSurface);
	SDL_Delay(GPU_TIMER_INTERVAL);

	needRelease = true;
	return result;
}
//-----------------------------------------------------------------------------
