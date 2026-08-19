/*	ChipPIT8253.cpp: Class for emulation of PIT 8253 chip
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

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
#include "ChipPIT8253.h"
//---------------------------------------------------------------------------
/**
 * Constructor creates object for chip PIT 8253. Initialization in Power-up
 * state corresponds to providing a power supply to the chip.
 * Original chip remains unitialized, while here all 3 counters are set to
 * mode 1, with binary decrement, Read/Load in mode LSB, MSB and initial
 * value 65535. All addresses of notification functions are set to NULL.
 */
ChipPIT8253::ChipPIT8253()
{
	for (int ii = 0; ii < 3; ii++) {
		Counters[ii].CWR = (BYTE)((ii << CNT_SHIFT) | RL_BOTH | MODE_1 | TYPE_BINARY);
		Counters[ii].CwrWritten = false;
		Counters[ii].InitValue = 0xFFFF;
		Counters[ii].OnInit = 0;
		Counters[ii].CounterValue = Counters[ii].InitValue;
		Counters[ii].WaitMsbRead = false;
		Counters[ii].CapturedValue = 0;
		Counters[ii].Captured = 0;

		Counters[ii].Counting = false;
		Counters[ii].InitValWritten = false;
		Counters[ii].Counting = false;

		Counters[ii].Gate = false;
		Counters[ii].Clock = false;
		Counters[ii].Out = false;

		Counters[ii].OnOutChange.disconnect_all();
		Counters[ii].OnOutChange602.disconnect_all();
	}
}
//---------------------------------------------------------------------------
/**
 * Method GetChipState is used during Snapshot creation and it saves some of
 * internal registers of chip into a buffer. If buffer is null it returns
 * size of buffer in bytes.
 * Data is saved into buffer in following order:
 *    CWR0, Init0L, Init0H, CWR1, Init1L, Init1H, CWR2, Init2L, Init2H,
 *    Cnt0L, Cnt0H, Cnt1L, Cnt1H, Cnt2L, Cnt2H
 *
 * @param buffer buffer where chip status will be saved
 * @return number of bytes saved into buffer
 */
int ChipPIT8253::GetChipState(BYTE *buffer)
{
	if (buffer != NULL) {
		*(buffer + 0) = Counters[0].CWR;
		*(buffer + 1) = (BYTE)(Counters[0].InitValue & 0xFF);
		*(buffer + 2) = (BYTE)((Counters[0].InitValue >> 8) & 0xFF);
		*(buffer + 3) = Counters[1].CWR;
		*(buffer + 4) = (BYTE)(Counters[1].InitValue & 0xFF);
		*(buffer + 5) = (BYTE)((Counters[1].InitValue >> 8) & 0xFF);
		*(buffer + 6) = Counters[2].CWR;
		*(buffer + 7) = (BYTE)(Counters[2].InitValue & 0xFF);
		*(buffer + 8) = (BYTE)((Counters[2].InitValue >> 8) & 0xFF);

		*(buffer + 9) = (BYTE)(Counters[0].CounterValue & 0xFF);
		*(buffer + 10) = (BYTE)((Counters[0].CounterValue >> 8) & 0xFF);
		*(buffer + 11) = (BYTE)(Counters[1].CounterValue & 0xFF);
		*(buffer + 12) = (BYTE)((Counters[1].CounterValue >> 8) & 0xFF);
		*(buffer + 13) = (BYTE)(Counters[2].CounterValue & 0xFF);
		*(buffer + 14) = (BYTE)((Counters[2].CounterValue >> 8) & 0xFF);

		*(buffer + 15) = (BYTE)(((Counters[0].Out) ? STAT_OUT  : 0)
		                    |  ((Counters[0].Gate) ? STAT_GATE : 0)
		                    | ((Counters[0].Clock) ? STAT_CLK  : 0));
		*(buffer + 16) = (BYTE)(((Counters[1].Out) ? STAT_OUT  : 0)
		                    |  ((Counters[1].Gate) ? STAT_GATE : 0)
		                    | ((Counters[1].Clock) ? STAT_CLK  : 0));
		*(buffer + 17) = (BYTE)(((Counters[2].Out) ? STAT_OUT  : 0)
		                    |  ((Counters[2].Gate) ? STAT_GATE : 0)
		                    | ((Counters[2].Clock) ? STAT_CLK  : 0));
	}

	return 18;
}
//---------------------------------------------------------------------------
/**
 * Method SetChipState is used during open Snapshot load to pre-set some
 * internal registers of chip.
 * Mandatory order of data in buffer:
 *    CWR0, Init0L, Init0H, CWR1, Init1L, Init1H, CWR2, Init2L, Init2H
 *
 * @param buffer buffer where chip status will be saved
 * @return number of bytes saved into buffer
 */
void ChipPIT8253::SetChipState(BYTE *buffer)
{
	if (buffer != NULL) {
		CpuWrite(CT_CWR, *(buffer + 0));
		if ((*(buffer + 0) & 16) != 0)
			CpuWrite(CT_0, *(buffer + 1));
		if ((*(buffer + 0) & 32) != 0)
			CpuWrite(CT_0, *(buffer + 2));

		CpuWrite(CT_CWR, *(buffer + 3));
		if ((*(buffer + 3) & 16) != 0)
			CpuWrite(CT_1, *(buffer + 4));
		if ((*(buffer + 3) & 32) != 0)
			CpuWrite(CT_1, *(buffer + 5));

		CpuWrite(CT_CWR, *(buffer + 6));
		if ((*(buffer + 6) & 16) != 0)
			CpuWrite(CT_2, *(buffer + 7));
		if ((*(buffer + 6) & 32) != 0)
			CpuWrite(CT_2, *(buffer + 8));
	}
}
//---------------------------------------------------------------------------
/**
 * Method CpuWrite is invoked by CPU during OUT instruction - write to port (CPU -> PIT).
 * It corresponds to setting logic level L on inputs /CS (21) and /WR (23).
 * Write operation depends on mode and Read/Load status. LSB or MSB register of counter
 * preset is written eventually. If status of counter is changed during write
 * of control word corresponding notification function is invoked.
 *
 * @param dest distinguish between counter value or control word (TPITCounter)
 * @param val value to be sent
 */
void ChipPIT8253::CpuWrite(TPITCounter dest, BYTE val)
{
	int cnt;
	bool oldOut;

	switch (dest) {
		case CT_0 :
		case CT_1 :
		case CT_2 :
			cnt = (int) dest;
			switch (Counters[cnt].CWR & RL_MASK) {
				case RL_LSB :
//					Counters[cnt].InitValue = (WORD)((Counters[cnt].InitValue & 0xFF00) | val);
					Counters[cnt].InitValue = (WORD) val;
					Counters[cnt].OnInit = 0;
					break;

				case RL_MSB :
//					Counters[cnt].InitValue = (WORD)((Counters[cnt].InitValue & 0xFF) | ((WORD)val << 8));
					Counters[cnt].InitValue = (WORD) (val << 8);
					Counters[cnt].OnInit = 0;
					break;

				case RL_BOTH :
					if (Counters[cnt].OnInit == 1) {
						Counters[cnt].InitValue |= (WORD) (val << 8); // MSB
						Counters[cnt].OnInit = 0;
					}
					else {
						Counters[cnt].InitValue = val;                // LSB
						Counters[cnt].OnInit = 1;
						if ((Counters[cnt].CWR & MODE_MASK) == MODE_0) {
							Counters[cnt].Counting = false;
							Counters[cnt].Out = false;
						}
					}
					break;
			}

			if (Counters[cnt].OnInit == 0) {
				Counters[cnt].InitValWritten = true;
				oldOut = Counters[cnt].Out;

				switch (Counters[cnt].CWR & MODE_MASK) {
					case MODE_0 :
						Counters[cnt].Out = false;
						Counters[cnt].Counting = Counters[cnt].Gate;
						Counters[cnt].Triggered = true;
						break;

					case MODE_1 :
					case MODE_5 :
						Counters[cnt].Out = true;
						Counters[cnt].Counting = Counters[cnt].Gate;
						break;

					case MODE_2 :
					case MODE_2X :
					case MODE_3 :
					case MODE_3X :
						Counters[cnt].Out = true;
						Counters[cnt].Counting = Counters[cnt].Gate;
						if (Counters[cnt].Counting == false)
							Counters[cnt].CounterValue = Counters[cnt].InitValue;
						else if (Counters[cnt].CwrWritten)
							Counters[cnt].Triggered = true;
						break;

					case MODE_4 :
						Counters[cnt].Out = true;
						Counters[cnt].Counting = Counters[cnt].Gate;
						Counters[cnt].Triggered = true;
						break;
				}

				if (oldOut != Counters[cnt].Out) {
					Counters[cnt].OnOutChange(dest, Counters[cnt].Out);
					Counters[cnt].OnOutChange602(dest, Counters[cnt].Out);
				}

				Counters[cnt].CwrWritten = false;
			}
			break;

		case CT_CWR :
			if ((val & CNT_MASK) == CNT_ILLEGAL)
				return;
			cnt = ((val & CNT_MASK) >> CNT_SHIFT);

			if ((val & RL_MASK) == RL_CAPTURE) {
				if (Counters[cnt].Captured == 0) {
					Counters[cnt].Captured = ((Counters[cnt].CWR & RL_MASK) == RL_BOTH) ? 2 : 1;
					Counters[cnt].CapturedValue = Counters[cnt].CounterValue;
				}
			}
			else {
				Counters[cnt].CWR = val;
				Counters[cnt].OnInit = ((Counters[cnt].CWR & RL_MASK) == RL_BOTH) ? 2 : 1;
				Counters[cnt].InitValWritten = false;
				Counters[cnt].WaitMsbRead = false;
				Counters[cnt].Captured = 0;
				Counters[cnt].Counting = false;

				oldOut = Counters[cnt].Out;
				Counters[cnt].Out = ((Counters[cnt].CWR & MODE_MASK) != MODE_0);

				if (oldOut != Counters[cnt].Out) {
					Counters[cnt].OnOutChange((TPITCounter) cnt, Counters[cnt].Out);
					Counters[cnt].OnOutChange602((TPITCounter) cnt, Counters[cnt].Out);
				}

				Counters[cnt].CwrWritten = true;
			}
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Method CpuRead is invoked by CPU during IN instruction - read port (CPU <- PIT).
 * It corresponds to setting logic level L on inputs /CS (21) and /RD (22).
 * It returns fetched value of counter if CAPTURE mode have been set. Current value
 * of counter is returned otherwise. Depending on mode Read/Load it returns LSB or MSB.
 * Reading of control word is not possible and return value is always 0xFF.
 *
 * @param src sets the counter to be read (TPITCounter)
 * @return value of counter
 */
BYTE ChipPIT8253::CpuRead(TPITCounter src)
{
	int cnt;
	BYTE retval;

	switch (src) {
		case CT_0 :
		case CT_1 :
		case CT_2 :
			cnt = (int) src;
			switch (Counters[cnt].CWR & RL_MASK) {
				case RL_LSB :
					if (Counters[cnt].Captured > 0) {
						retval = (BYTE)(Counters[cnt].CapturedValue & 0xFF);
						Counters[cnt].Captured = 0;
					}
					else
						retval = (BYTE)(Counters[cnt].CounterValue & 0xFF);
					break;

				case RL_MSB :
					if (Counters[cnt].Captured > 0) {
						retval = (BYTE)((Counters[cnt].CapturedValue >> 8) & 0xFF);
						Counters[cnt].Captured = 0;
					}
					else
						retval = (BYTE)((Counters[cnt].CounterValue >> 8) & 0xFF);
					break;

				case RL_BOTH :
					if (Counters[cnt].Captured > 0) {
						if (Counters[cnt].Captured == 2)
							retval = (BYTE)(Counters[cnt].CapturedValue & 0xFF);        // LSB
						else
							retval = (BYTE)((Counters[cnt].CapturedValue >> 8) & 0xFF); // MSB
						Counters[cnt].Captured--;
					}
					else {
						if (!Counters[cnt].WaitMsbRead)
							retval = (BYTE)(Counters[cnt].CounterValue & 0xFF);         // LSB
						else
							retval = (BYTE)((Counters[cnt].CounterValue >> 8) & 0xFF);  // MSB
						Counters[cnt].WaitMsbRead = !Counters[cnt].WaitMsbRead;
					}
					break;
			}
			break;

		case CT_CWR :  // data bus is in hi-Z (high impedance state)
			retval = 0xFF;
			break;
	}

	return retval;
}
//---------------------------------------------------------------------------
void ChipPIT8253::PeripheralSetGate(TPITCounter counter, bool state)
{
	if (counter == CT_CWR)
		return;

	int cnt = (int)counter;
	bool oldGate = Counters[cnt].Gate;
	bool oldOut = Counters[cnt].Out;

	switch (Counters[cnt].CWR & MODE_MASK) {
		case MODE_0 :
		case MODE_4 :
			Counters[cnt].Counting = state;
			break;

		case MODE_1 :
		case MODE_5 :
			Counters[cnt].Counting = state;
			if (!oldGate && state)
				Counters[cnt].Triggered = true;
			break;

		case MODE_2 :
		case MODE_2X :
		case MODE_3 :
		case MODE_3X :
			Counters[cnt].Counting = state;
			if (!state)
				Counters[cnt].Out = true;
			else if (!oldGate && state)
				Counters[cnt].Triggered = true;
			break;
	}

	Counters[cnt].Gate = state;

	if (oldOut != Counters[cnt].Out) {
		Counters[cnt].OnOutChange(counter, Counters[cnt].Out);
		Counters[cnt].OnOutChange602(counter, Counters[cnt].Out);
	}
}
//---------------------------------------------------------------------------
void ChipPIT8253::PeripheralSetClock(TPITCounter counter, bool state)
{
	if (counter == CT_CWR)
		return;

	int cnt = (int) counter;
//	if (Counters[cnt].OnInit > 0)
//		return;

	bool oldOut = Counters[cnt].Out;
	bool oldClock = Counters[cnt].Clock;
	Counters[cnt].Clock = state;

	switch (Counters[cnt].CWR & MODE_MASK) {
		case MODE_0 :
			if (oldClock && !state) {
				if (Counters[cnt].InitValWritten) {
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].InitValWritten = false;
				}
				else if (Counters[cnt].Counting) {
					if (DecrementCounter(cnt))
						Counters[cnt].Out = true;
				}
			}
			break;

		case MODE_1 :
			if (oldClock && !state) {
				if (Counters[cnt].Triggered) {
					Counters[cnt].Triggered = false;
					Counters[cnt].InitValWritten = false;
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].Out = false;
					Counters[cnt].Counting = true;
				}
				else if (Counters[cnt].Counting) {
					if (DecrementCounter(cnt))
						Counters[cnt].Out = true;
				}
			}
			break;

		case MODE_2 :
		case MODE_2X :
			if (oldClock && !state) {
				if (Counters[cnt].Triggered) {
					Counters[cnt].Triggered = false;
					Counters[cnt].InitValWritten = false;
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
				}
				else if (Counters[cnt].Counting) {
					if (DecrementCounter(cnt)) {
						Counters[cnt].InitValWritten = false;
						Counters[cnt].CounterValue = Counters[cnt].InitValue;
						Counters[cnt].Out = true;
					}
					else if (Counters[cnt].CounterValue == 1)
						Counters[cnt].Out = false;
				}
			}
			break;

		case MODE_3 :
		case MODE_3X :
			if (oldClock && !state) {
				if (Counters[cnt].Triggered) {
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].InitValWritten = false;
					Counters[cnt].Triggered = false;
				}
				else if (Counters[cnt].Counting) {
					if (Counters[cnt].Out) {
						DecrementCounter(cnt);
						if (Counters[cnt].CounterValue & 1)
							DecrementCounter(cnt);
					}
					else {
						if (Counters[cnt].CounterValue & 1)
							DecrementCounter(cnt);
						DecrementCounter(cnt);
						DecrementCounter(cnt);
					}

					if (Counters[cnt].CounterValue == 0) {
						Counters[cnt].InitValWritten = false;
						// special rule for 3 and 1
						if (!((!Counters[cnt].Out && Counters[cnt].InitValue == 3)
						    || (Counters[cnt].Out && Counters[cnt].InitValue == 1)))
							Counters[cnt].CounterValue = Counters[cnt].InitValue;

						Counters[cnt].Out = !Counters[cnt].Out;
					}
				}
			}
			break;

		case MODE_4 :
			if (oldClock && !state) {
				if (Counters[cnt].InitValWritten) {
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].InitValWritten = false;
				}
				else if (Counters[cnt].Counting) {
					if (DecrementCounter(cnt))
						Counters[cnt].Out = false;
					else if (((Counters[cnt].CWR & TYPE_MASK) == TYPE_BINARY
					        && Counters[cnt].CounterValue == 0xFFFF)
					      || ((Counters[cnt].CWR & TYPE_MASK) == TYPE_BCD
					        && Counters[cnt].CounterValue == 0x9999))
						Counters[cnt].Out = true;
				}
			}
			break;

		case MODE_5 :
			if (oldClock && !state) {
				if (Counters[cnt].Triggered) {
					Counters[cnt].Triggered = false;
					Counters[cnt].InitValWritten = false;
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].Out = true;
				}
				else if (Counters[cnt].Counting) {
					if (DecrementCounter(cnt))
						Counters[cnt].Out = false;
					else if (((Counters[cnt].CWR & TYPE_MASK) == TYPE_BINARY
					        && Counters[cnt].CounterValue == 0xFFFF)
					      || ((Counters[cnt].CWR & TYPE_MASK) == TYPE_BCD
					        && Counters[cnt].CounterValue == 0x9999))
						Counters[cnt].Out = true;
				}
			}
			break;
	}

	if (oldOut != Counters[cnt].Out) {
		Counters[cnt].OnOutChange(counter, Counters[cnt].Out);
		Counters[cnt].OnOutChange602(counter, Counters[cnt].Out);
	}
}
//---------------------------------------------------------------------------
bool ChipPIT8253::PeripheralReadOut(TPITCounter counter)
{
	if (counter == CT_CWR)
		return false;

	return Counters[(int) counter].Out;
}
//---------------------------------------------------------------------------
bool ChipPIT8253::DecrementCounter(int cnt)
{
	if (--Counters[cnt].CounterValue == 0)
		return true;
	if ((Counters[cnt].CWR & TYPE_MASK) == TYPE_BINARY)
		return false;

	WORD cntval = Counters[cnt].CounterValue;
	if ((cntval & 0x000F) == 0x000F) {
		cntval = (WORD) ((cntval & 0xFFF0) | 0x0009);
		if ((cntval & 0x00F0) == 0x00F0) {
			cntval = (WORD) ((cntval & 0xFF0F) | 0x0090);
			if ((cntval & 0x0F00) == 0x0F00) {
				cntval = (WORD) ((cntval & 0xF0FF) | 0x0900);
				if ((cntval & 0xF000) == 0xF000)
					cntval = (WORD) ((cntval & 0x0FFF) | 0x9000);
			}
		}

		Counters[cnt].CounterValue = cntval;
	}

	return false;
}
//---------------------------------------------------------------------------
