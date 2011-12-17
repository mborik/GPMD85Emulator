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
// metody zdedene z triedy PeripheralDevice
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri jeho resete.
 */
void SystemPIO::resetDevice(int ticks)
{
	currentTicks = ticks;
	ChipReset(false);
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty systemoveho PIO.
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
 * Metoda je volana procesorom pri citani z portov systemoveho PIO.
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
	{ SDLK_TAB,          13,  1 },  // C-D       (Tab)
	{ SDLK_KP_ENTER,     13, 16 },  // left EOL  (Enter)
	{ SDLK_RETURN,       14, 16 },  // right EOL (Enter)
//	{ SDLK_CAPSLOCK,     12, 16 },  // CapsLock for PMD 85-3 we can't handle
//	                                // because of weird SDL CapsLock behavior
	{ SDLK_SPACE,         0, 16 },  // SPACE
	{ SDLK_PAGEUP,       14,  1 },  // RCL    (Page Up)
	{ SDLK_PAGEDOWN,     14,  2 },  // CLR    (Page Down)
	{ SDLK_END,          13,  8 },  // END    (End)
	{ SDLK_HOME,         13,  4 },  // HOME   (Home)
	{ SDLK_LEFT,         12,  4 },  // <--    (Left)
	{ SDLK_UP,           12,  8 },  // [<--   (Up)
	{ SDLK_RIGHT,        14,  4 },  // -->    (Right)
	{ SDLK_DOWN,         14,  8 },  // -->]   (Down)
	{ SDLK_INSERT,       12,  2 },  // INS    (Insert)
	{ SDLK_DELETE,       13,  2 },  // DEL    (Delete)
	{ SDLK_0,             9,  2 },  // 0
	{ SDLK_1,             0,  2 },  // 1
	{ SDLK_2,             1,  2 },  // 2
	{ SDLK_3,             2,  2 },  // 3
	{ SDLK_4,             3,  2 },  // 4
	{ SDLK_5,             4,  2 },  // 5
	{ SDLK_6,             5,  2 },  // 6
	{ SDLK_7,             6,  2 },  // 7
	{ SDLK_8,             7,  2 },  // 8
	{ SDLK_9,             8,  2 },  // 9
	{ SDLK_a,             0,  8 },  // A
	{ SDLK_b,             5, 16 },  // B
	{ SDLK_c,             3, 16 },  // C
	{ SDLK_d,             2,  8 },  // D
	{ SDLK_e,             2,  4 },  // E
	{ SDLK_f,             3,  8 },  // F
	{ SDLK_g,             4,  8 },  // G
	{ SDLK_h,             5,  8 },  // H
	{ SDLK_i,             7,  4 },  // I
	{ SDLK_j,             6,  8 },  // J
	{ SDLK_k,             7,  8 },  // K
	{ SDLK_l,             8,  8 },  // L
	{ SDLK_m,             7, 16 },  // M
	{ SDLK_n,             6, 16 },  // N
	{ SDLK_o,             8,  4 },  // O
	{ SDLK_p,             9,  4 },  // P
	{ SDLK_q,             0,  4 },  // Q
	{ SDLK_r,             3,  4 },  // R
	{ SDLK_s,             1,  8 },  // S
	{ SDLK_t,             4,  4 },  // T
	{ SDLK_u,             6,  4 },  // U
	{ SDLK_v,             4, 16 },  // V
	{ SDLK_w,             1,  4 },  // W
	{ SDLK_x,             2, 16 },  // X
	{ SDLK_y,             5,  4 },  // Z
	{ SDLK_z,             1, 16 },  // Y
	{ SDLK_F1,            0,  1 },  // K0   (F1)
	{ SDLK_F2,            1,  1 },  // K1   (F2)
	{ SDLK_F3,            2,  1 },  // K2   (F3)
	{ SDLK_F4,            3,  1 },  // K3   (F4)
	{ SDLK_F5,            4,  1 },  // K4   (F5)
	{ SDLK_F6,            5,  1 },  // K5   (F6)
	{ SDLK_F7,            6,  1 },  // K6   (F7)
	{ SDLK_F8,            7,  1 },  // K7   (F8)
	{ SDLK_F9,            8,  1 },  // K8   (F9)
	{ SDLK_F10,           9,  1 },  // K9   (F10)
	{ SDLK_F11,          10,  1 },  // K10  (F11)
	{ SDLK_F12,          11,  1 },  // K11  (F12)
	{ SDLK_QUOTE,         9,  8 },  // ;    (')
	{ SDLK_EQUALS,       11,  8 },  // [ ]  (=)
	{ SDLK_COMMA,         8, 16 },  // ,    (,)
	{ SDLK_MINUS,        10,  2 },  // _    (-)
	{ SDLK_PERIOD,        9, 16 },  // .    (.)
	{ SDLK_SLASH,        10, 16 },  // /    (/)
	{ SDLK_BACKQUOTE,    12,  1 },  // WRK  (`)
	{ SDLK_LEFTBRACKET,  10,  4 },  // @    ([)
	{ SDLK_BACKSLASH,    11,  2 },  // { }  (\)
	{ SDLK_RIGHTBRACKET, 11,  4 },  // \    (])
	{ SDLK_SEMICOLON,    10,  8 },  // :    (;)
	{ 0, 0, 0 }
};

// NUMERIC KEYPAD MAP
// vkey, column, rowmask
KEYMAP SystemPIO::KeyMapNumpad[] = {
	{ SDLK_KP0,         9,   2 },  // Num 0
	{ SDLK_KP1,         0,   2 },  // Num 1
	{ SDLK_KP2,         1,   2 },  // Num 2
	{ SDLK_KP3,         2,   2 },  // Num 3
	{ SDLK_KP4,         3,   2 },  // Num 4
	{ SDLK_KP5,         4,   2 },  // Num 5
	{ SDLK_KP6,         5,   2 },  // Num 6
	{ SDLK_KP7,         6,   2 },  // Num 7
	{ SDLK_KP8,         7,   2 },  // Num 8
	{ SDLK_KP9,         8,   2 },  // Num 9
	{ SDLK_KP_PERIOD,   9,  16 },  // Num .
	{ SDLK_KP_DIVIDE,   10, 16 },  // Num /
	{ SDLK_KP_MULTIPLY, 10, 40 },  // Num *
	{ SDLK_KP_PLUS,     9,  40 },  // Num +
	{ SDLK_KP_MINUS,    9,  34 },  // Num -
	{ 0, 0, 0 }
};

// MATO SPECIAL KEYBOARD MAP
KEYMAP SystemPIO::KeyMapMato[] = {
	{ SDLK_SPACE,        6,  4 },  // SPACE
	{ SDLK_LEFT,         6, 16 },  // Left
	{ SDLK_UP,           6,  2 },  // Up
	{ SDLK_RIGHT,        6, 32 },  // Right
	{ SDLK_DOWN,         6,  1 },  // Down
	{ SDLK_0,            4,  2 },  // 0
	{ SDLK_1,            0,  1 },  // 1
	{ SDLK_2,            0,  2 },  // 2
	{ SDLK_3,            0,  4 },  // 3
	{ SDLK_4,            0,  8 },  // 4
	{ SDLK_5,            0, 16 },  // 5
	{ SDLK_6,            0, 32 },  // 6
	{ SDLK_7,            0, 64 },  // 7
	{ SDLK_8,            4,  8 },  // 8
	{ SDLK_9,            5,  8 },  // 9
	{ SDLK_a,            2,  1 },  // A
	{ SDLK_b,            3, 16 },  // B
	{ SDLK_c,            3,  4 },  // C
	{ SDLK_d,            2,  4 },  // D
	{ SDLK_e,            1,  4 },  // E
	{ SDLK_f,            2,  8 },  // F
	{ SDLK_g,            2, 16 },  // G
	{ SDLK_h,            2, 32 },  // H
	{ SDLK_i,            4, 64 },  // I
	{ SDLK_j,            2, 64 },  // J
	{ SDLK_k,            4, 32 },  // K
	{ SDLK_l,            5, 32 },  // L
	{ SDLK_m,            3, 64 },  // M
	{ SDLK_n,            3, 32 },  // N
	{ SDLK_o,            5, 64 },  // O
	{ SDLK_p,            5,  1 },  // P
	{ SDLK_q,            1,  1 },  // Q
	{ SDLK_r,            1,  8 },  // R
	{ SDLK_s,            2,  2 },  // S
	{ SDLK_t,            1, 16 },  // T
	{ SDLK_u,            1, 64 },  // U
	{ SDLK_v,            3,  8 },  // V
	{ SDLK_w,            1,  2 },  // W
	{ SDLK_x,            3,  2 },  // X
	{ SDLK_y,            1, 32 },  // Z
	{ SDLK_z,            3,  1 },  // Y
	{ SDLK_QUOTE,        5,  4 },  // ;  (')
	{ SDLK_COMMA,        4, 16 },  // ,  (,)
	{ SDLK_MINUS,        4,  1 },  // -  (-)
	{ SDLK_PERIOD,       5, 16 },  // .  (.)
	{ SDLK_SLASH,        5,  2 },  // /  (/)
	{ SDLK_LEFTBRACKET,  6,  8 },  // @  ([)
	{ SDLK_RIGHTBRACKET, 4,  4 },  // \  (])
	{ SDLK_SEMICOLON,    6, 64 },  // :  (;)
	{ 0, 0, 0 }
};

KEYMAP SystemPIO::KeyMapMatoExt[] = {
	{ SDLK_KP0,         4,   2 },  // Num 0
	{ SDLK_KP1,         0,   1 },  // Num 1
	{ SDLK_KP2,         0,   2 },  // Num 2
	{ SDLK_KP3,         0,   4 },  // Num 3
	{ SDLK_KP4,         0,   8 },  // Num 4
	{ SDLK_KP5,         0,  16 },  // Num 5
	{ SDLK_KP6,         0,  32 },  // Num 6
	{ SDLK_KP7,         0,  64 },  // Num 7
	{ SDLK_KP8,         4,   8 },  // Num 8
	{ SDLK_KP9,         5,   8 },  // Num 9
	{ SDLK_KP_PERIOD,   5,  16 },  // Num .
	{ SDLK_KP_DIVIDE,   5,   2 },  // Num /
	{ SDLK_KP_MINUS,    4,   1 },  // Num -
	{ SDLK_KP_MULTIPLY, 6, 192 },  // Num *
	{ SDLK_KP_PLUS,     5, 132 },  // Num +

	// CNT bit added - idx=15
	{ SDLK_TAB,         3,   4 },  // C-D    (Tab)
	{ SDLK_PAGEUP,      3,   2 },  // RCL    (Page Up)
	{ SDLK_PAGEDOWN,    3,  16 },  // CLL    (Page Down)
	{ SDLK_END,         6,  32 },  // ENDL   (End)
	{ SDLK_HOME,        6,   2 },  // BGNL   (Home)
	{ SDLK_INSERT,      3,  64 },  // INST   (Insert)
	{ SDLK_DELETE,      3,  32 },  // DELT   (Delete)
	{ SDLK_BACKQUOTE,   3,   1 },  // WRK

	{ SDLK_F1,          0,   1 },  // K0     (F1)
	{ SDLK_F2,          0,   2 },  // K1     (F2)
	{ SDLK_F3,          0,   4 },  // K2     (F3)
	{ SDLK_F4,          0,   8 },  // K3     (F4)
	{ SDLK_F5,          0,  16 },  // K4     (F5)
	{ SDLK_F6,          0,  32 },  // K5     (F6)
	{ SDLK_F7,          0,  64 },  // K6     (F7)
	{ SDLK_F8,          4,   8 },  // K7     (F8)
	{ SDLK_F9,          5,   8 },  // K8     (F9)
	{ SDLK_F10,         4,   2 },  // K9     (F10)
	{ SDLK_F11,         4,   1 },  // K10    (F11)
	{ SDLK_F12,         4,   4 },  // K11    (F12)
	{ 0, 0, 0 },

	// shifted - idx=36
	{ SDLK_INSERT,      5,   2 },  // PTL    (Shift + Insert)
	{ SDLK_PAGEUP,      4,  16 },  // MON    (Shift + Page Up)
	{ SDLK_PAGEDOWN,    5,  16 },  // DELL   (Shift + Page Down)
	{ SDLK_END,         6,  16 },  // BEEP   (Shift + End)
	{ SDLK_HOME,        3,   8 },  // CLS    (Shift + Home)
	{ 0, 0, 0 }
};
//---------------------------------------------------------------------------
/**
 * Metoda na zaklade stalcenych klaves modifikuje mapu reprezentujucu
 * klavesnicovu maticu. Je volana pravidelne prostrednictvom Refresh timera.
 * Podla modelu sa prezera prislusna mapa klaves.
 */
void SystemPIO::ScanKeyboard(BYTE *KeyBuffer)
{
	static BYTE keyExchZY, keyExchYZ;

	if (KeyBuffer == NULL)
		return;

	// pressing of ALT/META resets whole matrix because it's used for hotkeys
	if (KeyBuffer[SDLK_MENU] || KeyBuffer[SDLK_LALT] || KeyBuffer[SDLK_RALT]
	 || KeyBuffer[SDLK_LMETA] || KeyBuffer[SDLK_RMETA]
	 || KeyBuffer[SDLK_LSUPER] || KeyBuffer[SDLK_RSUPER]) {
		for (int ii = 0; ii < (int)sizeof(KeyColumns); ii++)
			KeyColumns[ii] = 0;
		ShiftStopCtrl = 0;
		return;
	}

	if (exchZY == true) {
		keyExchZY = KeyBuffer[SDLK_z];
		KeyBuffer[SDLK_z] = keyExchYZ = KeyBuffer[SDLK_y];
		KeyBuffer[SDLK_y] = keyExchZY;
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
		if (KeyBuffer[SDLK_BACKSPACE] && !KeyBuffer[SDLK_LEFT])
			KeyColumns[6] |= 16;

		// EOL
		if (KeyBuffer[SDLK_RETURN] || KeyBuffer[SDLK_KP_ENTER])
			KeyColumns[7] |= 0x80;
		else
			KeyColumns[7] &= ~0x80;

		// STOP
		if (KeyBuffer[SDLK_ESCAPE])
			ShiftStopCtrl |= 0x10;
		else
			ShiftStopCtrl &= ~0x10;

		// SHF
		if (KeyBuffer[SDLK_LSHIFT] || KeyBuffer[SDLK_RSHIFT])
			ShiftStopCtrl |= 0x20;
		else
			ShiftStopCtrl &= ~0x20;

		// CNT
		if (KeyBuffer[SDLK_LCTRL] || KeyBuffer[SDLK_RCTRL])
			ShiftStopCtrl |= 0x40;
		else
			ShiftStopCtrl &= ~0x40;

		// riadiace klavesy a numericka klavesnica
		if (extMato == true) {
			if (KeyBuffer[SDLK_LSHIFT] || KeyBuffer[SDLK_RSHIFT])
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
		if (KeyBuffer[SDLK_BACKSPACE] && !KeyBuffer[SDLK_LEFT])
			KeyColumns[12] |= 4;

		// SHIFT
		if (KeyBuffer[SDLK_LSHIFT] || KeyBuffer[SDLK_RSHIFT])
			ShiftStopCtrl |= 0x20;
		else
			ShiftStopCtrl &= ~0x20;

		// STOP
		if (KeyBuffer[SDLK_ESCAPE] || KeyBuffer[SDLK_LCTRL] || KeyBuffer[SDLK_RCTRL])
			ShiftStopCtrl |= 0x40;
		else
			ShiftStopCtrl &= ~0x40;

		// numericka klavesnica
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
		KeyBuffer[SDLK_z] = keyExchZY;
		KeyBuffer[SDLK_y] = keyExchYZ;
	}

}
//---------------------------------------------------------------------------
/**
 * Metoda je pouzita ako notifikacna funkcia pri citani z portu B.
 * Metoda pripravi stav portu B, co je prave adresovany stlpec klavesnicovej
 * matice.
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
 * Metoda je pouzita ako notifikacna funkcia pri citani z portu C pre pocitac
 * MATO.
 * Metoda pripravi stav portu C, co je stav klaves Shift, Ctrl a Stop.
 */
void SystemPIO::ReadKeyboardC()
{
	PeripheralWriteByte(PP_PortC, (BYTE) ((~ShiftStopCtrl & 0x70) | 0x01));
}
//---------------------------------------------------------------------------
/**
 * Metoda je pouzita ako notifikacna funkcia pri zapise na horne bity portu C.
 * Metoda obsluhuje strankovanie pamate.
 */
void SystemPIO::WritePaging()
{
	BYTE pg = PeripheralReadByte(PP_PortC);

	if (model == CM_C2717) {
		Tag = ((pg & 32) + 1);              // rezim 48/64 znakov na riadok
		memory->Page = ((pg & 64) ? 0 : 1); // AllRAM
		memory->C2717Remapped = (pg & 128); // preadresovanie od 0xC000
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
 * Metoda je pouzita ako notifikacna funkcia pri zapise na dolne bity portu C.
 * Metoda obsluhuje pipak a LED.
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
 * Metoda je posluchacom tickov procesora.
 * Vytvara 1kHz a 4kHz signaly (tony), ktore su originale brane z rozkladu
 * obrazu.
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
