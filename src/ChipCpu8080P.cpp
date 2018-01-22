/*	ChipCpu8080P.cpp: Derived class of slowed-down i8080 microprocessor
	Copyright (c) 2015 Roman Borik <pmd85emu@gmail.com>

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
