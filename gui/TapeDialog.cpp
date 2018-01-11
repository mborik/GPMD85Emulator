/*	TapeDialog.cpp: Part of GUI rendering class: Tape Browser Dialog
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
void UserInterface::DrawTapeDialog(bool update)
{
	if (update || tapeDialog->entries == NULL)
		TapeBrowser->FillFileList(&tapeDialog->entries,
				&tapeDialog->count, Settings->TapeBrowser->hex);

	cMenu_data = NULL;
	cMenu_leftMargin = cMenu_count = tapeDialog->count;
	cMenu_hilite = TapeBrowser->currBlockIdx;
	if (cMenu_hilite < 0)
		cMenu_hilite = 0;

	while (cMenu_hilite < cMenu_leftMargin) {
		cMenu_leftMargin -= GUI_CONST_TAPE_ITEMS;
		if (cMenu_leftMargin < 0)
			cMenu_leftMargin = 0;
	}

	if (cMenu_hilite >= (cMenu_leftMargin + GUI_CONST_TAPE_ITEMS))
		cMenu_leftMargin = cMenu_hilite - GUI_CONST_TAPE_ITEMS + 1;

	cMenu_rect->w = GUI_CONST_BORDER + (31 * fontWidth) + GUI_CONST_BORDER;
	cMenu_rect->h = (5 * GUI_CONST_BORDER) + (19 * GUI_CONST_ITEM_SIZE) + GUI_CONST_SEPARATOR;
	cMenu_rect->x = (frameWidth  - cMenu_rect->w) / 2;
	cMenu_rect->y = (frameHeight - cMenu_rect->h) / 2;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	DrawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y,
		cMenu_rect->w, cMenu_rect->h);
	PrintTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1,
		cMenu_rect->w, GUI_COLOR_BACKGROUND, "TAPE BROWSER");
	DrawLineH(defaultSurface, cMenu_rect->x + (GUI_CONST_BORDER / 2),
		cMenu_rect->y + (3 * GUI_CONST_BORDER) +
		(GUI_CONST_TAPE_ITEMS * GUI_CONST_ITEM_SIZE) + 6,
		cMenu_rect->w - GUI_CONST_BORDER, GUI_COLOR_SEPARATOR);

	int mx = cMenu_rect->x + cMenu_rect->w - GUI_CONST_BORDER - 1,
		my = cMenu_rect->y + cMenu_rect->h - 5 - (4 * fontLineHeight);

	PrintText(defaultSurface, mx - (10 * fontWidth), my,
		GUI_COLOR_FOREGROUND, "MENU \aE\aN\aT\aE\aR");

	PrintText(defaultSurface, mx - (6 * fontWidth) - GUI_CONST_HOTKEYCHAR,
		my + fontLineHeight, GUI_COLOR_FOREGROUND,
		(TapeBrowser->playing ? "STOP \a\203\aP" : "PLAY \a\203\aP"));

	mx = cMenu_rect->x + GUI_CONST_BORDER;
	PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
		GUI_COLOR_FOREGROUND, "\aH HEX/DEC");

	PrintCheck(defaultSurface, mx, my + fontLineHeight + 1, GUI_COLOR_CHECKED,
		SCHR_CHECK, Settings->TapeBrowser->flash);
	PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my + fontLineHeight,
		GUI_COLOR_FOREGROUND, "\aF FLASHLOAD");

	PrintCheck(defaultSurface, mx, my + (2 * fontLineHeight) + 1,
		GUI_COLOR_CHECKED, SCHR_CHECK, Settings->TapeBrowser->monitoring);
	PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my + (2 * fontLineHeight),
		GUI_COLOR_FOREGROUND, "\aO AUDIO-OUT");

	PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN,
		my + (3 * fontLineHeight), GUI_COLOR_FOREGROUND,
		"\aA AUTO-STOP:");

	static char autostop[12];
	switch (Settings->TapeBrowser->autoStop) {
		default:
		case AS_OFF:
			strcpy(autostop, "END OF TAPE");
			break;
		case AS_NEXTHEAD:
			strcpy(autostop, "NEXT HEADER");
			break;
		case AS_CURSOR:
			strcpy(autostop, "STOP-CURSOR");
			break;
	}

	PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN + (13 * fontWidth),
		my + (3 * fontLineHeight), GUI_COLOR_HOTKEY, autostop);

	static char *ptr = NULL;
	if (Settings->TapeBrowser->fileName && !TapeBrowser->preparedForSave) {
		ptr = strrchr(Settings->TapeBrowser->fileName, '/');
		if (ptr)
			ptr++;
		else
			ptr = Settings->TapeBrowser->fileName;
	}
	else
		ptr = (char *) "[NEW TAPE]";

	my = cMenu_rect->y + GUI_CONST_ITEM_SIZE + 1;
	PrintFormatted(defaultSurface, mx + GUI_CONST_HOTKEYCHAR, my,
		GUI_COLOR_BORDER, ((strlen(ptr) > 28) ? "%.28s\205" : "%s"), ptr);

	if (TapeBrowser->tapeChanged)
		PrintChar(defaultSurface, mx, my, GUI_COLOR_CHECKED, '*');

	DrawTapeDialogItems(defaultSurface);

	UnlockSurface(defaultTexture, defaultSurface);
	needRedraw = true;
}
//-----------------------------------------------------------------------------
void UserInterface::DrawTapeDialogItems(GUI_SURFACE *s)
{
	bool needUnlock = false;
	if (s == NULL) {
		s = LockSurface(defaultTexture);
		needUnlock = true;
	}

	SDL_Rect *r = new SDL_Rect(*cMenu_rect);

	r->x += GUI_CONST_BORDER;
	r->y += (3 * GUI_CONST_BORDER) + 4;
	r->w -= (2 * GUI_CONST_BORDER);

	PrintChar(s, r->x + r->w, r->y - 1, (cMenu_leftMargin > 0)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_UP);

	int i = cMenu_leftMargin, j = (cMenu_count) ? cMenu_hilite : -1;
	for (; i < (cMenu_leftMargin + GUI_CONST_TAPE_ITEMS); i++) {
		DrawRectangle(s, r->x - (GUI_CONST_BORDER / 2),
			r->y - 2, r->w, GUI_CONST_ITEM_SIZE,
			(i == j) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);

		if (cMenu_count && i < cMenu_count) {
			PrintChar(s, r->x, r->y, GUI_COLOR_FOREGROUND,
				(i == TapeBrowser->stopBlockIdx) ? SCHR_STOP :
				(i == TapeBrowser->currBlockIdx) ? SCHR_NAVIGATOR : ' ');

			PrintText(s, r->x + GUI_CONST_HOTKEYCHAR, r->y,
				GUI_COLOR_FOREGROUND, tapeDialog->entries[i]);

			if (tapeDialog->entries[i][28])
				DrawRectangle(s, r->x - (GUI_CONST_BORDER / 2),
					r->y - 2, 2, GUI_CONST_ITEM_SIZE, GUI_COLOR_SMARTKEY);
			if (tapeDialog->entries[i][29])
				PrintChar(s, r->x + r->w - GUI_CONST_HOTKEYCHAR - 2,
					r->y, GUI_COLOR_BORDER, tapeDialog->entries[i][29]);
		}

		r->y += GUI_CONST_ITEM_SIZE;
	}

	PrintChar(s, r->x + r->w,
		r->y - GUI_CONST_ITEM_SIZE + 2, (i < cMenu_count)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_DW);

	r->h = (GUI_CONST_TAPE_ITEMS * GUI_CONST_ITEM_SIZE) + (GUI_CONST_BORDER / 2);
	r->y -= r->h - (GUI_CONST_BORDER / 4);

	DrawLineV(s, r->x + GUI_CONST_HOTKEYCHAR + (14 * fontWidth),
		r->y, r->h, GUI_COLOR_SEPARATOR);
	DrawLineV(s, r->x + GUI_CONST_HOTKEYCHAR + (21 * fontWidth),
		r->y, r->h, GUI_COLOR_SEPARATOR);

	if (needUnlock) {
		UnlockSurface(defaultTexture, s);
		needRedraw = true;
	}

	delete r;
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerTapeDialog(WORD key)
{
	int i = cMenu_hilite, prevLeftMargin = 0;
	bool change = false;

	switch (key) {
		case SDL_SCANCODE_F1 | KM_ALT:
			key = SDL_SCANCODE_MENU;
			break;
		case SDL_SCANCODE_F4 | KM_ALT:
			key = SDL_SCANCODE_POWER;
			break;
		default:
			key &= (KM_ALT ^ 0xFFFF);
			break;
	}

	switch (key) {
		case SDL_SCANCODE_POWER:
			Emulator->ActionExit();
			MenuCloseAll();
			needRelease = true;
			return;

		case SDL_SCANCODE_ESCAPE:
			MenuClose();
			needRelease = true;
			return;

		case SDL_SCANCODE_F:
			prevLeftMargin = cMenu_leftMargin;
			Settings->TapeBrowser->flash = !Settings->TapeBrowser->flash;
			DrawTapeDialog(false);
			change = true;
			break;

		case SDL_SCANCODE_O:
			prevLeftMargin = cMenu_leftMargin;
			Settings->TapeBrowser->monitoring = !Settings->TapeBrowser->monitoring;
			DrawTapeDialog(false);
			change = true;
			break;

		case SDL_SCANCODE_A:
			prevLeftMargin = cMenu_leftMargin;
			if (Settings->TapeBrowser->autoStop == AS_OFF)
				Settings->TapeBrowser->autoStop = AS_NEXTHEAD;
			else if (Settings->TapeBrowser->autoStop == AS_NEXTHEAD)
				Settings->TapeBrowser->autoStop = AS_CURSOR;
			else if (Settings->TapeBrowser->autoStop == AS_CURSOR)
				Settings->TapeBrowser->autoStop = AS_OFF;
			DrawTapeDialog(false);
			change = true;
			break;

		case SDL_SCANCODE_H:
			prevLeftMargin = cMenu_leftMargin;
			Settings->TapeBrowser->hex = !Settings->TapeBrowser->hex;
			DrawTapeDialog();
			change = true;
			break;

		case SDL_SCANCODE_P:
			needRelease = true;
			if (!cMenu_count)
				break;
			if (TapeBrowser->playing) {
				TapeBrowser->ActionStop();
				prevLeftMargin = cMenu_leftMargin;
				DrawTapeDialog();
				change = true;
			}
			else {
				uiCallback.connect(Emulator, &TEmulator::ActionTapePlayStop);
				uiSetChanges |= PS_CLOSEALL;
				MenuCloseAll();
			}
			break;

		case SDL_SCANCODE_END | KM_SHIFT:
			needRelease = true;
			if (!cMenu_count)
				break;
			if (i >= 0 && i != TapeBrowser->currBlockIdx) {
				TapeBrowser->stopBlockIdx = i;
				change = true;
			}
			break;

		case SDL_SCANCODE_SPACE:
			needRelease = true;
			if (!cMenu_count)
				break;
			TapeBrowser->SetCurrentBlock(i);
			change = true;
			break;

		case SDL_SCANCODE_INSERT:
			if (!cMenu_count)
				break;
			prevLeftMargin = cMenu_leftMargin;
			TapeBrowser->ToggleSelection(i);
			DrawTapeDialog();
			if (i < (cMenu_count - 1))
				i++;
			change = true;
			break;

		case SDL_SCANCODE_DELETE | KM_SHIFT:
			needRelease = true;
			if (!cMenu_count)
				break;
			prevLeftMargin = cMenu_leftMargin;
			TapeBrowser->DeleteSelected(i);
			DrawTapeDialog();
			if (cMenu_count && i >= cMenu_count)
				i = cMenu_count - 1;
			change = true;
			break;

		case SDL_SCANCODE_UP   | KM_SHIFT:
			change = true;
		case SDL_SCANCODE_DOWN | KM_SHIFT:
			if (!cMenu_count)
				break;
			prevLeftMargin = cMenu_leftMargin;
			TapeBrowser->MoveSelected(change, &i);
			DrawTapeDialog();
			if (prevLeftMargin > i)
				prevLeftMargin = i;
			change = true;
			break;

		case SDL_SCANCODE_MENU:
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			needRelease = true;
			MenuOpen(GUI_TYPE_TAPE_POPUP);
			break;

		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_PAGEUP:
			if (i > 0) {
				i -= GUI_CONST_TAPE_ITEMS;
				if (i < 0)
					i = 0;
				change = true;
			}
			break;

		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_PAGEDOWN:
			if (i < (cMenu_count - 1)) {
				i += GUI_CONST_TAPE_ITEMS;
				if (i >= cMenu_count)
					i = (cMenu_count - 1);
				change = true;
			}
			break;

		case SDL_SCANCODE_UP:
			if (i > 0) {
				i--;
				change = true;
			}
			break;

		case SDL_SCANCODE_DOWN:
			if (i < (cMenu_count - 1)) {
				i++;
				change = true;
			}
			break;

		case SDL_SCANCODE_HOME:
			i = 0;
			needRelease = true;
			change = true;
			break;

		case SDL_SCANCODE_END:
			i = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			break;
	}

	if (change) {
		if (prevLeftMargin)
			cMenu_leftMargin = prevLeftMargin;

		if (i == cMenu_leftMargin - 1)
			cMenu_leftMargin = i;

		while (i < cMenu_leftMargin) {
			cMenu_leftMargin -= GUI_CONST_TAPE_ITEMS;
			if (cMenu_leftMargin < 0)
				cMenu_leftMargin = 0;
		}

		if (i >= (cMenu_leftMargin + GUI_CONST_TAPE_ITEMS))
			cMenu_leftMargin = i - GUI_CONST_TAPE_ITEMS + 1;

		cMenu_hilite = i;
		DrawTapeDialogItems();
	}
}
//-----------------------------------------------------------------------------
