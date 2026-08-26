/*	ChipCpu8080P.h: Derived class of slowed-down i8080 microprocessor
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

