//---------------------------------------------------------------------------
#ifndef IifTimerH
#define IifTimerH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIT8253.h"
#include "ChipCpu8080.h"
//---------------------------------------------------------------------------
// Interny casovac pouziva tieto adresy portov: 5Ch, 5Dh, 5Eh a 5Fh
#define IIF_TIMER_MASK      0xFC
#define IIF_TIMER_ADR       0x5C

#define IIF_TIMER_REG_MASK  0xFF
#define IIF_TIMER_REG_T0    0x5C
#define IIF_TIMER_REG_T1    0x5D
#define IIF_TIMER_REG_T2    0x5E
#define IIF_TIMER_REG_CWR   0x5F

#define HALF_SEC_RTC        ((CPU_FREQ / 1) / 2) // 1 Hz
//---------------------------------------------------------------------------
class IifTimer : public PeripheralDevice, public ChipPIT8253 {
public:
	IifTimer();

	virtual void writeToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE readFromDevice(BYTE port, int ticks);

	// pre IF Musica
	sigslot::signal3<int, bool, int> PrepareSample;

	void ITimerService(int ticks, int dur);
	void Timer0OutChange(TPITCounter cnt, bool out);
	void EnableMouse602(bool enable, ChipCpu8080 *_cpu);
	void Mouse602Clock(TPITCounter counter, bool outState);

	inline void EnableUsartClock(bool enable) { ct1On = enable; }
	inline void EnableIfMusica(bool enable) { musica = enable; }

private:
	int cntRtc;
	bool stateRtc;
	bool ct1On;

	bool musica;
	int currentTicks;

	// pre Mys602
	bool mys602;
	ChipCpu8080 *cpu;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

