/*	ChipCpu8080P.cpp: Derived class of slowed-down i8080 microprocessor
	Copyright (c) 2015 Roman Borik <pmd85emu@gmail.com>

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
#include "ChipCpu8080P.h"
//-----------------------------------------------------------------------------
// instructions T-cycles takes into account deceleration by video-processor:
// conditional Cc functions calls and Rc returns is solved separately,
//   this tick values are only if condition isn't met,
//   if some condition is met, we will add WTR5R3R3W3W3 or WTR5R3R3 ticks
int ChipCpu8080P::duration[256] = {
//           x0        x1            x2            x3            x4            x5        x6        x7
//           x8        x9            xA            xB            xC            xD        xE        xF
/* 0x */   WTR4, WTR4R3R3,       WTR4W3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
           WTR4, WTR4N3N3,       WTR4R3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
/* 1x */   WTR4, WTR4R3R3,       WTR4W3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
           WTR4, WTR4N3N3,       WTR4R3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
/* 2x */   WTR4, WTR4R3R3, WTR4R3R3W3W3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
           WTR4, WTR4N3N3, WTR4R3R3R3R3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
/* 3x */   WTR4, WTR4R3R3,   WTR4R3R3W3,         WTR5,     WTR4R3W3,     WTR4R3W3, WTR4R3W3,     WTR4,
           WTR4, WTR4N3N3,   WTR4R3R3R3,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR4,
/* 4x */   WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
           WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
/* 5x */   WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
           WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
/* 6x */   WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
           WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
/* 7x */ WTR4W3,   WTR4W3,       WTR4W3,       WTR4W3,       WTR4W3,       WTR4W3,   WTR4N1,   WTR4W3,
           WTR5,     WTR5,         WTR5,         WTR5,         WTR5,         WTR5,   WTR4R3,     WTR5,
/* 8x */   WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
           WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
/* 9x */   WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
           WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
/* Ax */   WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
           WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
/* Bx */   WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
           WTR4,     WTR4,         WTR4,         WTR4,         WTR4,         WTR4,   WTR4R3,     WTR4,
/* Cx */   WTR5, WTR4R3R3,     WTR4R3R3,     WTR4R3R3,     WTR5R3R3,     WTR5W3W3,   WTR4R3, WTR5W3W3,
           WTR5, WTR4R3R3,     WTR4R3R3,     WTR4R3R3,     WTR5R3R3, WTR5R3R3W3W3,   WTR4R3, WTR5W3W3,
/* Dx */   WTR5, WTR4R3R3,     WTR4R3R3,     WTR4R3W3,     WTR5R3R3,     WTR5W3W3,   WTR4R3, WTR5W3W3,
           WTR5, WTR4R3R3,     WTR4R3R3,     WTR4R3R3,     WTR5R3R3, WTR5R3R3W3W3,   WTR4R3, WTR5W3W3,
/* Ex */   WTR5, WTR4R3R3,     WTR4R3R3, WTR4R3R3W3W5,     WTR5R3R3,     WTR5W3W3,   WTR4R3, WTR5W3W3,
           WTR5,     WTR5,     WTR4R3R3,         WTR4,     WTR5R3R3, WTR5R3R3W3W3,   WTR4R3, WTR5W3W3,
/* Fx */   WTR5, WTR4R3R3,     WTR4R3R3,         WTR4,     WTR5R3R3,     WTR5W3W3,   WTR4R3, WTR5W3W3,
           WTR5,     WTR5,     WTR4R3R3,         WTR4,     WTR5R3R3, WTR5R3R3W3W3,   WTR4R3, WTR5W3W3
};
//-----------------------------------------------------------------------------
ChipCpu8080P::ChipCpu8080P(ChipMemory *mem) : ChipCpu8080(mem)
{
	slowCpu = true;
}
//-----------------------------------------------------------------------------
int ChipCpu8080P::GetDuration(BYTE opcode, bool jmp)
{
	if (!slowCpu)
		return ChipCpu8080::GetDuration(opcode, jmp);

	int secondMCycleSlowDown = (countTCycles & 1);
	if (!jmp)
		return duration[opcode] + secondMCycleSlowDown;
	if ((opcode & 0xC7) == 0xC0) // Rx
		return WTR5R3R3 + secondMCycleSlowDown;
//	if ((opcode & 0xC7) == 0xC4) // Cx
		return WTR5R3R3W3W3 + secondMCycleSlowDown;
}
//---------------------------------------------------------------------------
