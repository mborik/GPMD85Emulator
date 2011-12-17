/*	ChipPIT8253.h: Class for emulation of PIT 8253 chip
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
//---------------------------------------------------------------------------
class ChipPIT8253 : public sigslot::has_slots<>
{
private:
	typedef struct
	{
		BYTE CWR;

		WORD InitValue;
		int OnInit;

		WORD CounterValue;
		bool WaitMsbRead;

		WORD CapturedValue;
		int Captured;

		bool Gate;
		bool Clock;
		bool Out;

		bool Counting;
		sigslot::signal2<TPITCounter, bool> OnOutChange;

	} COUNTER;

public:
	ChipPIT8253();

	// strana CPU
	void CpuWrite(TPITCounter dest, BYTE val);
	BYTE CpuRead(TPITCounter src);

	int GetChipState(BYTE *buffer);
	void SetChipState(BYTE *buffer);

	COUNTER Counters[3];

protected:
	// strana periferie
	void PeripheralSetGate(TPITCounter counter, bool state);
	void PeripheralSetClock(TPITCounter counter, bool state);
	bool PeripheralReadOut(TPITCounter counter);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

