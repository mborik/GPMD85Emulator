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
#define MAX_NESTINGS         11
#define MAX_TRACE_LINES     256
//-----------------------------------------------------------------------------
#define TWF_BRANCH  0x010000
#define TWF_BRADDR  0x020000
#define TWF_LOOPCMD 0x040000
#define TWF_CALLCMD 0x080000
#define TWF_BLKCMD  0x100000
#define TWF_HALTCMD 0x200000
#define TWF_STEPOVR (TWF_HALTCMD | TWF_BLKCMD | TWF_CALLCMD)
//-----------------------------------------------------------------------------
#define URQ_FORCE   0x0100
#define URQ_LOAD_PC 0x0200
#define URQ_PAGE_SW 0x0400
#define URQ_BREAKPT 0x0800
//-----------------------------------------------------------------------------
#include "globals.h"
#include "ChipCpu8080.h"
#include "ChipMemory.h"
#include <vector>
//-----------------------------------------------------------------------------
enum TDisassLineColor { COL_NORMAL, COL_CURSOR, COL_CURRENT, COL_BREAKPT };
typedef struct TDisassLine {
	std::string text;       // rendered disassembly line
	TDisassLineColor color; // color of the disassembly line
	bool isBreakPoint;      // indicates breakpoint on the line
	bool isBranch;          // indicates if the line is a branch instruction
	bool isBranchFwdDir;    // direction of the branch (true forward `v`, false backward `^`)
	bool isBranchSource;    // indicates if the line is a source of a branch
	bool isBranchTarget;    // indicates if the line is a target of a branch
	WORD branchTarget;      // address of the branch target (if applicable)
} TDisassLine;
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
		int nestDepth;                      // depth of nest

		WORD cpuPCTrace[MAX_TRACE_LINES];   // buffer for PC trace
		unsigned cpuCursorY;                // cursor position in PC trace
		unsigned cpuTraceCur, cpuTraceTop;  // trace cursor and top position
		unsigned cpuTraceFlags, cpuNextPC;  // flag state and next PC
		unsigned currentNumberOfLines;      // current number of lines in the disassembly view
		unsigned reqUpdateRefresh;          // request update refresh flags

		WORD wsp;                           // stack pointer for "routine exit"

		ChipCpu8080 *cpu;
		ChipMemory *memory;
		TComputerModel model;

		static char instr8080[256][11];     // 8080 mnemonic
		static char instrZ80[256][14];      // Z80 mnemonic

		static char asm8080[][5];           // assembler instruction array
		static char asmZ80[][5];

		char lineBuffer[256];

		unsigned GetFlagState();
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
		inline BYTE *GetChangingMemState() { return memory->GetChangingMemState(); }

		void  FillDisass(std::vector<TDisassLine> &result, unsigned numberOfItems);
		void  FillRegs(std::vector<std::string> &result, bool memEdit = false);
		char *FillFlags();
		char *FillStack();
		char *FillBreakpoints(BYTE *ctrl);

		void RefreshRequest(bool firstTime = false);
		void DoStepInto();
		void DoStepOver();
		void DoStepOut();
		void DoStepToNext();
		bool CheckBreakPoint(WORD addr);
		bool CheckDebugRet(int *t);
};
//-----------------------------------------------------------------------------
#endif
