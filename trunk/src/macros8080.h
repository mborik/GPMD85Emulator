/*	macros8080.h: Macros for emulation of i8080 microprocessor
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>

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
#ifndef MACROS_8080
#define MACROS_8080
//-----------------------------------------------------------------------------
// registre
#define A   this->af.b.h
#define F   this->af.b.l
#define AF  this->af.w

#define B   this->bc.b.h
#define C   this->bc.b.l
#define BC  this->bc.w

#define D   this->de.b.h
#define E   this->de.b.l
#define DE  this->de.w

#define H   this->hl.b.h
#define L   this->hl.b.l
#define HL  this->hl.w

#define SPH this->sp.b.h
#define SPL this->sp.b.l
#define SP  this->sp.w

#define PCH this->pc.b.h
#define PCL this->pc.b.l
#define PC  this->pc.w
//-----------------------------------------------------------------------------
// priznaky
#define FLAG_CY 0x01
#define FLAG_1  0x02
#define FLAG_PE 0x04
#define FLAG_3  0x08
#define FLAG_AC 0x10
#define FLAG_5  0x20
#define FLAG_Z  0x40
#define FLAG_S  0x80
//-----------------------------------------------------------------------------
#define readword(address)         this->memory->ReadWord(address)
#define writeword(address, value) this->memory->WriteWord(address, value)
//-----------------------------------------------------------------------------
// makra pre casto pouzivane instrukcie

// ANA r   ANA M   ANI byte
#define ANA(value)\
{\
	F = (BYTE) (((A & 0x08) | ((value) & 0x08)) ? FLAG_AC : 0);\
	A &= (value);\
	F |= sz53p1Table[A];\
}

// ADC r   ADC M   ACI byte
#define ADC(value)\
{\
	wordtemp = (WORD)(A + (value) + (F & FLAG_CY));\
	lookup = ((A & 0x08) >> 3)\
			 | (((value) & 0x08) >> 2)\
			 | ((wordtemp & 0x08) >> 1);\
	A = (BYTE)wordtemp;\
	F = (BYTE)((wordtemp & 0x100 ? FLAG_CY : 0)\
		| auxcarryAddTable[lookup & 0x07]\
		| sz53p1Table[A]);\
}

// ADD r   ADD M   ADI byte
#define ADD(value)\
{\
	wordtemp = (WORD)(A + (value));\
	lookup = ((A & 0x08) >> 3)\
			 | (((value) & 0x08) >> 2)\
			 | ((wordtemp & 0x08) >> 1);\
	A = (BYTE)wordtemp;\
	F = (BYTE)((wordtemp & 0x100 ? FLAG_CY : 0)\
		| auxcarryAddTable[lookup & 0x07]\
		| sz53p1Table[A]);\
}

// CALL adr   Cc adr
#define CALL()\
{\
	PUSH(PC);\
	PC = wordtemp;\
}

// CMP r   CMP M   CPI byte
#define CMP(value)\
{\
	wordtemp = (WORD)(A - (value));\
	lookup = ((A & 0x08) >> 3)\
			 | (((value) & 0x08) >> 2 )\
			 | ((wordtemp & 0x08) >> 1 );\
	F = (BYTE)((wordtemp & 0x100 ? FLAG_CY : 0)\
		| auxcarrySubTable[lookup & 0x07]\
		| sz53p1Table[wordtemp & 0xFF]);\
}

// DAD rp
#define DAD(value)\
{\
	dwordtemp = HL + (value);\
	HL = (WORD)dwordtemp;\
	F = (BYTE)((F & (FLAG_S | FLAG_Z | FLAG_AC | FLAG_PE))\
		| FLAG_1 | (dwordtemp & 0x10000 ? FLAG_CY : 0));\
}

// DCR r   DCR M
#define DCR(value)\
{\
	F = (BYTE)((F & FLAG_CY)\
		| ((value) & 0x0f ? FLAG_AC : 0));\
	(value)--;\
	F |= sz53p1Table[(value)];\
}

// INR r   INR M
#define INR(value)\
{\
	(value)++;\
	F = (BYTE)((F & FLAG_CY)\
		| ((value) & 0x0f ? 0 : FLAG_AC)\
		| sz53p1Table[(value)]);\
}

// JMP adr   Jc adr
#define JMP()\
{\
	PC = wordtemp;\
}

// ORA r   ORA M   ORI byte
#define ORA(value)\
{\
	A |= (value);\
	F = sz53p1Table[A];\
}

// POP rp
#define POP(reg)\
{\
	(reg) = readword(SP);\
	SP += (WORD)2;\
}

// PUSH rp
#define PUSH(reg)\
{\
	SP -= (WORD)2;\
	writeword(SP, (reg));\
}

// RET   Rc
#define RET()\
{\
	POP(PC);\
}

// RST n
#define RST(value)\
{\
	PUSH(PC);\
	PC = (value);\
}

// SBB r   SBB M   SBI byte
#define SBB(value)\
{\
	wordtemp = (WORD)(A - (value) - (F & FLAG_CY));\
	lookup = ((A & 0x08) >> 3)\
			 | (((value) & 0x08) >> 2)\
			 | ((wordtemp & 0x08) >> 1);\
	A = (BYTE)wordtemp;\
	F = (BYTE)((wordtemp & 0x100 ? FLAG_CY : 0 )\
		| auxcarrySubTable[lookup & 0x07]\
		| sz53p1Table[A]);\
}

// SUB r   SUB M   SUI byte
#define SUB(value)\
{\
	wordtemp = (WORD)(A - (value));\
	lookup = ((A & 0x08) >> 3)\
			 | (((value) & 0x08) >> 2)\
			 | ((wordtemp & 0x08) >> 1);\
	A = (BYTE)wordtemp;\
	F = (BYTE)((wordtemp & 0x100 ? FLAG_CY : 0)\
		| auxcarrySubTable[lookup & 0x07]\
		| sz53p1Table[A]);\
}

// XRA r   XRA M   XRI byte
#define XRA(value)\
{\
	A ^= (value);\
	F = sz53p1Table[A];\
}
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------
