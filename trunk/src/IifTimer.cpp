//---------------------------------------------------------------------------
#include "IifTimer.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
IifTimer::IifTimer() : ChipPIT8253()
{
	cntRtc = 0;
	stateRtc = true;
	ct1On = false;
	musica = false;
	mys602 = false;
	cpu = NULL;

	PeripheralSetGate(CT_0, true);
	PeripheralSetClock(CT_0, false);
	PeripheralSetGate(CT_1, true);
	PeripheralSetClock(CT_1, false);
	PeripheralSetGate(CT_2, true);
	PeripheralSetClock(CT_2, stateRtc);

	Counters[0].OnOutChange.connect(this, &IifTimer::Timer0OutChange);
	ct1On = true;
}
//---------------------------------------------------------------------------
void IifTimer::writeToDevice(BYTE port, BYTE value, int ticks)
{
//	debug("ticks=%d, port=%u, value=%u", ticks, port, value);

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

//	debug("ticks=%d, port=%u, retval=%u", ticks, port, retval);

	return retval;
}
	//---------------------------------------------------------------------------
void IifTimer::ITimerService(int ticks, int dur)
{
	// Casovac T2 - RTC
	cntRtc += dur;
	while (cntRtc >= HALF_SEC_RTC) {
		cntRtc -= HALF_SEC_RTC;
		stateRtc = !stateRtc;
		PeripheralSetClock(CT_2, stateRtc);
	}

	// Casovac T1 - hodiny pre USART
	if (ct1On == true) {
		for (int ii = 0; ii < dur; ii++) {
			PeripheralSetClock(CT_1, true);
			PeripheralSetClock(CT_1, false);
		}
	}

	// Casovac T0 - hodiny pre 1. kanal IF Musica
	currentTicks = ticks;
	if (musica == true) {
		for (int ii = 0; ii < dur; ii++) {
			PeripheralSetClock(CT_0, true);
			PeripheralSetClock(CT_0, false);
			currentTicks++;
		}
	}
}
//---------------------------------------------------------------------------
void IifTimer::Timer0OutChange(TPITCounter cnt, bool out)
{
	// 1. kanal IF Musica
	if (musica == true)
		PrepareSample(CHNL_MUSICA_1, out, currentTicks);

	// Mys 602
	else if (mys602 == true && cpu != NULL && out == false)
		cpu->DoInterrupt();
}
//---------------------------------------------------------------------------
void IifTimer::EnableMouse602(bool enable, ChipCpu8080 *_cpu)
{
	if (enable == true && cpu != NULL) {
		Counters[1].OnOutChange.connect(this, &IifTimer::Mouse602Clock);

		mys602 = enable;
		cpu = _cpu;
		ct1On = true;
	}
}
//---------------------------------------------------------------------------
void IifTimer::Mouse602Clock(TPITCounter counter, bool outState)
{
	PeripheralSetClock(CT_0, outState);
}
//---------------------------------------------------------------------------
