/*  IifTimer.cpp: Class for emulation of timer interface
    Copyright (c) 2006-2018 Roman Borik <pmd85emu@gmail.com>

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
#include "IifTimer.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
IifTimer::IifTimer(TComputerModel model, ChipCpu8080 *_cpu) : ChipPIT8253()
{
	this->model = model;

	cntRtc = 0;
	stateRtc = true;
	mouse602 = false;
	ifMIF85 = false;
	cpu = _cpu;

	PeripheralSetGate(CT_0, true);
	PeripheralSetClock(CT_0, false);
	PeripheralSetGate(CT_1, true);
	PeripheralSetClock(CT_1, false);
	PeripheralSetGate(CT_2, true);
	PeripheralSetClock(CT_2, stateRtc);

	if (model == CM_C2717)
		Counters[1].OnOutChange.connect(this, &IifTimer::CT2Clock);
	else
		Counters[0].OnOutChange.connect(this, &IifTimer::Timer0OutChange);
}
//---------------------------------------------------------------------------
void IifTimer::WriteToDevice(BYTE port, BYTE value, int ticks)
{
//	debug("IfTimer", "ticks=%d, port=%u, value=%u", ticks, port, value);

	switch (port & IIF_TIMER_REG_MASK) {
		case IIF_TIMER_REG_T0:
			CpuWrite(CT_0, value);
			break;

		case IIF_TIMER_REG_T1:
			CpuWrite(CT_1, value);
			break;

		case IIF_TIMER_REG_T2:
			CpuWrite(CT_2, value);
			break;

		case IIF_TIMER_REG_CWR:
			CpuWrite(CT_CWR, value);
			break;
	}
}
	//---------------------------------------------------------------------------
BYTE IifTimer::ReadFromDevice(BYTE port, int ticks)
{
	BYTE retval;

	switch (port & IIF_TIMER_REG_MASK) {
		case IIF_TIMER_REG_T0:
			retval = CpuRead(CT_0);
			break;

		case IIF_TIMER_REG_T1:
			retval = CpuRead(CT_1);
			break;

		case IIF_TIMER_REG_T2:
			retval = CpuRead(CT_2);
			break;

		case IIF_TIMER_REG_CWR:
			retval = CpuRead(CT_CWR);
			break;

		default:
			retval = 0xFF;
			break;
	}

//	debug("IfTimer", "ticks=%d, port=%u, retval=%u", ticks, port, retval);

	return retval;
}
	//---------------------------------------------------------------------------
void IifTimer::ITimerService(int ticks, int dur)
{
	if (model == CM_C2717) {
		for (int ii = 0; ii < dur; ii++) {
			// Timer T0 - clock for USART
			PeripheralSetClock(CT_0, true);
			PeripheralSetClock(CT_0, false);
			// Timer T1 - user timer chained with T2
			PeripheralSetClock(CT_1, true);
			PeripheralSetClock(CT_1, false);
		}
	}
	else {
		// Timer T2 - RTC
		cntRtc += dur;
		while (cntRtc >= HALF_SEC_RTC) {
			cntRtc -= HALF_SEC_RTC;
			stateRtc = !stateRtc;
			PeripheralSetClock(CT_2, stateRtc);
		}

		// Timer T1 - clock for USART or Mouse 602
		for (int ii = 0; ii < dur; ii++) {
			PeripheralSetClock(CT_1, true);
			PeripheralSetClock(CT_1, false);
		}

		currentTicks = ticks;

		// Timer T0 - clock for MIF 85 interrupt
		if (ifMIF85) {
			for (int ii = 0; ii < dur; ii++) {
				PeripheralSetClock(CT_0, true);
				PeripheralSetClock(CT_0, false);
				currentTicks++;
			}
		}
	}
}
//---------------------------------------------------------------------------
void IifTimer::Timer0OutChange(TPITCounter counter, bool outState)
{
	if (IsPMD85() && !outState) {
		// Mouse 602 (Ing. Vit Libovicky concept)
		if (mouse602)
			cpu->DoInterrupt();

		// MIF 85 interface
		else if (ifMIF85 && mif85 != NULL && mif85->InterruptEnabled())
			cpu->DoInterrupt();
	}
}
//---------------------------------------------------------------------------
void IifTimer::CT2Clock(TPITCounter counter, bool outState)
{
	PeripheralSetClock(CT_2, outState);
}
//---------------------------------------------------------------------------
void IifTimer::Mouse602Clock(TPITCounter counter, bool outState)
{
	PeripheralSetClock(CT_0, outState);
}
//---------------------------------------------------------------------------
void IifTimer::EnableMouse602(bool enable)
{
	if (IsPMD85())
		mouse602 = enable;
	else
		mouse602 = false;

	if (mouse602)
		Counters[1].OnOutChange.connect(this, &IifTimer::Mouse602Clock);
	else
		Counters[1].OnOutChange.disconnect_all();
}
//---------------------------------------------------------------------------
void IifTimer::EnableMIF85(bool enable, Mif85 *_mif85)
{
	if (IsPMD85()) {
		ifMIF85 = enable;
		mif85 = _mif85;
	}
	else {
		ifMIF85 = false;
		mif85 = NULL;
	}
}
//---------------------------------------------------------------------------
