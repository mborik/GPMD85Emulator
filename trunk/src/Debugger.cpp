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
#include "Debugger.h"
#include "GPMD85main.h"
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
// @ - address in RET instruction
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
TDebugger::TDebugger()
{
	cpu = NULL;
	memory = NULL;

	int ii;
	memadr = 0;
	for (ii = 0; ii < OFFSETS; ii++)
		offsets[ii] = 0;

	na_depth = 0;
	for (ii = 0; ii < MAX_NESTINGS; ii++) {
		na[ii].addr = 0;
		na[ii].offset = 0;
	}

	for (ii = 0; ii < MAX_BREAK_POINTS; ii++) {
		bp[ii].addr = 0;
		bp[ii].active = false;
	}

	flag = 0;
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
	bp[0].addr = 0;
	bp[0].active = false;
	flag = 0;
}
//-----------------------------------------------------------------------------
char *TDebugger::MakeInstrLine(WORD *addr)
{
	BYTE opcode = memory->ReadByte(*addr);
	WORD oper = memory->ReadWord(*addr + 1);
	int ilen = cpu->GetLength(opcode);
	unsigned i, j = 14;

	sprintf(lineBuffer, radix ? "#%04X" : "%05d", *addr);

	switch (ilen) {
		default:
		case 1:
			sprintf(lineBuffer + 5, " %02X%6s", opcode, "");
			break;
		case 2:
			sprintf(lineBuffer + 5, " %02X%02X%4s", opcode, (oper & 0xff), "");
			break;
		case 3:
			sprintf(lineBuffer + 5, " %02X%02X%02X  ",
				opcode, (oper & 0xff), ((oper >> 8) & 0xff));
			break;
	}

	char *mnemo = (Settings->Debugger->z80) ? instrZ80[opcode] : instr8080[opcode];
	for (i = 0; i < strlen(mnemo); i++) {
		switch (mnemo[i]) {
			case '@':
				oper = (WORD)(opcode & 0x38);
			// ... and continue in next case

			case '%':
				sprintf(lineBuffer + j, radix ? "#%02X" : "%03d", (oper & 0xff));
				j += 3;
				break;

			case '*':
			case '&':
				sprintf(lineBuffer + j, radix ? "#%04X" : "%05d", oper);
				j += 5;
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
char *TDebugger::MakeDumpLine(WORD *addr)
{
	if (Settings->Debugger->listType == DL_DISASM)
		return MakeInstrLine(addr);

	int i, j = 6, k;
	WORD adr = *addr;
	BYTE ch;

	sprintf(lineBuffer, radix ? "#%04X " : "%05d ", adr);
	if (Settings->Debugger->listType == DL_DUMP) {
		for (i = 0; i < 8; i++) {
			ch = memory->ReadByte(adr++);
			sprintf(lineBuffer + j, radix ? "#%04X" : "%05d", ch);
			j += 4;
		}
		k = 8;
	}
	else
		k = 32;

	adr = *addr;
	for (i = 0; i < k; i++) {
		ch = memory->ReadByte(adr++);
		if (ch < 0x20 || ch > SCHR_ERROR)
			ch = SCHR_ERROR;
		lineBuffer[j++] = (char) ch;
	}

	*addr = adr;
	return lineBuffer;
}
//-----------------------------------------------------------------------------
WORD TDebugger::FindPeviousInstruction(WORD pc, int howmany)
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
char *TDebugger::FillDisass(BYTE *ctrl)
{
	static WORD pc;
	static int jump = -1;

	if (cpu == NULL || memory == NULL || ctrl == NULL)
		return NULL;

	if (*ctrl > 1) {
		jump = -1;
		pc = cpu->GetPC();

		if (strchr(instr8080[memory->ReadByte(pc)], '*') != NULL)
			jump = memory->ReadWord(pc + 1);

		pc = FindPeviousInstruction(pc, *ctrl);
	}

	*ctrl = ((int) pc == jump) ? 1 : 0;
	return MakeInstrLine(&pc);
}
//-----------------------------------------------------------------------------
char *TDebugger::FillRegs()
{
	static char regs[6][3] = { "AF", "BC", "DE", "HL", "PC", "SP" };
	int i, j = 0, k = 0;

	if (cpu == NULL || memory == NULL)
		return NULL;

	for (i = 0; i < 6; i++) {
		switch (i) {
			case 0:
				k = cpu->GetAF();
				break;
			case 1:
				k = cpu->GetBC();
				break;
			case 2:
				k = cpu->GetDE();
				break;
			case 3:
				k = cpu->GetHL();
				break;
			case 4:
				k = cpu->GetPC();
				break;
			case 5:
				k = cpu->GetSP();
				break;
		}

		sprintf(lineBuffer + j, (radix ? "%s:#%04X\n" : "%s:%05d\n"), regs[i], k);
		j += 9;
	}

	lineBuffer[--k] = '\0';
	return lineBuffer;
}
//-----------------------------------------------------------------------------
char *TDebugger::FillFlags()
{
	BYTE val = cpu->GetAF();

	sprintf(lineBuffer, "%s\n%s\n%s\n%s\n%s\n%s",
		((val & FLAG_S)  ? " M" : " P"),
		((val & FLAG_Z)  ? " Z" : "NZ"),
		((val & FLAG_AC) ? "AC" : "NA"),
		((val & FLAG_PE) ? "PE" : "PO"),
		((val & FLAG_CY) ? " C" : "NC"),
		(cpu->IsInterruptEnabled() ? "EI" : "DI"));

	return lineBuffer;
}
//-----------------------------------------------------------------------------
char *TDebugger::FillStack()
{
	WORD val = cpu->GetSP() - 2, j = 0;

	for (int i = 0; i < 5; i++) {
		sprintf(lineBuffer + j, (radix ? "#%04X  #%04X\n" : "%05d  %05d\n"),
				val, memory->ReadWord(val));

		val += (WORD) 2;
		j += 13;
	}

	lineBuffer[--j] = '\0';
	return lineBuffer;
}
//-----------------------------------------------------------------------------
char *TDebugger::FillBreakpoints(BYTE *ctrl)
{
	static int ii = -1;

	if (cpu == NULL || memory == NULL || ctrl == NULL)
		return NULL;

	if (*ctrl > 1)
		ii = 1;

	sprintf(lineBuffer, (radix ? "#%04X" : "%05d"), bp[ii].addr);
	*ctrl = (BYTE) bp[ii].active;

	ii++;
	return lineBuffer;
}
//-----------------------------------------------------------------------------
/*
void TDebugger::FillList()
{
	if (cpu == NULL || memory == NULL)
		return;

	string aLA;
	WORD adr = GetCurrentSourceAddress();
	int ls = CbListSrc->ItemIndex;
	int off = offsets[ls];

	switch (ls) {
		case 0:
			aLA = "(MEM";
			break;

		case 1:
			aLA = "(AF";
			break;

		case 2:
			aLA = "(BC";
			break;

		case 3:
			aLA = "(DE";
			break;

		case 4:
			aLA = "(HL";
			break;

		case 5:
			aLA = "(PC";
			break;

		case 6:
			aLA = "(SP";
			break;
	}
	adr += (WORD) off;
	LblListAddr->Hint = MakeNumber(&adr, true, false, true, false, !radix);

	if (off > 0)
		aLA += ("+" + MakeNumber(&off, true, false, true, false, !radix));
	else if (off < 0) {
		off = -off;
		aLA += ("-" + MakeNumber(&off, true, false, true, false, !radix));
	}
	aLA += ")";

	if (CbListType->ItemIndex == 2) {
		dump1->Caption = MakeInstrLine(&adr);
		dump2->Caption = MakeInstrLine(&adr);
		dump3->Caption = MakeInstrLine(&adr);
		dump4->Caption = MakeInstrLine(&adr);
		dump5->Caption = MakeInstrLine(&adr);
		dump6->Caption = MakeInstrLine(&adr);
		dump7->Caption = MakeInstrLine(&adr);
		dump8->Caption = MakeInstrLine(&adr);
		dump9->Caption = MakeInstrLine(&adr);
		dump10->Caption = MakeInstrLine(&adr);
	}
	else {
		dump1->Caption = MakeDumpLine(&adr);
		dump2->Caption = MakeDumpLine(&adr);
		dump3->Caption = MakeDumpLine(&adr);
		dump4->Caption = MakeDumpLine(&adr);
		dump5->Caption = MakeDumpLine(&adr);
		dump6->Caption = MakeDumpLine(&adr);
		dump7->Caption = MakeDumpLine(&adr);
		dump8->Caption = MakeDumpLine(&adr);
		dump9->Caption = MakeDumpLine(&adr);
		dump10->Caption = MakeDumpLine(&adr);
	}

	LblListAddr->Caption = aLA;
}
//-----------------------------------------------------------------------------
void TDebugger::FillBreakpoints()
{
	for (int ii = 1; ii < MAX_BREAK_POINTS; ii++) {
		ClbBreakPoints->Items->Strings[ii - 1]
				= MakeNumber(&bp[ii].addr, true, true, true, false, !radix);
		ClbBreakPoints->Checked[ii - 1] = bp[ii].active;
	}
	ClbBreakPoints->ItemIndex = -1;
}
//-----------------------------------------------------------------------------
void TDebugger::FillNesting()
{
	int adr;

	LbNestings->Clear();
	for (int ii = 0; ii < na_depth; ii++) {
		adr = na[ii].addr + na[ii].offset;
		LbNestings->Items->Add(MakeNumber(&adr, true, true, true, false, !radix));
	}
}
//-----------------------------------------------------------------------------
*/
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

	if (opcode == 0xCD || opcode == 0xDD || opcode == 0xED || opcode == 0xFD ||
			(opcode & 0xC7) == 0xC4 || (opcode & 0xC7) == 0xC7) {

		bp[0].active = false;
		if (CheckBreakPoint(adr))
			cpu->DoInstruction();

		bp[0].addr = FindNextInstruction(adr, 1);
		bp[0].active = true;
		flag = 9;
	}
	else {
		cpu->DoInstruction();
		Reset();
	}
}
//---------------------------------------------------------------------------
void TDebugger::DoStepOut()
{
	wsp = cpu->GetSP();
	bp[0].active = false;
	if (CheckBreakPoint(cpu->GetPC()))
		cpu->DoInstruction();

	flag = 8;
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
	flag = 9;
}
//---------------------------------------------------------------------------
bool TDebugger::CheckBreakPoint(WORD adr)
{
	for (int ii = 0; ii < MAX_BREAK_POINTS; ii++)
		return (bp[ii].active && adr == bp[ii].addr);

	return false;
}
//---------------------------------------------------------------------------
bool TDebugger::CheckDebugRet(int *t)
{
	BYTE oc = memory->ReadByte(cpu->GetPC());
	*t = cpu->DoInstruction();

	return ((oc == 0xC9 || oc == 0xD9 ||
		((oc & 0xC7) == 0xC0 && *t == 11)) &&
			wsp < cpu->GetSP());
}
//---------------------------------------------------------------------------
