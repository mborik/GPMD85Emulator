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
void UserInterface::EditBox(const char *title, const char *description, char *buffer, BYTE maxLength, bool decimal)
{
	if (maxLength <= 0 || maxLength > (maxCharsOnScreen - 2))
		maxLength = maxCharsOnScreen - 2;

	editBox->buffer = buffer;
	editBox->maxLength = maxLength;
	editBox->decimal = decimal;
	editBox->len = strlen(buffer);
	editBox->cursor = editBox->len;
	editBox->result = (BYTE) -1;
	editBox->atTheEnd = true;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	if (menuStackLevel < 0)
		memset(defaultSurface->pixels, 0, frameLength);
	else {
		menuStack[menuStackLevel].frame = new BYTE[frameLength];
		memcpy(menuStack[menuStackLevel].frame, defaultSurface->pixels, frameLength);
	}

	menuStackLevel++;
	menuStack[menuStackLevel].type = GUI_TYPE_EDITBOX;
	menuStack[menuStackLevel].data = editBox;
	menuStack[menuStackLevel].frame = NULL;
	menuStack[menuStackLevel].hilite = 0;

	editBox->w = (strlen(title) + 4) * fontWidth;
	WORD h = (4 * GUI_CONST_BORDER);

	if (description != NULL && editBox->w < ((strlen(description) + 4) * fontWidth))
		editBox->w = (strlen(description) + 4) * fontWidth;
	if (editBox->w < ((editBox->maxLength + 1) * fontWidth))
		editBox->w = ((editBox->maxLength + 1) * fontWidth);
	editBox->w = (2 * GUI_CONST_BORDER) + editBox->w;
	if (description != NULL)
		h += (2 * GUI_CONST_BORDER);
	editBox->x = (frameWidth  - editBox->w) / 2;
	editBox->y = (frameHeight - h) / 2;

	DrawDialogWithBorder(defaultSurface, editBox->x, editBox->y, editBox->w, h);
	PrintTitle(defaultSurface, editBox->x, editBox->y + 1, editBox->w, GUI_COLOR_BACKGROUND, title);

	editBox->x += GUI_CONST_BORDER;
	editBox->y += (2 * GUI_CONST_BORDER);
	editBox->w -= (2 * GUI_CONST_BORDER);

	if (description != NULL) {
		PrintText(defaultSurface, editBox->x, editBox->y, GUI_COLOR_DISABLED, description);
		editBox->y += (2 * GUI_CONST_BORDER);
	}
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerEditBox(WORD key)
{
	bool change = false;

	switch (key) {
		case SDL_SCANCODE_ESCAPE:
			editBox->result = 0;
			MenuClose();
			break;

		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			// trim left
			editBox->cursor = 0;
			while (editBox->buffer[editBox->cursor] == ' ')
				editBox->cursor++;

			if (editBox->cursor > 0)
				for (int i = 0; editBox->cursor <= editBox->len; i++, editBox->cursor++)
					editBox->buffer[i] = editBox->buffer[editBox->cursor];

			// trim right
			editBox->cursor = (editBox->len = strlen(editBox->buffer)) - 1;
			while (editBox->buffer[editBox->cursor] == ' ')
				editBox->cursor--;

			if (++editBox->cursor < editBox->len)
				editBox->buffer[editBox->cursor] = '\0';

			editBox->result = 1;
			MenuClose();
			break;

		case SDL_SCANCODE_LEFT:
			if (editBox->cursor > 0) {
				editBox->cursor--;
				change = true;
			}
			break;

		case SDL_SCANCODE_RIGHT:
			if (!editBox->atTheEnd) {
				editBox->cursor++;
				change = true;
			}
			break;

		case SDL_SCANCODE_BACKSPACE:
			if (editBox->cursor > 0) {
				editBox->cursor--;
				for (int i = editBox->cursor; i < editBox->len; i++)
					editBox->buffer[i] = editBox->buffer[i + 1];
				change = true;
			}
			break;

		case SDL_SCANCODE_INSERT:
			if (!editBox->atTheEnd && editBox->len < editBox->maxLength) {
				for (int i = editBox->len + 1; i > editBox->cursor; i--)
					editBox->buffer[i] = editBox->buffer[i - 1];
				editBox->buffer[editBox->cursor] = ' ';
				change = true;
			}
			break;

		case SDL_SCANCODE_DELETE:
			if (!editBox->atTheEnd) {
				for (int i = editBox->cursor; i < editBox->len; i++)
					editBox->buffer[i] = editBox->buffer[i + 1];
				change = true;
			}
			break;

		case SDL_SCANCODE_HOME:
			if (editBox->cursor > 0) {
				editBox->cursor = 0;
				change = true;
			}
			break;

		case SDL_SCANCODE_END:
			if (!editBox->atTheEnd) {
				editBox->cursor = editBox->len;
				change = true;
			}
			break;

		default:
			if (key < SDLK_SPACE || key > 0x007E)
				break;

			char c = (char) key & 0x7f;
			if (editBox->cursor < editBox->maxLength) {
				if (editBox->decimal && (c < '0' || c > '9'))
					break;

				editBox->buffer[editBox->cursor] = c;
				editBox->cursor++;
				if (editBox->atTheEnd)
					editBox->buffer[editBox->cursor] = '\0';

				change = true;
			}
			break;
	}

	if (change) {
		GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

		DrawRectangle(defaultSurface,
			editBox->x, editBox->y - 1, editBox->w, fontHeight + 1, GUI_COLOR_BACKGROUND);
		DrawRectangle(defaultSurface,
			editBox->x + (editBox->cursor * fontWidth), editBox->y - 1,
			fontWidth, fontHeight + 1, GUI_COLOR_HIGHLIGHT);
		PrintText(defaultSurface,
			editBox->x, editBox->y, GUI_COLOR_FOREGROUND, editBox->buffer);

		editBox->len = strlen(editBox->buffer);
		editBox->atTheEnd = (editBox->cursor == editBox->len);

		UnlockSurface(defaultTexture, defaultSurface);
	}
}
//-----------------------------------------------------------------------------
