/*	IifTimer.h: Class for emulation of timer interface
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
#ifndef IifTimerH
#define IifTimerH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIT8253.h"
#include "ChipCpu8080.h"
#include "Mif85.h"
//---------------------------------------------------------------------------
// Internal timer using these port addresses: 5Ch, 5Dh, 5Eh a 5Fh
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
	IifTimer(TComputerModel model, ChipCpu8080 *_cpu);

	virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE ReadFromDevice(BYTE port, int ticks);

	void ITimerService(int ticks, int dur);
	void Timer0OutChange(TPITCounter cnt, bool out);
	void CT2Clock(TPITCounter cnt, bool out);
	void Mouse602Clock(TPITCounter counter, bool outState);
	void EnableMouse602(bool enable);
	void EnableMIF85(bool enable, Mif85 *_mif85);

private:
	ChipCpu8080 *cpu;
	TComputerModel model;
	Mif85 *mif85;

	int  cntRtc;
	bool stateRtc;
	int  currentTicks;

	// Mouse 602 (Ing. Vit Libovicky concept)
	bool mouse602;
	// MIF 85 interface
	bool ifMIF85;

	inline bool IsPMD85() {
		return model == CM_V1 || model == CM_V2 || model == CM_V2A || model == CM_V3;
	}
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

