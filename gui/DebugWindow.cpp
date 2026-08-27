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
void UserInterface::DrawDebugWidgetDisass(bool full)
{
/*
	int l1 = (full ? 8 : 4), l2 = (full ? 25 : 12);

	BYTE b = l1;
	char *line = NULL;
	for (int i = 0; i < l2; i++) {
		line = Debugger->FillDisass(&b);

		if (line)
			PrintText(s, mx, my, GUI_COLOR_DBG_TEXT, line);
		if (b)
			PrintChar(s, mx - GUI_CONST_HOTKEYCHAR, my,
					GUI_COLOR_HIGHLIGHT, SCHR_NAVIGATOR);
	}
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetRegs()
{
/*
	Debugger->FillRegs()
	Debugger->FillFlags()
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetStack()
{
/*
	Debugger->FillStack()
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetBreaks()
{
/*
	BYTE b = -1;
	char *line = NULL;
	for (int i = 0; i < 6; i++) {
		line = Debugger->FillBreakpoints(&b);

		if (line)
			PrintText(s, mx, my, GUI_COLOR_DBG_TEXT, line);

		PrintCheck(s, mx - GUI_CONST_HOTKEYCHAR + 1, my + 1,
				GUI_COLOR_CHECKED, SCHR_CHECK, (bool) b);
	}
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWindow()
{
/*
	DrawDebugWidgetDisass((Settings->Debugger->listType == DL_DISASM));
	DrawDebugWidgetRegs();
	DrawDebugWidgetStack();
	DrawDebugWidgetBreaks();
*/
}
//-----------------------------------------------------------------------------
