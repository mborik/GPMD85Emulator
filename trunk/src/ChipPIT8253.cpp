/*	ChipPIT8253.cpp: Class for emulation of PIT 8253 chip
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
#include "ChipPIT8253.h"
//---------------------------------------------------------------------------
/**
 * Konstruktor pre vytvorenie objektu cipu PIT 8253. Zodpoveda stavu Power-up,
 * teda pripojeniu napajania.
 * Na rozdiel od originalneho cipu, ktory je po pripojeni napajania v neurcitom
 * stave, su vsetky 3 citace nastavene do modu 0, s binarnym odpocitavanim,
 * Read/Load v mode LSB, MSB a s pociatocnou hodnotou 65535.
 * Zaroven sa nastavia adresy notifykacnych funkcii na NULL.
 */
ChipPIT8253::ChipPIT8253()
{
	for (int ii = 0; ii < 3; ii++) {
		Counters[ii].CWR = (BYTE)((ii << CNT_SHIFT) | RL_BOTH | MODE_1 | TYPE_BINARY);
		Counters[ii].InitValue = 0xFFFF;
		Counters[ii].OnInit = 0;
		Counters[ii].CounterValue = Counters[ii].InitValue;
		Counters[ii].WaitMsbRead = false;
		Counters[ii].CapturedValue = 0;
		Counters[ii].Captured = 0;

		Counters[ii].Counting = false;

		Counters[ii].Gate = false;
		Counters[ii].Clock = false;
		Counters[ii].Out = false;

		Counters[ii].OnOutChange.disconnect_all();
	}
}
//---------------------------------------------------------------------------
/**
 * Metoda GetChipState je pouzivana pri vytvarani Snapshotu a ulozi niektore
 * vnutorne registre chipu do buffra. Ak je buffer null, vrati potrebnu velkost
 * buffra v bytoch.
 * Data sa do buffra ulozia v poradi:
 *    CWR0, Init0L, Init0H, CWR1, Init1L, Init1H, CWR2, Init2L, Init2H
 *
 * @param buffer buffer kam sa ulozi stav chipu
 * @return pocet bytov ulozenych do buffra
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
	}

	return 9;
}
//---------------------------------------------------------------------------
/**
 * Metoda SetChipState je pouzivana po otvoreni Snapshotu pre prednastavenie
 * niektorych vnutornych registrov chipu.
 * Data v buffri musia byt v poradi:
 *    CWR0, Init0L, Init0H, CWR1, Init1L, Init1H, CWR2, Init2L, Init2H
 *
 * @param buffer buffer kam sa ulozi stav chipu
 * @return pocet bytov ulozenych do buffra
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
 * Metodu CpuWrite vola mikroprocesor pri vykonavani instrukcie OUT - zapis na
 * port (CPU -> PIT). Zodpoveda teda privedeniu urovne L na vstupy /CS (21) a
 * /WR (23).
 * Podla rezimu a stavu Read/Load sa zapis udeje do LSB alebo MSB registra
 * inicializacnej hodnoty citaca.
 * Ak sa pri zapise riadiaceho slova zmeni stav vystupu citaca, je volana
 * prislusna notifikacna funkcia.
 *
 * @param dest oznacuje citac alebo riadiace slovo (TPITCounter)
 * @param val posielana hodnota
 */
void ChipPIT8253::CpuWrite(TPITCounter dest, BYTE val)
{
	int cnt;
	bool oldOut;

	switch (dest) {
		case CT_0 :
		case CT_1 :
		case CT_2 :
			cnt = (int)dest;
			switch (Counters[cnt].CWR & RL_MASK) {
				case RL_LSB :
					Counters[cnt].InitValue = (WORD)((Counters[cnt].InitValue & 0xFF00) | val);
//					Counters[cnt].InitValue = (WORD)(val);
					Counters[cnt].OnInit = 0;
					break;

				case RL_MSB :
					Counters[cnt].InitValue = (WORD)((Counters[cnt].InitValue & 0xFF) | ((WORD)val << 8));
//					Counters[cnt].InitValue = (WORD)(val << 8);
					Counters[cnt].OnInit = 0;
					break;

				case RL_BOTH :
					if (Counters[cnt].OnInit == 1) {
						Counters[cnt].InitValue |= (WORD)(val << 8);  // MSB
						Counters[cnt].OnInit = 0;
					}
					else {
						Counters[cnt].InitValue = val;                // LSB
						Counters[cnt].OnInit = 1;
					}
					break;
			}
			Counters[cnt].Counting = false;

			if (Counters[cnt].OnInit == 0) {
				if ((Counters[cnt].CWR & TYPE_MASK) == TYPE_BCD) {
					Counters[cnt].InitValue
						= (WORD)(((Counters[cnt].InitValue >> 12) & 0x0F) * 1000
						+ ((Counters[cnt].InitValue >> 8) & 0x0F) * 100
						+ ((Counters[cnt].InitValue >> 4) & 0x0F) * 10
						+ (Counters[cnt].InitValue & 0x0F));
				}
				oldOut = Counters[cnt].Out;

				switch (Counters[cnt].CWR & MODE_MASK) {
					case MODE_0 :
						Counters[cnt].Out = false;
						Counters[cnt].CounterValue = Counters[cnt].InitValue;
						Counters[cnt].Counting = Counters[cnt].Gate;
						break;

					case MODE_1 :
					case MODE_5 :
						Counters[cnt].Out = true;
						Counters[cnt].Counting = false;
						break;

					case MODE_2 :
					case MODE_2X :
					case MODE_3 :
					case MODE_3X :
					case MODE_4 :
						Counters[cnt].Out = true;
						Counters[cnt].CounterValue = Counters[cnt].InitValue;
						Counters[cnt].Counting = Counters[cnt].Gate;
						break;
				}

				if (oldOut != Counters[cnt].Out)
					Counters[cnt].OnOutChange(dest, Counters[cnt].Out);
			}
			break;

		case CT_CWR :
			if ((val & CNT_MASK) == CNT_ILLEGAL)
				return;
			cnt = ((val & CNT_MASK) >> CNT_SHIFT);

			if ((val & RL_MASK) == RL_CAPTURE) {
				Counters[cnt].Captured = ((Counters[cnt].CWR & RL_MASK) == RL_BOTH) ? 2 : 1;
				if ((Counters[cnt].CWR & TYPE_MASK) == TYPE_BCD) {
					Counters[cnt].CapturedValue = 0;
					WORD cntval = Counters[cnt].CounterValue;
					for (int ii = 0; ii < 4 && cntval > 0; ii++) {
						Counters[cnt].CapturedValue += (WORD)((cntval % 10) << (4 * ii));
						cntval /= (WORD)10;
					}
				}
				else
					Counters[cnt].CapturedValue = Counters[cnt].CounterValue;
			}
			else {
				Counters[cnt].CWR = val;
				Counters[cnt].OnInit = ((Counters[cnt].CWR & RL_MASK) == RL_BOTH) ? 2 : 1;
				Counters[cnt].WaitMsbRead = false;
				Counters[cnt].Captured = 0;
			}

			Counters[cnt].Counting = false;

			oldOut = Counters[cnt].Out;
			if ((Counters[cnt].CWR & MODE_MASK) == MODE_0)
				Counters[cnt].Out = false;
			else
				Counters[cnt].Out = true;

			if (oldOut != Counters[cnt].Out)
				Counters[cnt].OnOutChange((TPITCounter)cnt, Counters[cnt].Out);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Metodu CpuRead vola mikroprocesor pri vykonavani instrukcie IN - citanie z
 * portu (CPU <- PIT). Zodpoveda teda privedeniu urovne L na vstupy /CS (21) a
 * /RD (22).
 * Ak bol nastaveny mod CAPTURE, vratena hodnota zodpoveda predtym zachytenej
 * hodnote daneho citaca. Inak je vratena aktualna hodnota citaca. Podla rezimu
 * Read/Load je to LSB alebo MSB.
 * Citanie riadiaceho slova nie je mozne a je vratena hodnota 0xFF.
 *
 * @param src oznacuje citac (TPITCounter), z ktoreho sa ma udaj precitat
 * @return hodnota z citaca
 */
BYTE ChipPIT8253::CpuRead(TPITCounter src)
{
	int cnt = (int) src;
	BYTE retval = 0xFF;

	switch (src) {
		case CT_0 :
		case CT_1 :
		case CT_2 :
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
						if (Counters[cnt].WaitMsbRead == false)
							retval = (BYTE)(Counters[cnt].CounterValue & 0xFF);         // LSB
						else
							retval = (BYTE)((Counters[cnt].CounterValue >> 8) & 0xFF);  // MSB
						Counters[cnt].WaitMsbRead = !Counters[cnt].WaitMsbRead;
					}
					break;
			}
			break;

		default :  // datova zbernica je v Z (tretom stave)
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
			if (oldGate == false && state == true) {
				Counters[cnt].CounterValue = Counters[cnt].InitValue;
				Counters[cnt].Counting = true;
				Counters[cnt].Out = false;
			}
			break;

		case MODE_2 :
		case MODE_2X :
		case MODE_3 :
		case MODE_3X :
			Counters[cnt].Counting = state;
			if (state == false)
				Counters[cnt].Out = true;
			if (oldGate == false && state == true)
				Counters[cnt].CounterValue = Counters[cnt].InitValue;
			break;

		case MODE_5 :
			Counters[cnt].Counting = state;
			if (oldGate == false && state == true) {
				Counters[cnt].CounterValue = Counters[cnt].InitValue;
				Counters[cnt].Out = true;
			}
			break;
	}

	Counters[cnt].Gate = state;

	if (oldOut != Counters[cnt].Out)
		Counters[cnt].OnOutChange(counter, Counters[cnt].Out);
}
//---------------------------------------------------------------------------
void ChipPIT8253::PeripheralSetClock(TPITCounter counter, bool state)
{
	if (counter == CT_CWR)
		return;

	int cnt = (int)counter;
	if (Counters[cnt].OnInit > 0)
		return;

	bool oldOut = Counters[cnt].Out;
	bool oldClock = Counters[cnt].Clock;
	Counters[cnt].Clock = state;

	switch (Counters[cnt].CWR & MODE_MASK) {
		case MODE_0 :
			if (oldClock == true && state == false
					&& Counters[cnt].Counting == true) {
				if (--Counters[cnt].CounterValue == 0)
					Counters[cnt].Out = true;
				if (Counters[cnt].CounterValue == 0xFFFF
						&& ((Counters[cnt].CWR & TYPE_MASK) == TYPE_BCD))
					Counters[cnt].CounterValue = 9999;
			}
			break;

		case MODE_1 :
			if (oldClock == true && state == false
					&& Counters[cnt].Counting == true) {
				if (--Counters[cnt].CounterValue == 0) {
					Counters[cnt].Out = true;
					Counters[cnt].Counting = false;
				}
			}
			break;

		case MODE_2 :
		case MODE_2X :
			if (oldClock == true && state == false
					&& Counters[cnt].Counting == true) {
				if (--Counters[cnt].CounterValue == 0) {
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].Out = true;
				}
				else if (Counters[cnt].CounterValue == 1)
					Counters[cnt].Out = false;
			}
			break;

		case MODE_3 :
		case MODE_3X :
			if (oldClock == true && state == false
					&& Counters[cnt].Counting == true) {
				if (Counters[cnt].Out == true) {
					Counters[cnt].CounterValue--;
					if (Counters[cnt].CounterValue & 1)
						Counters[cnt].CounterValue--;
				}
				else {
					if (Counters[cnt].CounterValue & 1)
						Counters[cnt].CounterValue--;
					Counters[cnt].CounterValue -= (WORD)2;
				}

				if (Counters[cnt].CounterValue == 0) {
					Counters[cnt].CounterValue = Counters[cnt].InitValue;
					Counters[cnt].Out = !Counters[cnt].Out;
				}
			}
			break;

		case MODE_4 :
		case MODE_5 :
			if (oldClock == true && state == false
					&& Counters[cnt].Counting == true) {
				if (--Counters[cnt].CounterValue == 0) {
					Counters[cnt].Out = true;
					Counters[cnt].Counting = false;
				}
				else if (Counters[cnt].CounterValue == 1)
					Counters[cnt].Out = false;
			}
			break;
	}

	if (oldOut != Counters[cnt].Out)
		Counters[cnt].OnOutChange(counter, Counters[cnt].Out);
}
//---------------------------------------------------------------------------
bool ChipPIT8253::PeripheralReadOut(TPITCounter counter)
{
	if (counter == CT_CWR)
		return false;

	return Counters[(int) counter].Out;
}
//---------------------------------------------------------------------------
