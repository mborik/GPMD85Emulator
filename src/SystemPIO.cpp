/*	SystemPIO.cpp: Class for emulation of system PIO
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
#include "SystemPIO.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
SystemPIO::SystemPIO(TComputerModel model, ChipMemory *memory) : ChipPIO8255(false)
{
	this->model = model;
	this->memory = memory;

	PrepareSample.disconnect_all();

	if (model == CM_MATO)
		OnCpuReadC.connect(this, &SystemPIO::ReadKeyboardC);

	OnCpuReadB.connect(this, &SystemPIO::ReadKeyboardB);
	OnCpuWriteCL.connect(this, &SystemPIO::WriteSound);
	OnCpuWriteCH.connect(this, &SystemPIO::WritePaging);

	ledState = 0;
	width384 = 0;
	ShiftStopCtrl = 0;
	for (int ii = 0; ii < (int) sizeof(KeyColumns); ii++)
		KeyColumns[ii] = 0;
	exchZY = false;
	numpad = false;
	extMato = false;

#ifdef BEEP_FREQ_SEPARATED
	cnt1kh = 0;
	cnt4kh = 0;
	state1kh = false;
	state4kh = false;
#else
	videoCounter = 0;
#endif
}
//---------------------------------------------------------------------------
// methods inherited from PeripheralDevice class
//---------------------------------------------------------------------------
/**
 * Method is executed after CPU reset.
 */
void SystemPIO::resetDevice(int ticks)
{
	currentTicks = ticks;
	ChipReset(false);
}
//---------------------------------------------------------------------------
/**
 * Method is called by CPU during write to ports of system PIO.
 */
void SystemPIO::writeToDevice(BYTE port, BYTE value, int ticks)
{
	currentTicks = ticks;

	if (memory->Page == 2 && model != CM_C2717)
		memory->Page = 1;

	switch (port & SYSTEM_REG_MASK) {
		case SYSTEM_REG_A:
			CpuWrite(PP_PortA, value);
			break;

		case SYSTEM_REG_B:
			CpuWrite(PP_PortB, value);
			break;

		case SYSTEM_REG_C:
			CpuWrite(PP_PortC, value);
			break;

		case SYSTEM_REG_CWR:
			CpuWrite(PP_CWR, value);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Method is called by CPU during read from ports of system PIO.
 */
BYTE SystemPIO::readFromDevice(BYTE port, int ticks)
{
	BYTE retval;

	if (memory->Page == 2 && model == CM_C2717)
		memory->Page = 1;

	switch (port & SYSTEM_REG_MASK) {
		case SYSTEM_REG_A:
			retval = CpuRead(PP_PortA);
			break;

		case SYSTEM_REG_B:
			retval = CpuRead(PP_PortB);
			break;

		case SYSTEM_REG_C:
			retval = CpuRead(PP_PortC);
			break;

		case SYSTEM_REG_CWR:
			retval = CpuRead(PP_CWR);
			break;

		default:
			retval = 0xFF;
			break;
	}

	return retval;
}
//---------------------------------------------------------------------------
// KEYBOARD MAPS
// vkey, column, rowmask // comment: PMD key (PC key)
KEYMAP SystemPIO::KeyMap[] = {
	{ SDL_SCANCODE_TAB,          13,  1 },  // C-D       (Tab)
	{ SDL_SCANCODE_KP_ENTER,     13, 16 },  // left EOL  (Enter)
	{ SDL_SCANCODE_RETURN,       14, 16 },  // right EOL (Enter)
//	{ SDL_SCANCODE_CAPSLOCK,     12, 16 },  // CapsLock for PMD 85-3 we can't handle
//	                                // because of weird SDL CapsLock behavior
	{ SDL_SCANCODE_SPACE,         0, 16 },  // SPACE
	{ SDL_SCANCODE_PAGEUP,       14,  1 },  // RCL    (Page Up)
	{ SDL_SCANCODE_PAGEDOWN,     14,  2 },  // CLR    (Page Down)
	{ SDL_SCANCODE_END,          13,  8 },  // END    (End)
	{ SDL_SCANCODE_HOME,         13,  4 },  // HOME   (Home)
	{ SDL_SCANCODE_LEFT,         12,  4 },  // <--    (Left)
	{ SDL_SCANCODE_UP,           12,  8 },  // [<--   (Up)
	{ SDL_SCANCODE_RIGHT,        14,  4 },  // -->    (Right)
	{ SDL_SCANCODE_DOWN,         14,  8 },  // -->]   (Down)
	{ SDL_SCANCODE_INSERT,       12,  2 },  // INS    (Insert)
	{ SDL_SCANCODE_DELETE,       13,  2 },  // DEL    (Delete)
	{ SDL_SCANCODE_0,             9,  2 },  // 0
	{ SDL_SCANCODE_1,             0,  2 },  // 1
	{ SDL_SCANCODE_2,             1,  2 },  // 2
	{ SDL_SCANCODE_3,             2,  2 },  // 3
	{ SDL_SCANCODE_4,             3,  2 },  // 4
	{ SDL_SCANCODE_5,             4,  2 },  // 5
	{ SDL_SCANCODE_6,             5,  2 },  // 6
	{ SDL_SCANCODE_7,             6,  2 },  // 7
	{ SDL_SCANCODE_8,             7,  2 },  // 8
	{ SDL_SCANCODE_9,             8,  2 },  // 9
	{ SDL_SCANCODE_A,             0,  8 },  // A
	{ SDL_SCANCODE_B,             5, 16 },  // B
	{ SDL_SCANCODE_C,             3, 16 },  // C
	{ SDL_SCANCODE_D,             2,  8 },  // D
	{ SDL_SCANCODE_E,             2,  4 },  // E
	{ SDL_SCANCODE_F,             3,  8 },  // F
	{ SDL_SCANCODE_G,             4,  8 },  // G
	{ SDL_SCANCODE_H,             5,  8 },  // H
	{ SDL_SCANCODE_I,             7,  4 },  // I
	{ SDL_SCANCODE_J,             6,  8 },  // J
	{ SDL_SCANCODE_K,             7,  8 },  // K
	{ SDL_SCANCODE_L,             8,  8 },  // L
	{ SDL_SCANCODE_M,             7, 16 },  // M
	{ SDL_SCANCODE_N,             6, 16 },  // N
	{ SDL_SCANCODE_O,             8,  4 },  // O
	{ SDL_SCANCODE_P,             9,  4 },  // P
	{ SDL_SCANCODE_Q,             0,  4 },  // Q
	{ SDL_SCANCODE_R,             3,  4 },  // R
	{ SDL_SCANCODE_S,             1,  8 },  // S
	{ SDL_SCANCODE_T,             4,  4 },  // T
	{ SDL_SCANCODE_U,             6,  4 },  // U
	{ SDL_SCANCODE_V,             4, 16 },  // V
	{ SDL_SCANCODE_W,             1,  4 },  // W
	{ SDL_SCANCODE_X,             2, 16 },  // X
	{ SDL_SCANCODE_Y,             5,  4 },  // Z
	{ SDL_SCANCODE_Z,             1, 16 },  // Y
	{ SDL_SCANCODE_F1,            0,  1 },  // K0   (F1)
	{ SDL_SCANCODE_F2,            1,  1 },  // K1   (F2)
	{ SDL_SCANCODE_F3,            2,  1 },  // K2   (F3)
	{ SDL_SCANCODE_F4,            3,  1 },  // K3   (F4)
	{ SDL_SCANCODE_F5,            4,  1 },  // K4   (F5)
	{ SDL_SCANCODE_F6,            5,  1 },  // K5   (F6)
	{ SDL_SCANCODE_F7,            6,  1 },  // K6   (F7)
	{ SDL_SCANCODE_F8,            7,  1 },  // K7   (F8)
	{ SDL_SCANCODE_F9,            8,  1 },  // K8   (F9)
	{ SDL_SCANCODE_F10,           9,  1 },  // K9   (F10)
	{ SDL_SCANCODE_F11,          10,  1 },  // K10  (F11)
	{ SDL_SCANCODE_F12,          11,  1 },  // K11  (F12)
	{ SDL_SCANCODE_SEMICOLON,     9,  8 },  // ;    (')
	{ SDL_SCANCODE_EQUALS,       11,  8 },  // [ ]  (=)
	{ SDL_SCANCODE_COMMA,         8, 16 },  // ,    (,)
	{ SDL_SCANCODE_MINUS,        10,  2 },  // _    (-)
	{ SDL_SCANCODE_PERIOD,        9, 16 },  // .    (.)
	{ SDL_SCANCODE_SLASH,        10, 16 },  // /    (/)
	{ SDL_SCANCODE_GRAVE,        12,  1 },  // WRK  (`)
	{ SDL_SCANCODE_LEFTBRACKET,  10,  4 },  // @    ([)
	{ SDL_SCANCODE_BACKSLASH,    11,  2 },  // { }  (\)
	{ SDL_SCANCODE_RIGHTBRACKET, 11,  4 },  // \    (])
	{ SDL_SCANCODE_SEMICOLON,    10,  8 },  // :    (;)
	{ 0, 0, 0 }
};

// NUMERIC KEYPAD MAP
// vkey, column, rowmask
KEYMAP SystemPIO::KeyMapNumpad[] = {
	{ SDL_SCANCODE_KP_0,        9,   2 },  // Num 0
	{ SDL_SCANCODE_KP_1,        0,   2 },  // Num 1
	{ SDL_SCANCODE_KP_2,        1,   2 },  // Num 2
	{ SDL_SCANCODE_KP_3,        2,   2 },  // Num 3
	{ SDL_SCANCODE_KP_4,        3,   2 },  // Num 4
	{ SDL_SCANCODE_KP_5,        4,   2 },  // Num 5
	{ SDL_SCANCODE_KP_6,        5,   2 },  // Num 6
	{ SDL_SCANCODE_KP_7,        6,   2 },  // Num 7
	{ SDL_SCANCODE_KP_8,        7,   2 },  // Num 8
	{ SDL_SCANCODE_KP_9,        8,   2 },  // Num 9
	{ SDL_SCANCODE_KP_PERIOD,   9,  16 },  // Num .
	{ SDL_SCANCODE_KP_DIVIDE,   10, 16 },  // Num /
	{ SDL_SCANCODE_KP_MULTIPLY, 10, 40 },  // Num *
	{ SDL_SCANCODE_KP_PLUS,     9,  40 },  // Num +
	{ SDL_SCANCODE_KP_MINUS,    9,  34 },  // Num -
	{ 0, 0, 0 }
};

// MATO SPECIAL KEYBOARD MAP
KEYMAP SystemPIO::KeyMapMato[] = {
	{ SDL_SCANCODE_SPACE,        6,  4 },  // SPACE
	{ SDL_SCANCODE_LEFT,         6, 16 },  // Left
	{ SDL_SCANCODE_UP,           6,  2 },  // Up
	{ SDL_SCANCODE_RIGHT,        6, 32 },  // Right
	{ SDL_SCANCODE_DOWN,         6,  1 },  // Down
	{ SDL_SCANCODE_0,            4,  2 },  // 0
	{ SDL_SCANCODE_1,            0,  1 },  // 1
	{ SDL_SCANCODE_2,            0,  2 },  // 2
	{ SDL_SCANCODE_3,            0,  4 },  // 3
	{ SDL_SCANCODE_4,            0,  8 },  // 4
	{ SDL_SCANCODE_5,            0, 16 },  // 5
	{ SDL_SCANCODE_6,            0, 32 },  // 6
	{ SDL_SCANCODE_7,            0, 64 },  // 7
	{ SDL_SCANCODE_8,            4,  8 },  // 8
	{ SDL_SCANCODE_9,            5,  8 },  // 9
	{ SDL_SCANCODE_A,            2,  1 },  // A
	{ SDL_SCANCODE_B,            3, 16 },  // B
	{ SDL_SCANCODE_C,            3,  4 },  // C
	{ SDL_SCANCODE_D,            2,  4 },  // D
	{ SDL_SCANCODE_E,            1,  4 },  // E
	{ SDL_SCANCODE_F,            2,  8 },  // F
	{ SDL_SCANCODE_G,            2, 16 },  // G
	{ SDL_SCANCODE_H,            2, 32 },  // H
	{ SDL_SCANCODE_I,            4, 64 },  // I
	{ SDL_SCANCODE_J,            2, 64 },  // J
	{ SDL_SCANCODE_K,            4, 32 },  // K
	{ SDL_SCANCODE_L,            5, 32 },  // L
	{ SDL_SCANCODE_M,            3, 64 },  // M
	{ SDL_SCANCODE_N,            3, 32 },  // N
	{ SDL_SCANCODE_O,            5, 64 },  // O
	{ SDL_SCANCODE_P,            5,  1 },  // P
	{ SDL_SCANCODE_Q,            1,  1 },  // Q
	{ SDL_SCANCODE_R,            1,  8 },  // R
	{ SDL_SCANCODE_S,            2,  2 },  // S
	{ SDL_SCANCODE_T,            1, 16 },  // T
	{ SDL_SCANCODE_U,            1, 64 },  // U
	{ SDL_SCANCODE_V,            3,  8 },  // V
	{ SDL_SCANCODE_W,            1,  2 },  // W
	{ SDL_SCANCODE_X,            3,  2 },  // X
	{ SDL_SCANCODE_Y,            1, 32 },  // Z
	{ SDL_SCANCODE_Z,            3,  1 },  // Y
	{ SDL_SCANCODE_SEMICOLON,    5,  4 },  // ;  (')
	{ SDL_SCANCODE_COMMA,        4, 16 },  // ,  (,)
	{ SDL_SCANCODE_MINUS,        4,  1 },  // -  (-)
	{ SDL_SCANCODE_PERIOD,       5, 16 },  // .  (.)
	{ SDL_SCANCODE_SLASH,        5,  2 },  // /  (/)
	{ SDL_SCANCODE_LEFTBRACKET,  6,  8 },  // @  ([)
	{ SDL_SCANCODE_RIGHTBRACKET, 4,  4 },  // \  (])
	{ SDL_SCANCODE_SEMICOLON,    6, 64 },  // :  (;)
	{ 0, 0, 0 }
};

KEYMAP SystemPIO::KeyMapMatoExt[] = {
	{ SDL_SCANCODE_KP_0,        4,   2 },  // Num 0
	{ SDL_SCANCODE_KP_1,        0,   1 },  // Num 1
	{ SDL_SCANCODE_KP_2,        0,   2 },  // Num 2
	{ SDL_SCANCODE_KP_3,        0,   4 },  // Num 3
	{ SDL_SCANCODE_KP_4,        0,   8 },  // Num 4
	{ SDL_SCANCODE_KP_5,        0,  16 },  // Num 5
	{ SDL_SCANCODE_KP_6,        0,  32 },  // Num 6
	{ SDL_SCANCODE_KP_7,        0,  64 },  // Num 7
	{ SDL_SCANCODE_KP_8,        4,   8 },  // Num 8
	{ SDL_SCANCODE_KP_9,        5,   8 },  // Num 9
	{ SDL_SCANCODE_KP_PERIOD,   5,  16 },  // Num .
	{ SDL_SCANCODE_KP_DIVIDE,   5,   2 },  // Num /
	{ SDL_SCANCODE_KP_MINUS,    4,   1 },  // Num -
	{ SDL_SCANCODE_KP_MULTIPLY, 6, 192 },  // Num *
	{ SDL_SCANCODE_KP_PLUS,     5, 132 },  // Num +

	// CNT bit added - idx=15
	{ SDL_SCANCODE_TAB,         3,   4 },  // C-D    (Tab)
	{ SDL_SCANCODE_PAGEUP,      3,   2 },  // RCL    (Page Up)
	{ SDL_SCANCODE_PAGEDOWN,    3,  16 },  // CLL    (Page Down)
	{ SDL_SCANCODE_END,         6,  32 },  // ENDL   (End)
	{ SDL_SCANCODE_HOME,        6,   2 },  // BGNL   (Home)
	{ SDL_SCANCODE_INSERT,      3,  64 },  // INST   (Insert)
	{ SDL_SCANCODE_DELETE,      3,  32 },  // DELT   (Delete)
	{ SDL_SCANCODE_GRAVE,       3,   1 },  // WRK

	{ SDL_SCANCODE_F1,          0,   1 },  // K0     (F1)
	{ SDL_SCANCODE_F2,          0,   2 },  // K1     (F2)
	{ SDL_SCANCODE_F3,          0,   4 },  // K2     (F3)
	{ SDL_SCANCODE_F4,          0,   8 },  // K3     (F4)
	{ SDL_SCANCODE_F5,          0,  16 },  // K4     (F5)
	{ SDL_SCANCODE_F6,          0,  32 },  // K5     (F6)
	{ SDL_SCANCODE_F7,          0,  64 },  // K6     (F7)
	{ SDL_SCANCODE_F8,          4,   8 },  // K7     (F8)
	{ SDL_SCANCODE_F9,          5,   8 },  // K8     (F9)
	{ SDL_SCANCODE_F10,         4,   2 },  // K9     (F10)
	{ SDL_SCANCODE_F11,         4,   1 },  // K10    (F11)
	{ SDL_SCANCODE_F12,         4,   4 },  // K11    (F12)
	{ 0, 0, 0 },

	// shifted - idx=36
	{ SDL_SCANCODE_INSERT,      5,   2 },  // PTL    (Shift + Insert)
	{ SDL_SCANCODE_PAGEUP,      4,  16 },  // MON    (Shift + Page Up)
	{ SDL_SCANCODE_PAGEDOWN,    5,  16 },  // DELL   (Shift + Page Down)
	{ SDL_SCANCODE_END,         6,  16 },  // BEEP   (Shift + End)
	{ SDL_SCANCODE_HOME,        3,   8 },  // CLS    (Shift + Home)
	{ 0, 0, 0 }
};
//---------------------------------------------------------------------------
/**
 * This method identifies a map of keyboard matrix based on keys pressed.
 * It is called repeatedly by Refresh timer.
 * The keymap is being searched based on computer model.
 */
void SystemPIO::ScanKeyboard(BYTE *KeyBuffer)
{
	static BYTE keyExchZY, keyExchYZ;

	if (KeyBuffer == NULL)
		return;

	// pressing of ALT/META resets whole matrix because it's used for hotkeys
	if (KeyBuffer[SDL_SCANCODE_APPLICATION] || KeyBuffer[SDL_SCANCODE_LALT] || KeyBuffer[SDL_SCANCODE_RALT]
	 || KeyBuffer[SDL_SCANCODE_LGUI] || KeyBuffer[SDL_SCANCODE_RGUI]) {
		for (int ii = 0; ii < (int)sizeof(KeyColumns); ii++)
			KeyColumns[ii] = 0;
		ShiftStopCtrl = 0;
		return;
	}

	if (exchZY == true) {
		keyExchZY = KeyBuffer[SDL_SCANCODE_Z];
		KeyBuffer[SDL_SCANCODE_Z] = keyExchYZ = KeyBuffer[SDL_SCANCODE_Y];
		KeyBuffer[SDL_SCANCODE_Y] = keyExchZY;
	}

	int bi = 0;
	if (model == CM_MATO) { // ********** MATO **********
		while (KeyMapMato[bi].vkey) {
			if (KeyBuffer[KeyMapMato[bi].vkey])
				KeyColumns[KeyMapMato[bi].column] |= KeyMapMato[bi].rowmask;
			else
				KeyColumns[KeyMapMato[bi].column] &= ~KeyMapMato[bi].rowmask;
			bi++;
		}

		// BackSpace
		if (KeyBuffer[SDL_SCANCODE_BACKSPACE] && !KeyBuffer[SDL_SCANCODE_LEFT])
			KeyColumns[6] |= 16;

		// EOL
		if (KeyBuffer[SDL_SCANCODE_RETURN] || KeyBuffer[SDL_SCANCODE_KP_ENTER])
			KeyColumns[7] |= 0x80;
		else
			KeyColumns[7] &= ~0x80;

		// STOP
		if (KeyBuffer[SDL_SCANCODE_ESCAPE])
			ShiftStopCtrl |= 0x10;
		else
			ShiftStopCtrl &= ~0x10;

		// SHF
		if (KeyBuffer[SDL_SCANCODE_LSHIFT] || KeyBuffer[SDL_SCANCODE_RSHIFT])
			ShiftStopCtrl |= 0x20;
		else
			ShiftStopCtrl &= ~0x20;

		// CNT
		if (KeyBuffer[SDL_SCANCODE_LCTRL] || KeyBuffer[SDL_SCANCODE_RCTRL])
			ShiftStopCtrl |= 0x40;
		else
			ShiftStopCtrl &= ~0x40;

		// control keys and numeric keypad
		if (extMato == true) {
			if (KeyBuffer[SDL_SCANCODE_LSHIFT] || KeyBuffer[SDL_SCANCODE_RSHIFT])
				bi = 36;
			else
				bi = 0;

			while (KeyMapMatoExt[bi].vkey) {
				if (KeyBuffer[KeyMapMatoExt[bi].vkey]) {
					KeyColumns[KeyMapMatoExt[bi].column] |= (KeyMapMatoExt[bi].rowmask & 127);
					if (bi >= 15)
						ShiftStopCtrl |= 0x40;  // CNT
					if (KeyMapMatoExt[bi].rowmask & 128)
						ShiftStopCtrl |= 0x20;  // SHF
					if (bi >= 36)
						ShiftStopCtrl &= ~0x20;
				}
				bi++;
			}
		}

	}
	else {  // ********** PMD 85 **********
		while (KeyMap[bi].vkey) {
			if (KeyBuffer[KeyMap[bi].vkey])
				KeyColumns[KeyMap[bi].column] |= KeyMap[bi].rowmask;
			else
				KeyColumns[KeyMap[bi].column] &= ~KeyMap[bi].rowmask;
			bi++;
		}

		// BackSpace
		if (KeyBuffer[SDL_SCANCODE_BACKSPACE] && !KeyBuffer[SDL_SCANCODE_LEFT])
			KeyColumns[12] |= 4;

		// SHIFT
		if (KeyBuffer[SDL_SCANCODE_LSHIFT] || KeyBuffer[SDL_SCANCODE_RSHIFT])
			ShiftStopCtrl |= 0x20;
		else
			ShiftStopCtrl &= ~0x20;

		// STOP
		if (KeyBuffer[SDL_SCANCODE_ESCAPE] || KeyBuffer[SDL_SCANCODE_LCTRL] || KeyBuffer[SDL_SCANCODE_RCTRL])
			ShiftStopCtrl |= 0x40;
		else
			ShiftStopCtrl &= ~0x40;

		// numeric keypad
		if (numpad == true) {
			bi = 0;
			while (KeyMapNumpad[bi].vkey) {
				if (KeyBuffer[KeyMapNumpad[bi].vkey]) {
					KeyColumns[KeyMapNumpad[bi].column] |= (KeyMapNumpad[bi].rowmask & 31);
					if (KeyMapNumpad[bi].rowmask & 32)
						ShiftStopCtrl |= 0x20;
				}
				bi++;
			}
		}
	}

	if (exchZY == true) {
		KeyBuffer[SDL_SCANCODE_Z] = keyExchZY;
		KeyBuffer[SDL_SCANCODE_Y] = keyExchYZ;
	}

}
//---------------------------------------------------------------------------
/**
 * Method is used as notification function during reading of Port B.
 * Method setup status of Port B which represents currently addressed column
 * of key matrix.
 */
void SystemPIO::ReadKeyboardB()
{
	BYTE val, col;

	if (model == CM_MATO) {
		col = PeripheralReadByte(PP_PortA);
		val = 0;
		for (int xx = 0; xx < 7; xx++) {
			if ((col & 1) == 0)
				val |= KeyColumns[xx];
			col >>= 1;
		}
		val |= KeyColumns[7];
		val = (BYTE) (~val);
	}
	else
		val = (BYTE) ((~KeyColumns[PeripheralReadByte(PP_PortA) & 0x0F] & 0x1F)
			| (~ShiftStopCtrl & 0x60));

	PeripheralWriteByte(PP_PortB, val);
}
//---------------------------------------------------------------------------
/**
 * Method is used as notification function during readin of Port C for
 * computer MATO. Method setup status of Port C which represents status
 * of Shift, Ctrl & Stop keys.
 */
void SystemPIO::ReadKeyboardC()
{
	PeripheralWriteByte(PP_PortC, (BYTE) ((~ShiftStopCtrl & 0x70) | 0x01));
}
//---------------------------------------------------------------------------
/**
 * Method is used as notification function while upper bits of port C are
 * being written Method handles memory paging.
 */
void SystemPIO::WritePaging()
{
	BYTE pg = PeripheralReadByte(PP_PortC);

	if (model == CM_C2717) {
		width384 = ((pg & 32) + 1);         // 48/64 chars per line mode
		memory->Page = ((pg & 64) ? 0 : 1); // AllRAM
		memory->C2717Remapped = (pg & 128); // re-adressing to C000h
	}
	else {
		if (model == CM_V2A || model == CM_V3) {
			if (model == CM_V3 && (pg & 32))
				memory->Page = 2; // ROM only
			else if (pg & 16)
				memory->Page = 1; // ROM/RAM
			else
				memory->Page = 0; // AllRAM
		}
	}

	// AllRAM
	if (memory->Page == 0)
		ledState |= LED_BLUE;
	else
		ledState &= ~LED_BLUE;
}
//---------------------------------------------------------------------------
/**
 * Method is used as notification function while lower bits of port C are
 * being written. Method handles speaker and LED.
 */
void SystemPIO::WriteSound()
{
	BYTE beep = PeripheralReadByte(PP_PortC);

	if (model == CM_MATO) {
		PrepareSample(CHNL_SPEAKER, beep & 6, currentTicks);

		if (beep & 6)
			ledState |= LED_YELLOW;
		else
			ledState &= ~LED_YELLOW;
	}
	else {
#ifdef BEEP_FREQ_SEPARATED
		bool out = (beep & 4) || ((beep & 2) && state4kh) || ((beep & 1) && state1kh);
#else
		bool out = (beep & 4)
		       || ((beep & 2) && (videoCounter & R7_MASK))
		       || ((beep & 1) && (videoCounter & R9_MASK));
#endif

		PrepareSample(CHNL_SPEAKER, out, currentTicks);

		if (out)
			ledState |= LED_YELLOW;
		else
			ledState &= ~LED_YELLOW;

		if (beep & 8)
			ledState |= LED_RED;
		else
			ledState &= ~LED_RED;
	}
}
//---------------------------------------------------------------------------
/**
 * Method is listener of CPU ticks.
 * It generates 1kHz and 4kHz signals (tones) which are derived from screen
 * synchronization.
 */
void SystemPIO::SoundService(int ticks, int dur)
{
	BYTE beep = PeripheralReadByte(PP_PortC);
	bool out;

#ifdef BEEP_FREQ_SEPARATED
	cnt4kh += dur;
	if (cnt4kh >= HALF_PERIOD_4KH) {
		cnt4kh -= HALF_PERIOD_4KH;
		state4kh = !state4kh;
		cnt1kh++;
		if ((cnt1kh % 4) == 0)
			state1kh = !state1kh;

		out = (beep & 4) || ((beep & 2) && state4kh)
				|| ((beep & 1) && state1kh);

		PrepareSample(CHNL_SPEAKER, out, ticks - cnt4kh);

		if (out)
			ledState |= LED_YELLOW;
		else
			ledState &= ~LED_YELLOW;
	}
#else
	videoCounter = (videoCounter + dur) % R_MAX_COUNT;

	out = (beep & 4)
		|| ((beep & 2) && (videoCounter & R7_MASK))
		|| ((beep & 1) && (videoCounter & R9_MASK));
	PrepareSample(CHNL_SPEAKER, out, ticks);

	if (out)
		ledState |= LED_YELLOW;
	else
		ledState &= ~LED_YELLOW;
#endif
}
//---------------------------------------------------------------------------
