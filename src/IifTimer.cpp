/*	IifTimer.cpp: Class for emulation of timer interface
	Copyright (c) 2006-2018 Roman Borik <pmd85emu@gmail.com>

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
		Counters[1].OnOutChange.connect(&IifTimer::CT2Clock, this);
	else
		Counters[0].OnOutChange.connect(&IifTimer::Timer0OutChange, this);
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

		// Timer T0 - clock for MIF 85 interrupt or Mouse 602
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
	mouse602 = (IsPMD85() || model == CM_C2717) ? enable : false;
	if (mouse602)
		Counters[1].OnOutChange602.connect(&IifTimer::Mouse602Clock, this);
	else
		Counters[1].OnOutChange602.disconnect(this);
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
