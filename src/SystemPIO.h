/*	SystemPIO.h: Class for emulation of system PIO
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
#ifndef SystemPIOH
#define SystemPIOH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIO8255.h"
#include "ChipMemory.h"
//---------------------------------------------------------------------------
#define SYSTEM_PIO_MASK   0x8C
#define SYSTEM_PIO_ADR    0x84

#define SYSTEM_REG_MASK   0x8F
#define SYSTEM_REG_A      0x84
#define SYSTEM_REG_B      0x85
#define SYSTEM_REG_C      0x86
#define SYSTEM_REG_CWR    0x87

#define BEEP_FREQ_SEPARATED

#ifdef BEEP_FREQ_SEPARATED
#define HALF_PERIOD_1KH   ((CPU_FREQ / 1000) / 2) // 1 kHz  (R9 z rozkl. obrazu)
#define HALF_PERIOD_4KH   ((CPU_FREQ / 4000) / 2) // 4 kHz  (R7 z rozkl. obrazu)
#else
#define R_CNT_PERIOD      (CPU_FREQ / 1024000) // STB 1,024 MHz
#define R_MAX_COUNT       (20480 * R_CNT_PERIOD)
#define R9_MASK           (1 << (9 + 1))
#define R7_MASK           (1 << (7 + 1))
#endif
//---------------------------------------------------------------------------
// struktura mapy klavesnice
typedef struct {
	WORD vkey;
	BYTE column;
	BYTE rowmask;
} KEYMAP;
//---------------------------------------------------------------------------
class SystemPIO : public PeripheralDevice, public ChipPIO8255 {
public:
	SystemPIO(TComputerModel model, ChipMemory *mem);

	virtual void resetDevice(int ticks);
	virtual void writeToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE readFromDevice(BYTE port, int ticks);

	void ScanKeyboard(BYTE *keybuf);
	void SoundService(int ticks, int dur);

	sigslot::signal3<int, bool, int> PrepareSample;

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
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
