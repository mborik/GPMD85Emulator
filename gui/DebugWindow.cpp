/*	DebugWindow.cpp: Part of GUI rendering class: Debugger Window
	Copyright (c) 2012 Martin Borik <mborik@users.sourceforge.net>

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
void UserInterface::drawDebugWidgetDisass(SDL_Rect *r, bool full)
{
	int mx = r->x + (2 * fontWidth),
		my = r->y + (GUI_CONST_BORDER / 2),
		l1 = (full ? 8 : 4), l2 = (full ? 25 : 12);

	r->w = (33 * fontWidth) + 1;
	r->h = (l2 * fontLineHeight) + GUI_CONST_BORDER;

	drawDebugFrame(defaultSurface, r->x, r->y, r->w, r->h);
	drawRectangle(defaultSurface, r->x, my + (l1 * fontLineHeight) - 1,
			r->w - 1, fontLineHeight, GUI_COLOR_DBG_CURSOR);

	BYTE b = l1;
	char *line = NULL;
	for (int i = 0; i < l2; i++) {
		line = Debugger->FillDisass(&b);

		if (line)
			printText(defaultSurface, mx, my, GUI_COLOR_DBG_TEXT, line);
		if (b)
			printChar(defaultSurface, mx - GUI_CONST_HOTKEYCHAR, my,
					GUI_COLOR_HIGHLIGHT, SCHR_NAVIGATOR);

		my += fontLineHeight;
	}

	r->x += r->w + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::drawDebugWidgetRegs(SDL_Rect *r)
{
	int mx, my = r->y + (GUI_CONST_BORDER / 2);

	r->w = (10 * fontWidth);
	r->h = (6 * fontLineHeight) + GUI_CONST_BORDER;
	mx = r->x + r->w;

	drawDebugFrame(defaultSurface, r->x, r->y, r->w, r->h);
	drawRectangle(defaultSurface, r->x, my + (4 * fontLineHeight) - 1,
			r->w - 1, fontLineHeight, GUI_COLOR_DBG_CURSOR);
	printText(defaultSurface, r->x + fontWidth, my,
			GUI_COLOR_DBG_TEXT, Debugger->FillRegs());

	r->w = (4 * fontWidth);
	drawDebugFrame(defaultSurface, mx, r->y, r->w, r->h);
	printText(defaultSurface, mx + fontWidth, r->y + (GUI_CONST_BORDER / 2),
			GUI_COLOR_DBG_TEXT, Debugger->FillFlags());

	r->y += r->h + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::drawDebugWidgetStack(SDL_Rect *r)
{
	int my = r->y + (GUI_CONST_BORDER / 2);

	r->w = (14 * fontWidth);
	r->h = (5 * fontLineHeight) + GUI_CONST_BORDER - 1;

	drawDebugFrame(defaultSurface, r->x, r->y, r->w, r->h);
	drawRectangle(defaultSurface, r->x, my + fontLineHeight - 1,
			r->w - 1, fontLineHeight, GUI_COLOR_DBG_CURSOR);
	printText(defaultSurface, r->x + fontWidth, my,
			GUI_COLOR_DBG_TEXT, Debugger->FillStack());

	r->y += r->h + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::drawDebugWidgetBreaks(SDL_Rect *r)
{
	int mx = r->x + (2 * fontWidth),
		my = r->y + (GUI_CONST_BORDER / 2);

	r->w = 284;
	r->h = fontLineHeight + GUI_CONST_BORDER;

	drawDebugFrame(defaultSurface, r->x, r->y, r->w + 1, r->h);

	BYTE b = -1;
	char *line = NULL;
	for (int i = 0; i < 6; i++) {
		line = Debugger->FillBreakpoints(&b);

		if (line)
			printText(defaultSurface, mx, my, GUI_COLOR_DBG_TEXT, line);

		printCheck(defaultSurface, mx - GUI_CONST_HOTKEYCHAR + 1, my + 1,
				GUI_COLOR_CHECKED, SCHR_CHECK, (bool) b);

		mx += fontWidth * 8;
	}

	r->y += r->h + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::drawDebugWindow()
{
	cMenu_data = NULL;
	cMenu_hilite = cMenu_leftMargin = cMenu_count = 0;

	int base;
	SDL_Rect *r = new SDL_Rect;
	memset(defaultSurface->pixels, 0, frameLength);

	r->x = base = (defaultSurface->clip_rect.w - 284) / 2;
	r->y = (defaultSurface->clip_rect.h - 252) / 2;

	drawDebugWidgetDisass(r, (Settings->Debugger->listType == DL_DISASM));
	drawDebugWidgetRegs(r);
	drawDebugWidgetStack(r);

	if (Settings->Debugger->listType == DL_DUMP ||
		Settings->Debugger->listType == DL_ASCII) {

		r->x = base;
		drawDebugWidgetBreaks(r);
	}

	delete r;
	needRedraw = true;
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerDebugWindow(WORD key)
{
	bool UNUSED_VARIABLE change = false;

	switch (key) {
		case SDLK_F1 | KM_ALT:
			key = SDLK_MENU;
			break;
		case SDLK_F4 | KM_ALT:
			key = SDLK_POWER;
			break;
		default:
			key &= (KM_ALT ^ 0xFFFF);
			break;
	}

	switch (key) {
		case SDLK_POWER:
			Emulator->ActionExit();
			menuCloseAll();
			needRelease = true;
			return;

		case SDLK_ESCAPE:
			menuClose();
			needRelease = true;
			return;

		case SDLK_TAB:
			if (Settings->Debugger->listType == DL_DUMP)
				Settings->Debugger->listType = DL_ASCII;
			else if (Settings->Debugger->listType == DL_ASCII)
				Settings->Debugger->listType = DL_DISASM;
			else if (Settings->Debugger->listType == DL_DISASM)
				Settings->Debugger->listType = DL_DUMP;

			drawDebugWindow();
			change = true;
			break;

		case SDLK_h | KM_CTRL:
			Settings->Debugger->hex = !Settings->Debugger->hex;
			drawDebugWindow();
			change = true;
			break;

		case SDLK_z | KM_CTRL:
			Settings->Debugger->z80 = !Settings->Debugger->z80;
			drawDebugWindow();
			change = true;
			break;

		case SDLK_F7 | KM_SHIFT:
			Debugger->DoStepToNext();
			break;

		case SDLK_F7:
			Debugger->DoStepInto();
			drawDebugWindow();
			break;

		case SDLK_F8 | KM_SHIFT:
			Debugger->DoStepOut();
			break;

		case SDLK_F8:
			Debugger->DoStepOver();
			drawDebugWindow();
			break;
	}

	// controlling of emulation via debugger
	if (Debugger->flag == 8 || Debugger->flag == 9) {
		if (Debugger->flag == 8)
			Debugger->flag = 0;
		menuClose();
		needRelease = true;
		uiSetChanges |= PS_CLOSEALL;
	}
}
//-----------------------------------------------------------------------------
