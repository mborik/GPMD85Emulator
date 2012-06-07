/*  IifTimer.cpp: Class for emulation of timer interface
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
//---------------------------------------------------------------------------
#include "IifTimer.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
IifTimer::IifTimer(TComputerModel model) : ChipPIT8253()
{
	this->model = model;

	cntRtc = 0;
	stateRtc = true;
	ctUsartOn = false;
	mouse602 = false;
	cpu = NULL;

	PeripheralSetGate(CT_0, true);
	PeripheralSetClock(CT_0, false);
	PeripheralSetGate(CT_1, true);
	PeripheralSetClock(CT_1, false);
	PeripheralSetGate(CT_2, true);
	PeripheralSetClock(CT_2, stateRtc);

	if (model == CM_V1 && model == CM_V2 && model == CM_V2A && model == CM_V3)
		Counters[0].OnOutChange.connect(this, &IifTimer::Timer0OutChange);
}
//---------------------------------------------------------------------------
void IifTimer::writeToDevice(BYTE port, BYTE value, int ticks)
{
//	debug("IfTimer", "ticks=%d, port=%u, value=%u", ticks, port, value);

	switch (port & IIF_TIMER_REG_MASK) {
		case IIF_TIMER_REG_T0 :
		CpuWrite(CT_0, value);
		break;

		case IIF_TIMER_REG_T1 :
		CpuWrite(CT_1, value);
		break;

		case IIF_TIMER_REG_T2 :
		CpuWrite(CT_2, value);
		break;

		case IIF_TIMER_REG_CWR :
		CpuWrite(CT_CWR, value);
		break;
	}
}
	//---------------------------------------------------------------------------
BYTE IifTimer::readFromDevice(BYTE port, int ticks)
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
	// Timer T2 - RTC
	cntRtc += dur;
	while (cntRtc >= HALF_SEC_RTC) {
		cntRtc -= HALF_SEC_RTC;
		stateRtc = !stateRtc;
		PeripheralSetClock(CT_2, stateRtc);
	}

	// Timer T1 - clock for USART - PMD 85
	// Timer T0 - clock for USART - C2717
	if (ctUsartOn == true) {
		if (model == CM_C2717) {
			for (int ii = 0; ii < dur; ii++) {
				PeripheralSetClock(CT_0, true);
				PeripheralSetClock(CT_0, false);
			}
		}
		else {
			for (int ii = 0; ii < dur; ii++) {
				PeripheralSetClock(CT_1, true);
				PeripheralSetClock(CT_1, false);
			}
		}
	}

	currentTicks = ticks;
}
//---------------------------------------------------------------------------
void IifTimer::Timer0OutChange(TPITCounter cnt, bool out)
{
	// Mouse 602 (Ing. Vit Libovicky concept)
	if (mouse602 && cpu != NULL && !out)
		cpu->DoInterrupt();
}
//---------------------------------------------------------------------------
void IifTimer::EnableMouse602(bool enable, ChipCpu8080 *_cpu)
{
	mouse602 = (model == CM_V1) ? enable : false;

	if (mouse602 && _cpu)
		Counters[1].OnOutChange.connect(this, &IifTimer::Mouse602Clock);
	else {
		Counters[1].OnOutChange.disconnect_all();
		mouse602 = false;
		_cpu = NULL;
	}

	ctUsartOn = mouse602;
	cpu = _cpu;
}
//---------------------------------------------------------------------------
void IifTimer::Mouse602Clock(TPITCounter counter, bool outState)
{
	PeripheralSetClock(CT_0, outState);
}
//---------------------------------------------------------------------------
