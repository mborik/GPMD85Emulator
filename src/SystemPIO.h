/*	SystemPIO.h: Class for emulation of system PIO
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

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
#ifndef SystemPIOH
#define SystemPIOH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIO8255.h"
#include "ChipMemory.h"
//---------------------------------------------------------------------------
#define BEEP_FREQ_SEPARATED

#define SYSTEM_PIO_MASK   0x8C
#define SYSTEM_PIO_ADR    0x84

#define SYSTEM_REG_MASK   0x8F
#define SYSTEM_REG_A      0x84
#define SYSTEM_REG_B      0x85
#define SYSTEM_REG_C      0x86
#define SYSTEM_REG_CWR    0x87

#ifdef BEEP_FREQ_SEPARATED
#define HALF_PERIOD_1KH   ((CPU_FREQ / 1000) / 2) // 1 kHz  (R9 from horizontal synchronization)
#define HALF_PERIOD_4KH   ((CPU_FREQ / 4000) / 2) // 4 kHz  (R7 from horizontal synchronization)
#else
#define R_CNT_PERIOD      (CPU_FREQ / 1024000)    // STB 1,024 MHz
#define R_MAX_COUNT       (20480 * R_CNT_PERIOD)
#define R9_MASK           (1 << (9 + 1))
#define R7_MASK           (1 << (7 + 1))
#endif
//---------------------------------------------------------------------------
// keyboard map structure
typedef struct {
	WORD vkey;
	BYTE column;
	BYTE rowmask;
} KEYMAP;
//---------------------------------------------------------------------------
class SystemPIO : public PeripheralDevice, public ChipPIO8255 {
public:
	SystemPIO(TComputerModel model, ChipMemory *mem);

	virtual void ResetDevice(int ticks);
	virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE ReadFromDevice(BYTE port, int ticks);

	void ScanKeyboard(BYTE *keybuf);
	void SoundService(int ticks, int dur);
	void inline SetMatoTapeIn(bool _matoTapeIn)
		{ matoTapeIn = _matoTapeIn; }

	sigslot::signal<int, bool, int> PrepareSample;

	BYTE ledState;
	BYTE width384;
	bool exchZY;
	bool numpad;
	bool extMato;

private:
	void ReadKeyboardB();
	void ReadKeyboardC();

	void WriteSound();
	void WritePaging();

	ChipMemory *memory;
	TComputerModel model;

	int currentTicks;
#ifdef BEEP_FREQ_SEPARATED
	int cnt1kh;
	int cnt4kh;
	bool state1kh;
	bool state4kh;
#else
	int videoCounter;
#endif

	static KEYMAP KeyMap[];
	static KEYMAP KeyMapNumpad[];
	static KEYMAP KeyMapMato[];
	static KEYMAP KeyMapMatoExt[];
	BYTE ShiftStopCtrl;
	BYTE KeyColumns[16];

	bool matoTapeIn;
	bool matoTapeOut;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
