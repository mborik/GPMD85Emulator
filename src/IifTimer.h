/*  IifTimer.h: Class for emulation of timer interface
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

