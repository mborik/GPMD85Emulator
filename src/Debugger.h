/*	Debugger.h: Class for built-in tracing and debugging CPU activity.
	Copyright (c) 2006-2007 Roman Borik <pmd85emu@gmail.com>
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
#ifndef DEBUGGER_H_
#define DEBUGGER_H_
//-----------------------------------------------------------------------------
#define MAX_BREAK_POINTS      7   // one "stop-point", six break-points
#define MAX_NESTINGS          11
#define OFFSETS               7
//-----------------------------------------------------------------------------
#include "globals.h"
#include "ChipCpu8080.h"
#include "ChipMemory.h"
//-----------------------------------------------------------------------------
class TDebugger
{
	private:
		typedef struct {
			WORD addr;        // nesting address
			int offset;       // active offset
		} NESTING;

		typedef struct {
			WORD addr;        // breakpoint address
			bool active;      // breakpoint activity
		} BREAK_POINT;

		BREAK_POINT bp[MAX_BREAK_POINTS];   // breakpoints array
		NESTING na[MAX_NESTINGS];           // nestings array
		int  na_depth;                      // depth of nest
		WORD memadr;                        // listing address
		int  offsets[OFFSETS];              // offsets of each listing pointer

		WORD wsp;                           // stack pointer for "routine exit"

		ChipCpu8080 *cpu;
		ChipMemory *memory;
		TComputerModel model;

		static char instr8080[256][11];     // 8080 mnemonic
		static char instrZ80[256][14];      // Z80 mnemonic

		static char asm8080[][5];           // assembler instruction array
		static char asmZ80[][5];

		char lineBuffer[256];

		WORD  FindPeviousInstruction(WORD pc, int howmany);
		WORD  FindNextInstruction(WORD pc, int howmany);
		void  FillList();
		char *MakeInstrLine(WORD *addr);
		char *MakeDumpLine(WORD *addr);
		WORD  GetCurrentSourceAddress();
		void  FillBreakpoints();
		void  FillNesting();

	public:
		int flag;

		TDebugger();
		void SetParams(ChipCpu8080 *cpu, ChipMemory *mem, TComputerModel model);
		void Reset();

		char *FillDisass(BYTE *ctrl);
		char *FillRegs();
		char *FillFlags();
		char *FillStack();
		char *FillBreakpoints(BYTE *ctrl);

		void DoStepInto();
		void DoStepOver();
		void DoStepOut();
		void DoStepToNext();
		bool CheckBreakPoint(WORD adr);
		bool CheckDebugRet(int *t);
};
//-----------------------------------------------------------------------------
#endif
