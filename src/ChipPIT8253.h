/*	ChipPIT8253.h: Class for emulation of PIT 8253 chip
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
//---------------------------------------------------------------------------
#ifndef ChipPIT8253H
#define ChipPIT8253H
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
enum TPITCounter { CT_0 = 0, CT_1, CT_2, CT_CWR };
//---------------------------------------------------------------------------
#define CNT_MASK    0xC0
#define CNT_0       0x00
#define CNT_1       0x40
#define CNT_2       0x80
#define CNT_ILLEGAL 0xC0
#define CNT_SHIFT   6

#define RL_MASK     0x30
#define RL_CAPTURE  0x00
#define RL_LSB      0x10
#define RL_MSB      0x20
#define RL_BOTH     0x30
#define RL_SHIFT    4

#define MODE_MASK   0x0E
#define MODE_0      0x00
#define MODE_1      0x02
#define MODE_2      0x04
#define MODE_3      0x06
#define MODE_4      0x08
#define MODE_5      0x0A
#define MODE_2X     0x0C
#define MODE_3X     0x0E
#define MODE_SHIFT  1

#define TYPE_MASK   0x01
#define TYPE_BINARY 0x00
#define TYPE_BCD    0x01

#define STAT_OUT    0x80
#define STAT_NULL   0x40
#define STAT_GATE   0x20
#define STAT_CLK    0x10
//---------------------------------------------------------------------------
class ChipPIT8253
{
private:
	typedef struct
	{
		BYTE CWR;
		bool CwrWritten;

		WORD InitValue;
		int OnInit;
		bool InitValWritten;

		WORD CounterValue;
		bool WaitMsbRead;

		WORD CapturedValue;
		int Captured;

		bool Gate;
		bool Clock;
		bool Out;

		bool Triggered;
		bool Counting;

		sigslot::signal<TPITCounter, bool> OnOutChange;
		sigslot::signal<TPITCounter, bool> OnOutChange602;
	} COUNTER;

public:
	ChipPIT8253();

	// CPU side
	void CpuWrite(TPITCounter dest, BYTE val);
	BYTE CpuRead(TPITCounter src);

	int GetChipState(BYTE *buffer);
	void SetChipState(BYTE *buffer);

	COUNTER Counters[3];
	bool DecrementCounter(int cnt);

protected:
	// peripheral side
	void PeripheralSetGate(TPITCounter counter, bool state);
	void PeripheralSetClock(TPITCounter counter, bool state);
	bool PeripheralReadOut(TPITCounter counter);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

