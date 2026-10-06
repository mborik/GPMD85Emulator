/*	Debugger.cpp: Class for built-in tracing and debugging CPU activity.
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
#include "Debugger.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
TDebugger *Debugger;
//-----------------------------------------------------------------------------
#define radix Settings->Debugger->hex
//-----------------------------------------------------------------------------
// ! - undocumented instructions
// % - 8-bit operand
// & - 16-bit operand / address
// * - 16-bit jump address
char TDebugger::instr8080[256][11] = {
	" NOP",      " LXI  B,&",  " STAX B",   " INX  B",   " INR  B",   " DCR  B",   " MVI  B,%", " RLC",
	"!NOP",      " DAD  B",    " LDAX B",   " DCX  B",   " INR  C",   " DCR  C",   " MVI  C,%", " RRC",
	"!NOP",      " LXI  D,&",  " STAX D",   " INX  D",   " INR  D",   " DCR  D",   " MVI  D,%", " RAL",
	"!NOP",      " DAD  D",    " LDAX D",   " DCX  D",   " INR  E",   " DCR  E",   " MVI  E,%", " RAR",
	"!NOP",      " LXI  H,&",  " SHLD &",   " INX  H",   " INR  H",   " DCR  H",   " MVI  H,%", " DAA",
	"!NOP",      " DAD  H",    " LHLD &",   " DCX  H",   " INR  L",   " DCR  L",   " MVI  L,%", " CMA",
	"!NOP",      " LXI  SP,&", " STA  &",   " INX  SP",  " INR  M",   " DCR  M",   " MVI  M,%", " STC",
	"!NOP",      " DAD  SP",   " LDA  &",   " DCX  SP",  " INR  A",   " DCR  A",   " MVI  A,%", " CMC",

	" MOV  B,B", " MOV  B,C",  " MOV  B,D", " MOV  B,E", " MOV  B,H", " MOV  B,L", " MOV  B,M", " MOV  B,A",
	" MOV  C,B", " MOV  C,C",  " MOV  C,D", " MOV  C,E", " MOV  C,H", " MOV  C,L", " MOV  C,M", " MOV  C,A",
	" MOV  D,B", " MOV  D,C",  " MOV  D,D", " MOV  D,E", " MOV  D,H", " MOV  D,L", " MOV  D,M", " MOV  D,A",
	" MOV  E,B", " MOV  E,C",  " MOV  E,D", " MOV  E,E", " MOV  E,H", " MOV  E,L", " MOV  E,M", " MOV  E,A",
	" MOV  H,B", " MOV  H,C",  " MOV  H,D", " MOV  H,E", " MOV  H,H", " MOV  H,L", " MOV  H,M", " MOV  H,A",
	" MOV  L,B", " MOV  L,C",  " MOV  L,D", " MOV  L,E", " MOV  L,H", " MOV  L,L", " MOV  L,M", " MOV  L,A",
	" MOV  M,B", " MOV  M,C",  " MOV  M,D", " MOV  M,E", " MOV  M,H", " MOV  M,L", " HLT",      " MOV  M,A",
	" MOV  A,B", " MOV  A,C",  " MOV  A,D", " MOV  A,E", " MOV  A,H", " MOV  A,L", " MOV  A,M", " MOV  A,A",

	" ADD  B",   " ADD  C",    " ADD  D",   " ADD  E",   " ADD  H",   " ADD  L",   " ADD  M",   " ADD  A",
	" ADC  B",   " ADC  C",    " ADC  D",   " ADC  E",   " ADC  H",   " ADC  L",   " ADC  M",   " ADC  A",
	" SUB  B",   " SUB  C",    " SUB  D",   " SUB  E",   " SUB  H",   " SUB  L",   " SUB  M",   " SUB  A",
	" SBB  B",   " SBB  C",    " SBB  D",   " SBB  E",   " SBB  H",   " SBB  L",   " SBB  M",   " SBB  A",
	" ANA  B",   " ANA  C",    " ANA  D",   " ANA  E",   " ANA  H",   " ANA  L",   " ANA  M",   " ANA  A",
	" XRA  B",   " XRA  C",    " XRA  D",   " XRA  E",   " XRA  H",   " XRA  L",   " XRA  M",   " XRA  A",
	" ORA  B",   " ORA  C",    " ORA  D",   " ORA  E",   " ORA  H",   " ORA  L",   " ORA  M",   " ORA  A",
	" CMP  B",   " CMP  C",    " CMP  D",   " CMP  E",   " CMP  H",   " CMP  L",   " CMP  M",   " CMP  A",

	" RNZ",      " POP  B",    " JNZ  *",   " JMP  *",   " CNZ  *",   " PUSH B",   " ADI  %",   " RST  0",
	" RZ",       " RET",       " JZ   *",   "!JMP  *",   " CZ   *",   " CALL *",   " ACI  %",   " RST  1",
	" RNC",      " POP  D",    " JNC  *",   " OUT  %",   " CNC  *",   " PUSH D",   " SUI  %",   " RST  2",
	" RC",       "!RET",       " JC   *",   " IN   %",   " CC   *",   "!CALL *",   " SBI  %",   " RST  3",
	" RPO",      " POP  H",    " JPO  *",   " XTHL",     " CPO  *",   " PUSH H",   " ANI  %",   " RST  4",
	" RPE",      " PCHL",      " JPE  *",   " XCHG",     " CPE  *",   "!CALL *",   " XRI  %",   " RST  5",
	" RP",       " POP  PSW",  " JP   *",   " DI",       " CP   *",   " PUSH PSW", " ORI  %",   " RST  6",
	" RM",       " SPHL",      " JM   *",   " EI",       " CM   *",   "!CALL *",   " CPI  %",   " RST  7"
};
//---------------------------------------------------------------------------
// ! - undocumented instructions
// % - 8-bit operand
// & - 16-bit operand / address
// * - 16-bit jump address
// @ - address in RST instruction
char TDebugger::instrZ80[256][14] = {
	" nop",         " ld   bc,&",   " ld   (bc),a", " inc  bc",      " inc  b",      " dec  b",      " ld   b,%",    " rlca",
	"!nop",         " add  hl,bc",  " ld   a,(bc)", " dec  bc",      " inc  c",      " dec  c",      " ld   c,%",    " rrca",
	"!nop",         " ld   de,&",   " ld   (de),a", " inc  de",      " inc  d",      " dec  d",      " ld   d,%",    " rla",
	"!nop",         " add  hl,de",  " ld   a,(de)", " dec  de",      " inc  e",      " dec  e",      " ld   e,%",    " rra",
	"!nop",         " ld   hl,&",   " ld   (&),hl", " inc  hl",      " inc  h",      " dec  h",      " ld   h,%",    " daa",
	"!nop",         " add  hl,hl",  " ld   hl,(&)", " dec  hl",      " inc  l",      " dec  l",      " ld   l,%",    " cpl",
	"!nop",         " ld   sp,&",   " ld   (&),a",  " inc  sp",      " inc  (hl)",   " dec  (hl)",   " ld   (hl),%", " scf",
	"!nop",         " add  hl,sp",  " ld   a,(&)",  " dec  sp",      " inc  a",      " dec  a",      " ld   a,%",    " ccf",

	" ld   b,b",    " ld   b,c",    " ld   b,d",    " ld   b,e",     " ld   b,h",    " ld   b,l",    " ld   b,(hl)", " ld   b,a",
	" ld   c,b",    " ld   c,c",    " ld   c,d",    " ld   c,e",     " ld   c,h",    " ld   c,l",    " ld   c,(hl)", " ld   c,a",
	" ld   d,b",    " ld   d,c",    " ld   d,d",    " ld   d,e",     " ld   d,h",    " ld   d,l",    " ld   d,(hl)", " ld   d,a",
	" ld   e,b",    " ld   e,c",    " ld   e,d",    " ld   e,e",     " ld   e,h",    " ld   e,l",    " ld   e,(hl)", " ld   e,a",
	" ld   h,b",    " ld   h,c",    " ld   h,d",    " ld   h,e",     " ld   h,h",    " ld   h,l",    " ld   h,(hl)", " ld   h,a",
	" ld   l,b",    " ld   l,c",    " ld   l,d",    " ld   l,e",     " ld   l,h",    " ld   l,l",    " ld   l,(hl)", " ld   l,a",
	" ld   (hl),b", " ld   (hl),c", " ld   (hl),d", " ld   (hl),e",  " ld   (hl),h", " ld   (hl),l", " halt",        " ld   (hl),a",
	" ld   a,b",    " ld   a,c",    " ld   a,d",    " ld   a,e",     " ld   a,h",    " ld   a,l",    " ld   a,(hl)", " ld   a,a",

	" add  a,b",    " add  a,c",    " add  a,d",    " add  a,e",     " add  a,h",    " add  a,l",    " add  a,(hl)", " add  a,a",
	" adc  a,b",    " adc  a,c",    " adc  a,d",    " adc  a,e",     " adc  a,h",    " adc  a,l",    " adc  a,(hl)", " adc  a,a",
	" sub  b",      " sub  c",      " sub  d",      " sub  e",       " sub  h",      " sub  l",      " sub  (hl)",   " sub  a",
	" sbc  a,b",    " sbc  a,c",    " sbc  a,d",    " sbc  a,e",     " sbc  a,h",    " sbc  a,l",    " sbc  a,(hl)", " sbc  a,a",
	" and  b",      " and  c",      " and  d",      " and  e",       " and  h",      " and  l",      " and  (hl)",   " and  a",
	" xor  b",      " xor  c",      " xor  d",      " xor  e",       " xor  h",      " xor  l",      " xor  (hl)",   " xor  a",
	" or   b",      " or   c",      " or   d",      " or   e",       " or   h",      " or   l",      " or   (hl)",   " or   a",
	" cp   b",      " cp   c",      " cp   d",      " cp   e",       " cp   h",      " cp   l",      " cp   (hl)",   " cp   a",

	" ret  nz",     " pop  bc",     " jp   nz,*",   " jp   *",       " call nz,*",   " push bc",     " add  a,%",    " rst  @",
	" ret  z",      " ret",         " jp   z,*",    "!jp   *",       " call z,*",    " call *",      " adc  a,%",    " rst  @",
	" ret  nc",     " pop  de",     " jp   nc,*",   " out  (%),a",   " call nc,*",   " push de",     " sub  %",      " rst  @",
	" ret  c",      "!ret",         " jp   c,*",    " in   a,(%)",   " call c,*",    "!call *",      " sbc  a,%",    " rst  @",
	" ret  po",     " pop  hl",     " jp   po,*",   " ex   (sp),hl", " call po,*",   " push hl",     " and  %",      " rst  @",
	" ret  pe",     " jp   (hl)",   " jp   pe,*",   " ex   de,hl",   " call pe,*",   "!call *",      " xor  %",      " rst  @",
	" ret  p",      " pop  af",     " jp   p,*",    " di",           " call p,*",    " push af",     " or   %",      " rst  @",
	" ret  m",      " ld   sp,hl",  " jp   m,*",    " ei",           " call m,*",    "!call *",      " cp   %",      " rst  @"
};
//-----------------------------------------------------------------------------
char TDebugger::asm8080[][5] = {
	/*  0 */ "NOP", "MOV", "OUT", "IN", "DI", "EI", "HLT",
	/*  7 */ "XCHG", "XTHL", "SPHL", "PCHL",
	/* 11 */ "LXI", "MVI", "STAX", "LDAX", "SHLD", "LHLD", "STA", "LDA",
	/* 19 */ "INR", "DCR", "INX", "DCX", "DAD", "POP", "PUSH", "RST",
	/* 27 */ "RLC", "RRC", "RAL", "RAR", "DAA", "CMA", "STC", "CMC",
	/* 35 */ "ADD", "ADC", "SUB", "SBB", "ANA", "XRA", "ORA", "CMP",
	/* 43 */ "ADI", "ACI", "SUI", "SBI", "ANI", "XRI", "ORI", "CPI",
	/* 51 */ "JMP", "JNZ", "JZ", "JNC", "JC", "JPO", "JPE", "JP", "JM",
	/* 60 */ "CALL", "CNZ", "CZ", "CNC", "CC", "CPO", "CPE", "CP", "CM",
	/* 69 */ "RET", "RNZ", "RZ", "RNC", "RC", "RPO", "RPE", "RP", "RM",
	/* 78 */ "B", "C", "D", "E", "H", "L", "M", "A",
	/* 86 */ "B", "D", "H", "SP", "PSW"
};
//-----------------------------------------------------------------------------
char TDebugger::asmZ80[][5] = {
	/*  0 */ "nop", "ld", "out", "in", "di", "ei", "halt", "ex",
	/*  8 */ "inc", "dec", "pop", "push", "rst",
	/* 13 */ "rlca", "rrca", "rla", "rra", "daa", "cpl", "scf", "ccf",
	/* 21 */ "add", "adc", "sub", "sbc", "and", "xor", "or", "cp",
	/* 29 */ "jp", "call", "ret", "nz", "z", "nc", "c", "po", "pe", "p", "m",
	/* 40 */ "b", "c", "d", "e", "h", "l", "(hl)", "a",
	/* 48 */ "bc", "de", "hl", "sp", "af",
	/* 53 */ "(bc)", "(de)", "(sp)"
};
//-----------------------------------------------------------------------------
char TDebugger::regs[6][3] = { "AF", "BC", "DE", "HL", "PC", "SP" };
//-----------------------------------------------------------------------------
TDebugger::TDebugger()
{
	cpu = NULL;
	memory = NULL;

	cpuTraceCur = 0;
	cpuTraceTop = 0;
	cpuTraceFlags = 0;
	cpuCursorY = -1U;
	cpuNextPC = -1U;
	memset(cpuPCTrace, 0, sizeof(cpuPCTrace));

	reqUpdateRefresh = URQ_FORCE;
	currentNumberOfLines = 0;
	nestDepth = 0;

	int ii;
	for (ii = 0; ii < MAX_NESTINGS; ii++) {
		na[ii].addr = 0;
		na[ii].offset = 0;
	}

	for (ii = 0; ii < MAX_BREAK_POINTS; ii++) {
		if (ii) {
			bp[ii].addr = Settings->Debugger->breakpoint[ii - 1].memory & 0xFFFF;
			bp[ii].active = Settings->Debugger->breakpoint[ii - 1].active;
		}
		else {
			bp[ii].addr = 0;
			bp[ii].active = false;
		}
	}

	emulationControl = DBGCTL_RUNNING;
}
//-----------------------------------------------------------------------------
void TDebugger::SetParams(ChipCpu8080 *cpu, ChipMemory *mem, TComputerModel model)
{
	this->cpu = cpu;
	this->memory = mem;
	this->model = model;
}
//-----------------------------------------------------------------------------
void TDebugger::Reset()
{
	emulationControl = DBGCTL_STOPPED;
	Emulator->ActionPlayPause(false, false);

	bp[0].addr = 0;
	bp[0].active = false;
	reqUpdateRefresh = URQ_LOAD_PC;
}
//-----------------------------------------------------------------------------
BYTE TDebugger::GetMemState(int addr, BYTE *value)
{
	BYTE state;
	memory->GetMemState(addr, &state, value);
	return state;
}
//-----------------------------------------------------------------------------
unsigned TDebugger::GetFlagState()
{
	static const BYTE flags[] = { 0x40, 0x01, 0x04, 0x80 }; // ZF,CF,PV,SF
	WORD readptr = cpu->GetPC();
	BYTE opcode = memory->ReadByte(readptr),
		 fstate = cpu->GetAF() & 0xFF;

	auto doRet = [&]() -> unsigned {
		WORD ptr = cpu->GetSP();
		unsigned fl = TWF_BRANCH | TWF_BRADDR;
		fl |= memory->ReadByte(ptr++);
		fl |= memory->ReadByte(ptr) << 8;
		return fl;
	};
	auto doJump = [&](unsigned fl = 0) -> unsigned {
		fl |= TWF_BRANCH;
		fl |= memory->ReadByte(++readptr);
		fl |= memory->ReadByte(++readptr) << 8;
		return fl;
	};

	if (opcode == 0xE9) // jp (hl)
		return cpu->GetHL() | TWF_BRANCH | TWF_BRADDR;

	if (opcode == 0x76) // halt
		return TWF_HALTCMD | ((cpu->IsInterruptEnabled()) ? 0x38 : 0);

	if (opcode == 0xC9) // ret
		return doRet();

	if (opcode == 0xC3) // jp
		return doJump();

	if (opcode == 0xCD) // call
		return doJump(TWF_CALLCMD);

	if ((opcode & 0xC1) == 0xC0) {
		BYTE flag = flags[(opcode >> 4) & 3];
		BYTE res = fstate & flag;

		if (!(opcode & 0x08))
			res ^= flag;
		if (!res)
			return 0;

		// ret cc
		if ((opcode & 0xC7) == 0xC0)
			return doRet();
		// call cc
		if ((opcode & 0xC7) == 0xC4)
			return doJump(TWF_CALLCMD);
		// jp cc
		if ((opcode & 0xC7) == 0xC2)
			return doJump(TWF_LOOPCMD);
	}

	// rst #xx
	if ((opcode & 0xC7) == 0xC7)
		return (opcode & 0x38) | TWF_CALLCMD | TWF_BRANCH;

	return 0;
}
//-----------------------------------------------------------------------------
char *TDebugger::MakeInstrLine(WORD *addr)
{
	BYTE opcode = memory->ReadByte(*addr);
	WORD oper = memory->ReadWord(*addr + 1);
	int ilen = cpu->GetLength(opcode);
	unsigned i = 1, j = 15, l;

	memset(lineBuffer, ' ', j);
	i += sprintf(lineBuffer + i, radix ? "#%04X" : "%05d", *addr);

	lineBuffer[i] = ' ';
	i = 7;

	switch (ilen) {
		default:
		case 1:
			i += sprintf(lineBuffer + i, "%02X", opcode);
			break;
		case 2:
			i += sprintf(lineBuffer + i, "%02X%02X", opcode, (oper & 0xff));
			break;
		case 3:
			i += sprintf(lineBuffer + i, "%02X%02X%02X",
				opcode, (oper & 0xff), ((oper >> 8) & 0xff));
			break;
	}

	lineBuffer[i] = ' ';

	char *mnemo = (Settings->Debugger->z80) ? instrZ80[opcode] : instr8080[opcode];
	l = strlen(mnemo);

	for (i = 0; i < l; i++) {
		switch (mnemo[i]) {
			case '@':
				oper = (WORD)(opcode & 0x38);
			// ... and continue in next case

			case '%':
				j += std::sprintf(lineBuffer + j, radix ? "#%02X" : "%3d", (oper & 0xff));
				break;

			case '*':
			case '&':
				j += std::sprintf(lineBuffer + j, radix ? "#%04X" : "%5d", oper);
				break;

			default :
				lineBuffer[j++] = mnemo[i];
				break;
		}
	}

	lineBuffer[j] = '\0';
	*addr += (WORD) ilen;
	return lineBuffer;
}
//-----------------------------------------------------------------------------
WORD TDebugger::FindPreviousInstruction(WORD pc, int howmany)
{
	while (howmany-- > 0) {
		pc -= (WORD) 3;
		if (cpu->GetLength(memory->ReadByte(pc)) == 3)
			continue;
		pc++;
		if (cpu->GetLength(memory->ReadByte(pc)) == 2)
			continue;
		pc++;
	}

	return pc;
}
//-----------------------------------------------------------------------------
WORD TDebugger::FindNextInstruction(WORD pc, int howmany)
{
	while (howmany-- > 0)
		pc += (WORD) cpu->GetLength(memory->ReadByte(pc));

	return pc;
}
//-----------------------------------------------------------------------------
const char *TDebugger::GetMemoryState()
{
	static char allRAMState[16] = {0};

	if (memory == NULL)
		return "";
	if (memory->IsInReset())
		return "Memory Reset";

	std::snprintf(allRAMState, sizeof(allRAMState), "AllRAM:%s ",
		memory->HasAllRAM() ? (memory->IsAllRAM() ? "1" : "0") : "-");

	if (memory->IsMem256())
		std::snprintf(allRAMState + 9, 4, "P:%x", memory->GetPage());
	else if (model == CM_C2717) {
		// if (systemPIO->width384) // TODO
		// 	strcpy(allRAMState + 9, "384");
		std::snprintf(allRAMState + 9, 4, "R:%d",
			memory->IsRemapped() ? memory->GetRemapType() : 0);
	}

	return allRAMState;
}
//-----------------------------------------------------------------------------
void TDebugger::FillDisass(std::vector<TDisassLine> &result, unsigned numberOfItems)
{
	if (cpu == NULL || memory == NULL)
		return;

	unsigned newNumberOfLines = SDL_min(numberOfItems, MAX_TRACE_LINES - 1);
	if (newNumberOfLines != currentNumberOfLines) {
		// if number of lines was changed, fix cpuTraceTop+cpuTraceCur (to keep in view)
		currentNumberOfLines = newNumberOfLines;
		RefreshRequest(true);
	}

	result.resize(currentNumberOfLines);

	WORD realPC = cpu->GetPC(), nextPC;
	unsigned ii, pc;

	cpuTraceFlags = GetFlagState();
	cpuNextPC = cpuCursorY = -1U;
	pc = cpuTraceTop;

	for (ii = 0; ii < currentNumberOfLines; ii++) {
		TDisassLine disassLine;
		disassLine.color = COL_NORMAL;
		disassLine.isBreakPoint = false;
		disassLine.isBranch = false;
		disassLine.isBranchFwdDir = false;
		disassLine.isBranchTarget = false;
		disassLine.isBranchSource = false;
		disassLine.branchTarget = 0;

		nextPC = (pc &= 0xFFFF);
		cpuPCTrace[ii] = nextPC;

		disassLine.text = MakeInstrLine(&nextPC);

		if (pc == cpuTraceCur) {
			disassLine.color = COL_CURSOR;
			cpuCursorY = ii;
		}

		if (pc == realPC) {
			disassLine.color = COL_CURRENT;
			if (cpuTraceFlags & TWF_STEPOVR)
				cpuNextPC = nextPC;
		}

		if (CheckBreakPoint(pc, true)) {
			disassLine.isBreakPoint = true;
			if (pc == realPC)
				disassLine.color = COL_BREAKPT;
		}

		if (cpuTraceFlags & TWF_BRANCH) {
			if (pc == realPC) {
				disassLine.isBranch = true;

				unsigned addr = cpuTraceFlags & 0xFFFF;
				disassLine.isBranchFwdDir = (addr > pc);

				if (cpuTraceFlags & TWF_BRADDR) {
					disassLine.isBranchTarget = true;
					disassLine.branchTarget = (WORD) addr;
				}
			}
			else if (pc == (cpuTraceFlags & 0xFFFF))
				disassLine.isBranch = disassLine.isBranchSource = true;
		}

		result[ii] = disassLine;
		pc = nextPC;
	}

	cpuPCTrace[ii] = pc;
}
//-----------------------------------------------------------------------------
void TDebugger::FillRegs(std::vector<std::string> &result, bool memEdit)
{
	if (cpu == NULL || memory == NULL)
		return;
	const char *fmt = radix ? "%s:#%04X" : "%s:%05d";
	if (memEdit)
		fmt = "%s:%04X";

	result.clear();
	for (int i = 0, value = 0; i < 6; i++) {
		switch (i) {
			case 0:
				value = cpu->GetAF();
				break;
			case 1:
				value = cpu->GetBC();
				break;
			case 2:
				value = cpu->GetDE();
				break;
			case 3:
				value = cpu->GetHL();
				break;
			case 4:
				value = cpu->GetPC();
				break;
			case 5:
				value = cpu->GetSP();
				break;
		}

		std::string regLine(10, 0);
		std::snprintf(regLine.data(), regLine.size(), fmt, regs[i], value);
		result.push_back(regLine);
	}
}
//-----------------------------------------------------------------------------
void TDebugger::FillFlags(std::vector<std::string> &result)
{
	BYTE val = cpu->GetAF();

	result.clear();
	result.push_back((val & FLAG_S)  ? " M" : " P");
	result.push_back((val & FLAG_Z)  ? " Z" : "NZ");
	result.push_back((val & FLAG_AC) ? "AC" : "NA");
	result.push_back((val & FLAG_PE) ? "PE" : "PO");
	result.push_back((val & FLAG_CY) ? " C" : "NC");
	result.push_back(cpu->IsInterruptEnabled() ? "EI" : "DI");
}
//-----------------------------------------------------------------------------
void TDebugger::FillStack(std::vector<std::string> &result)
{
	static const char offsetPrefixes[6][6] = {
		"SP-02", "SP+00", "SP+02", "SP+04", "SP+06", "SP+08"
	};

	WORD val = cpu->GetSP() - 2;

	result.clear();
	for (int i = 0; i < 6; i++, val += 2) {
		std::string stackLine(16, 0);
		std::snprintf(stackLine.data(), stackLine.size(),
			(radix ? "%s: #%04X" : "%s: %05d"),
			offsetPrefixes[i], memory->ReadWord(val));
		result.push_back(stackLine);
	}
}
//-----------------------------------------------------------------------------
void TDebugger::FillNestings(std::vector<std::string> &result)
{
	result.clear();
	for (int i = 0; i < nestDepth; i++) {
		std::string nestLine(6, 0);
		std::snprintf(nestLine.data(), nestLine.size(),
			(radix ? "#%04X" : "%05d"), na[i].addr);
		result.push_back(nestLine);
	}
}
//-----------------------------------------------------------------------------
void TDebugger::FillBreakpoints(std::vector<std::pair<std::string, bool>> &result)
{
	if (cpu == NULL || memory == NULL)
		return;

	result.clear();
	std::string lineBuffer(16, 0);
	for (int i = 1; i < MAX_BREAK_POINTS; i++) {
		std::snprintf(lineBuffer.data(), lineBuffer.size(),
			(radix ? "BP%d:#%04X" : "BP%d:%05d"), i, bp[i].addr);
		result.emplace_back(lineBuffer, bp[i].active);
	}
}
//-----------------------------------------------------------------------------
void TDebugger::FillWatchMemory(std::vector<std::string> &result, unsigned numberOfItems)
{
	std::string line(16, 0);
	const char *reg;
	WORD addr;

	switch (Settings->Debugger->listSource) {
		case 0: // MEM
			addr = Settings->Debugger->listMemoryAddress;
			reg = "MEM";
			break;
		case 4: // SP
			addr = cpu->GetSP();
			reg = regs[5];
			break;
		case 5: // PC
			addr = cpu->GetPC();
			reg = regs[4];
			break;
		case 1: // HL
			addr = cpu->GetHL();
			reg = regs[3];
			break;
		case 2: // DE
			addr = cpu->GetDE();
			reg = regs[2];
			break;
		case 3: // BC
			addr = cpu->GetBC();
			reg = regs[1];
			break;
	}

	std::snprintf(
		line.data(), line.size(),
		(radix ? "%s:#%04X" : "%s:%05d"),
		reg, addr
	);

	result.clear();
	result.push_back(line);

	int i = 0, offset = Settings->Debugger->listOffset;
	for (; i < numberOfItems; i++, offset += 2) {
		WORD val = memory->ReadWord(addr + offset);
		BYTE lo = val & 0xFF;
		BYTE hi = (val >> 8) & 0xFF;
		char ch1 = (lo >= 32 && lo <= 126) ? lo : '.';
		char ch2 = (hi >= 32 && hi <= 126) ? hi : '.';

		std::snprintf(
			line.data(), line.size(),
			"%c%02X %02X %02X %c%c",
			offset < 0 ? '-' : '+', abs(offset),
			lo, hi, ch1, ch2
		);
		result.push_back(line);
	}
}
//-----------------------------------------------------------------------------
void TDebugger::RefreshRequest(bool firstTime)
{
	if (reqUpdateRefresh & URQ_LOAD_PC || firstTime) {
		WORD newpc = cpu->GetPC();
		cpuTraceCur = newpc;

		if (firstTime ||
			newpc < cpuTraceTop ||
			newpc >= cpuPCTrace[currentNumberOfLines] ||
			cpuCursorY == -1U)

			cpuTraceTop = newpc;
	}
	else if (reqUpdateRefresh & URQ_PAGE_SW)
		cpuTraceCur = cpuPCTrace[reqUpdateRefresh & (URQ_LOAD_PC - 1)];

	if (reqUpdateRefresh & URQ_BREAKPT)
		bp[0].active = false;

	reqUpdateRefresh = 0;
}
//---------------------------------------------------------------------------
void TDebugger::HandleKeyboardInput(TKeyInput key)
{
	unsigned i, curs = 0;

	auto updatePCTrace = [&]() {
		WORD nextPC;
		unsigned ii, pc = cpuTraceTop;

		cpuCursorY = -1U;
		for (ii = 0; ii < currentNumberOfLines; ii++) {
			nextPC = (pc &= 0xFFFF);
			cpuPCTrace[ii] = nextPC;

			MakeInstrLine(&nextPC);
			if (pc == cpuTraceCur)
				cpuCursorY = ii;

			pc = nextPC;
		}

		cpuPCTrace[ii] = pc;
	};

	if (key == K_UP) {
		if (cpuTraceCur > cpuTraceTop) {
			for (i = 1; i < currentNumberOfLines; i++) {
				if (cpuPCTrace[i] == cpuTraceCur)
					cpuTraceCur = cpuPCTrace[i - 1];
			}
		}
		else
			cpuTraceTop = cpuTraceCur = FindPreviousInstruction(cpuTraceCur, 1);
	}
	else if (key == K_DOWN) {
		for (i = 0; i < currentNumberOfLines; i++) {
			if (cpuPCTrace[i] == cpuTraceCur) {
				cpuTraceCur = cpuPCTrace[i + 1];

				if (i == currentNumberOfLines - 1)
					cpuTraceTop = cpuPCTrace[1];
				break;
			}
		}
	}
	else if (key == K_PAGEUP) {
		for (i = 0; i < currentNumberOfLines; i++)
			if (cpuTraceCur == cpuPCTrace[i])
				curs = i;

		cpuTraceTop = FindPreviousInstruction(cpuTraceTop, currentNumberOfLines - 1);
		updatePCTrace();

		reqUpdateRefresh = URQ_PAGE_SW | curs;
	}
	else if (key == K_PAGEDOWN) {
		for (i = 0; i < currentNumberOfLines; i++)
			if (cpuTraceCur == cpuPCTrace[i])
				curs = i;

		cpuTraceTop = cpuPCTrace[currentNumberOfLines - 1];
		updatePCTrace();

		reqUpdateRefresh = URQ_PAGE_SW | curs;
	}
	else if (key == K_HOME) {
		cpuTraceTop = cpuTraceCur = cpu->GetPC();

		updatePCTrace();
		reqUpdateRefresh = URQ_PAGE_SW;
	}
}
//---------------------------------------------------------------------------
bool TDebugger::DoTrace(bool run)
{
	if ((run && emulationControl != DBGCTL_STOPPED) ||
	   (!run && emulationControl != DBGCTL_RUNNING))
			return Emulator->isRunning;

	if (run) {
		bp[0].active = false;
		if (CheckBreakPoint(cpu->GetPC()))
			cpu->DoInstruction();
		emulationControl = DBGCTL_TAKE_RUN;
	}
	else
		Reset();

	return Emulator->isRunning;
}
//---------------------------------------------------------------------------
void TDebugger::DoStepInto()
{
	cpu->DoInstruction();
	Reset();
}
//---------------------------------------------------------------------------
void TDebugger::DoStepOver()
{
	WORD adr = cpu->GetPC();
	BYTE opcode = memory->ReadByte(adr);

	if ((opcode & 0xCD) == 0xCD || // CALL
	    (opcode & 0xC7) == 0xC4 || // Cx
	    (opcode & 0xC7) == 0xC7) { // RST x

		bp[0].active = false;
		if (CheckBreakPoint(adr))
			cpu->DoInstruction();

		bp[0].addr = FindNextInstruction(adr, 1);
		bp[0].active = true;
		emulationControl = DBGCTL_STEP_OVER;
	}
	else {
		cpu->DoInstruction();
		Reset();
	}
}
//---------------------------------------------------------------------------
void TDebugger::DoStepOut()
{
	WORD adr = cpu->GetPC();
	BYTE opcode = memory->ReadByte(adr);
	if (opcode == 0xC9 || opcode == 0xD9 || (opcode & 0xC7) == 0xC0) { // RET, Rx
		cpu->DoInstruction();
		Reset();
		return;
	}

	wsp = cpu->GetSP();
	bp[0].active = false;
	if (CheckBreakPoint(cpu->GetPC()))
		cpu->DoInstruction();

	emulationControl = DBGCTL_STEP_OUT;
}
//---------------------------------------------------------------------------
void TDebugger::DoStepToNext()
{
	WORD adr = cpu->GetPC();

	bp[0].active = false;
	if (CheckBreakPoint(adr))
		cpu->DoInstruction();

	bp[0].addr = FindNextInstruction(adr, 1);
	bp[0].active = true;
	emulationControl = DBGCTL_TAKE_RUN;
}
//---------------------------------------------------------------------------
void TDebugger::ModifyRegister(const char *reg, const char *value)
{
	size_t gotoAddr;
	const char *fmt = radix ? "%" _pfSizeT "X" : "%" _pfSizeT "u";

	if (sscanf(value, fmt, &gotoAddr) == 1)
		ModifyRegister(reg, gotoAddr);
}
//---------------------------------------------------------------------------
void TDebugger::ModifyRegister(const char *reg, unsigned value)
{
	int regCode = -1;
	for (int i = 0; i < 6; i++) {
		if (strncmp(reg, regs[i], 2) == 0) {
			regCode = i;
			break;
		}
	}

	value &= 0xFFFF;
	switch (regCode) {
		case 0: // AF
			cpu->SetAF(value);
			break;
		case 1: // BC
			cpu->SetBC(value);
			break;
		case 2: // DE
			cpu->SetDE(value);
			break;
		case 3: // HL
			cpu->SetHL(value);
			break;
		case 4: // PC
			cpu->SetPC(value);
			break;
		case 5: // SP
			cpu->SetSP(value);
			break;
	}
}
//---------------------------------------------------------------------------
void TDebugger::ModifyFlag(int index)
{
	WORD af = cpu->GetAF();

	switch (index) {
		case 0: // Sign flag
			cpu->SetAF(af ^ FLAG_S);
			break;
		case 1: // Zero flag
			cpu->SetAF(af ^ FLAG_Z);
			break;
		case 2: // Auxiliary carry flag
			cpu->SetAF(af ^ FLAG_AC);
			break;
		case 3: // Parity flag
			cpu->SetAF(af ^ FLAG_PE);
			break;
		case 4: // Carry flag
			cpu->SetAF(af ^ FLAG_CY);
			break;
		case 5: // Interrupt flag
			cpu->SetIff(!cpu->IsInterruptEnabled());
			break;
	}
}
//---------------------------------------------------------------------------
void TDebugger::ModifyStack(int offset, const char *value)
{
	unsigned val;
	const char *fmt = radix ? "%" _pfSizeT "X" : "%" _pfSizeT "u";

	if (sscanf(value, fmt, &val) == 1)
		ModifyStack(offset, val);
}
//---------------------------------------------------------------------------
void TDebugger::ModifyStack(int offset, unsigned value)
{
	memory->WriteWord(cpu->GetSP() + offset, value);
}
//---------------------------------------------------------------------------
void TDebugger::ToggleBreakPoint(int index, bool active)
{
	ToggleBreakPoint(nullptr, index, &active);
}
//---------------------------------------------------------------------------
void TDebugger::ToggleBreakPoint(const char *addr, int index, bool *active)
{
	int newAddr = addr ? strtoul(addr, nullptr, radix ? 16 : 10) & 0xFFFF : -1;

	// if index < 0 find the one with newAddr OR first inactive breakpoint slot
	if (index < 0) {
		if (newAddr < 0)
			return;

		for (int ii = 1; ii < MAX_BREAK_POINTS; ii++) {
			if (bp[ii].addr == newAddr) {
				index = ii;
				break;
			}
		}
	}
	if (index < 0) {
		for (int ii = 1; ii < MAX_BREAK_POINTS; ii++) {
			if (!bp[ii].active) {
				index = ii;
				break;
			}
		}
	}
	if (index >= 0 && index < MAX_BREAK_POINTS) {
		if (active)
			bp[index].active = *active;
		else
			bp[index].active = !bp[index].active;

		if (newAddr >= 0)
			bp[index].addr = newAddr;
		if (index > 0) {
			Settings->Debugger->breakpoint[index - 1].active = bp[index].active;
			Settings->Debugger->breakpoint[index - 1].memory = bp[index].addr;
		}
	}
}
//---------------------------------------------------------------------------
bool TDebugger::CheckBreakPoint(WORD addr, bool userOnly)
{
	for (int ii = userOnly ? 1 : 0; ii < MAX_BREAK_POINTS; ii++)
		if (bp[ii].active && addr == bp[ii].addr)
			return true;

	return false;
}
//---------------------------------------------------------------------------
bool TDebugger::CheckDebugRet(int *t)
{
	BYTE opcode = memory->ReadByte(cpu->GetPC());
	*t = cpu->DoInstruction();

	if (opcode == 0xC9 || opcode == 0xD9 || ((opcode & 0xC7) == 0xC0 && *t == 11)) {
		if ((emulationControl == DBGCTL_STEP_OVER && wsp == cpu->GetSP()) ||
		    (emulationControl == DBGCTL_STEP_OUT && wsp < cpu->GetSP()))
				return true;
	}

	return false;
}
//---------------------------------------------------------------------------
