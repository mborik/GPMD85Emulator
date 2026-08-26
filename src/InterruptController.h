/*	InteruptController.h: Abstract class InteruptController
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

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
#ifndef InterruptControllerH
#define InterruptControllerH
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
enum TInterruptVector { IV_OPCODE, IV_OPERAND_L, IV_OPERAND_H };
//---------------------------------------------------------------------------
class InterruptController {
public:
	int Tag;
	void *cpu;

	// Pure virtual methods:
	// It was implemented by interupt control class and called by processor.
	virtual BYTE getInterruptVector(TInterruptVector intVector) = 0;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
