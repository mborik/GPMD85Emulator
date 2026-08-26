/*	DebugWindow.cpp: Part of GUI rendering class: Debugger Window
	Copyright (c) 2012-2026 Martin Borik <martin@borik.net>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom
	the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
	OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
	OR OTHER DEALINGS IN THE SOFTWARE.
*/
//-----------------------------------------------------------------------------
#include "UserInterface.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetDisass(GUI_SURFACE *s, SDL_Rect *r, bool full)
{
	int mx = r->x + (2 * fontWidth),
		my = r->y + (GUI_CONST_BORDER / 2),
		l1 = (full ? 8 : 4), l2 = (full ? 25 : 12);

	r->w = (33 * fontWidth) + 1;
	r->h = (l2 * fontLineHeight) + GUI_CONST_BORDER;

	DrawDebugFrame(s, r->x, r->y, r->w, r->h);
	DrawRectangle(s, r->x, my + (l1 * fontLineHeight) - 1,
			r->w - 1, fontLineHeight, GUI_COLOR_DBG_CURSOR);

	BYTE b = l1;
	char *line = NULL;
	for (int i = 0; i < l2; i++) {
		line = Debugger->FillDisass(&b);

		if (line)
			PrintText(s, mx, my, GUI_COLOR_DBG_TEXT, line);
		if (b)
			PrintChar(s, mx - GUI_CONST_HOTKEYCHAR, my,
					GUI_COLOR_HIGHLIGHT, SCHR_NAVIGATOR);

		my += fontLineHeight;
	}

	r->x += r->w + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetRegs(GUI_SURFACE *s, SDL_Rect *r)
{
	int mx, my = r->y + (GUI_CONST_BORDER / 2);

	r->w = (10 * fontWidth);
	r->h = (6 * fontLineHeight) + GUI_CONST_BORDER;
	mx = r->x + r->w;

	DrawDebugFrame(s, r->x, r->y, r->w, r->h);
	DrawRectangle(s, r->x, my + (4 * fontLineHeight) - 1,
			r->w - 1, fontLineHeight, GUI_COLOR_DBG_CURSOR);
	PrintText(s, r->x + fontWidth, my,
			GUI_COLOR_DBG_TEXT, Debugger->FillRegs());

	r->w = (4 * fontWidth);
	DrawDebugFrame(s, mx, r->y, r->w, r->h);
	PrintText(s, mx + fontWidth, r->y + (GUI_CONST_BORDER / 2),
			GUI_COLOR_DBG_TEXT, Debugger->FillFlags());

	r->y += r->h + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetStack(GUI_SURFACE *s, SDL_Rect *r)
{
	int my = r->y + (GUI_CONST_BORDER / 2);

	r->w = (14 * fontWidth);
	r->h = (5 * fontLineHeight) + GUI_CONST_BORDER - 1;

	DrawDebugFrame(s, r->x, r->y, r->w, r->h);
	DrawRectangle(s, r->x, my + fontLineHeight - 1,
			r->w - 1, fontLineHeight, GUI_COLOR_DBG_CURSOR);
	PrintText(s, r->x + fontWidth, my,
			GUI_COLOR_DBG_TEXT, Debugger->FillStack());

	r->y += r->h + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetBreaks(GUI_SURFACE *s, SDL_Rect *r)
{
	int mx = r->x + (2 * fontWidth),
		my = r->y + (GUI_CONST_BORDER / 2);

	r->w = 284;
	r->h = fontLineHeight + GUI_CONST_BORDER;

	DrawDebugFrame(s, r->x, r->y, r->w + 1, r->h);

	BYTE b = -1;
	char *line = NULL;
	for (int i = 0; i < 6; i++) {
		line = Debugger->FillBreakpoints(&b);

		if (line)
			PrintText(s, mx, my, GUI_COLOR_DBG_TEXT, line);

		PrintCheck(s, mx - GUI_CONST_HOTKEYCHAR + 1, my + 1,
				GUI_COLOR_CHECKED, SCHR_CHECK, (bool) b);

		mx += fontWidth * 8;
	}

	r->y += r->h + 2;
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWindow()
{
	cMenu_data = NULL;
	cMenu_hilite = cMenu_leftMargin = cMenu_count = 0;

	int base;
	SDL_Rect *r = new SDL_Rect;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);
	memset(defaultSurface->pixels, 0, frameLength);

	r->x = base = (frameWidth - 284) / 2;
	r->y = (frameHeight - 252) / 2;

	DrawDebugWidgetDisass(defaultSurface, r, (Settings->Debugger->listType == DL_DISASM));
	DrawDebugWidgetRegs(defaultSurface, r);
	DrawDebugWidgetStack(defaultSurface, r);

	if (Settings->Debugger->listType == DL_DUMP ||
		Settings->Debugger->listType == DL_ASCII) {

		r->x = base;
		DrawDebugWidgetBreaks(defaultSurface, r);
	}

	delete r;

	UnlockSurface(defaultTexture, defaultSurface);
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerDebugWindow(WORD key)
{
	bool UNUSED_VARIABLE change = false;

	switch (key) {
		case SDL_SCANCODE_F1 | KM_ALT:
			key = SDL_SCANCODE_APPLICATION;
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
			return;

		case SDL_SCANCODE_ESCAPE:
			MenuClose();
			return;

		case SDL_SCANCODE_TAB:
			if (Settings->Debugger->listType == DL_DUMP)
				Settings->Debugger->listType = DL_ASCII;
			else if (Settings->Debugger->listType == DL_ASCII)
				Settings->Debugger->listType = DL_DISASM;
			else if (Settings->Debugger->listType == DL_DISASM)
				Settings->Debugger->listType = DL_DUMP;

			DrawDebugWindow();
			change = true;
			break;

		case SDL_SCANCODE_H | KM_CTRL:
			Settings->Debugger->hex = !Settings->Debugger->hex;
			DrawDebugWindow();
			change = true;
			break;

		case SDL_SCANCODE_Z | KM_CTRL:
			Settings->Debugger->z80 = !Settings->Debugger->z80;
			DrawDebugWindow();
			change = true;
			break;

		case SDL_SCANCODE_F7 | KM_SHIFT:
			Debugger->DoStepToNext();
			break;

		case SDL_SCANCODE_F7:
			Debugger->DoStepInto();
			DrawDebugWindow();
			break;

		case SDL_SCANCODE_F8 | KM_SHIFT:
			Debugger->DoStepOut();
			break;

		case SDL_SCANCODE_F8:
			Debugger->DoStepOver();
			DrawDebugWindow();
			break;
	}

	// controlling of emulation via debugger
	if (Debugger->flag == 8 || Debugger->flag == 9) {
		if (Debugger->flag == 8)
			Debugger->flag = 0;
		MenuClose();
		uiSetChanges |= PS_CLOSEALL;
	}
}
//-----------------------------------------------------------------------------
