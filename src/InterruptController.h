/*	InteruptController.h: Abstract class InteruptController
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

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
