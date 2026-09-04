/*	Debugger.h: Class for built-in tracing and debugging CPU activity.
	Copyright (c) 2006-2007 Roman Borik <pmd85emu@gmail.com>
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
		BYTE GetMemState(int addr, BYTE *value = nullptr);
		inline void WriteByte(int addr, BYTE value) { memory->WriteByte(addr, value); }
		inline BYTE GetChangingBufferValue(int off) { return memory ? memory->memChanging[off] : 0; }

		char *FillDisass(BYTE *ctrl);
		char *FillRegs();
		char *FillFlags();
		char *FillStack();
		char *FillBreakpoints(BYTE *ctrl);

		void DoStepInto();
		void DoStepOver();
		void DoStepOut();
		void DoStepToNext();
		bool CheckBreakPoint(WORD addr);
		bool CheckDebugRet(int *t);
};
//-----------------------------------------------------------------------------
#endif
