/*	ChipCpu8080P.h: Derived class of slowed-down i8080 microprocessor
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
//---------------------------------------------------------------------------
#ifndef ChipCpu8080PH
#define ChipCpu8080PH
//---------------------------------------------------------------------------
#include "ChipCpu8080.h"
//---------------------------------------------------------------------------
// instruction duration definition according to the processor-cycle lengths
// (these durations takes into account deceleration by video-processor)
#define WTR4             4
#define WTR4R3           7
#define WTR4R3R3        11
#define WTR4R3R3R3      15
#define WTR4R3R3R3R3    19
#define WTR4W3           8
#define WTR4R3W3        10
#define WTR4R3R3W3      14
#define WTR4R3R3W3W3    18
#define WTR4R3R3W3W5    20
#define WTR5             5
#define WTR5R3R3        13
#define WTR5R3R3W3W3    20
#define WTR5W3W3        12
#define WTR4N3N3        10
#define WTR4N1           5
//---------------------------------------------------------------------------
class ChipCpu8080P : public ChipCpu8080 {
public:
	ChipCpu8080P(ChipMemory *mem);

private:
	static int duration[256];
	virtual int GetDuration(BYTE opcode, bool jmp);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

