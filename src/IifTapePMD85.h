/*	IifTapePMD85.h: Derived class for emulation of PMD 85 tape interface
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

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
#ifndef IifTapePMD85H
#define IifTapePMD85H
//---------------------------------------------------------------------------
#include "globals.h"
#include "IifTape.h"
#include "PeripheralDevice.h"
#include "ChipUSART8251.h"
#include "ChipPIT8253.h"
//---------------------------------------------------------------------------
class IifTapePMD85 : public PeripheralDevice, public IifTape, public ChipUSART8251 {
	public:
		IifTapePMD85(TComputerModel model, TTapeIfType ifType);

		virtual void ResetDevice(int ticks);
		virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
		virtual BYTE ReadFromDevice(BYTE port, int ticks);

		void TapeClockService123(int ticks, int dur);
		void TapeClockService23(TPITCounter counter, bool outState);

	private:
		void FnOnRtsSet();
		void FnOnTxRChange();
		void InitTapeTx();
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
