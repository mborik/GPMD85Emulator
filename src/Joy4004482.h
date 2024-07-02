/*	Joy4004482.h: Class for emulation of Joystick 4004/482 ZO Svazarmu
	Copyright (c) 2009-2021 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2024 Martin Borik <mborik@users.sourceforge.net>

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
#ifndef Joy4004482H
#define Joy4004482H
//---------------------------------------------------------------------------
#include "globals.h"
#include "Settings.h"
#include "IifGPIO.h"
//---------------------------------------------------------------------------
#define JOY_MASK_DOWN    (~1)
#define JOY_MASK_UP      (~2)
#define JOY_MASK_RIGHT   (~4)
#define JOY_MASK_LEFT    (~8)
#define JOY_MASK_FIRE    (~16)

#define JOY_AXES_MIN_VAL -32768
#define JOY_AXES_MAX_VAL 32767
#define JOY_AXES_RANGE   (JOY_AXES_MAX_VAL - JOY_AXES_MIN_VAL)
//---------------------------------------------------------------------------
class Joy4004482 : public sigslot::has_slots<>
{
public:
	Joy4004482(IifGPIO *pio, TSettings::SetJoystick* settings);
	virtual ~Joy4004482();

	void ScanJoy(BYTE *keyBuffer);

protected:
	void ReadJoy0();
	void ReadJoy1();

private:
	typedef struct {
		TSettings::SetJoystickGPIO *map;
		SDL_GameController * controller;
		BYTE value;
	} JOY;

	IifGPIO *pio;
	JOY joy[2];
	uint8_t joyCnt;
	bool sameDev;

	void ScanJoyKey(JOY *joy, BYTE *keyBuffer);
	void ScanJoyButtons(JOY *joy, bool onlyFire);
	void ScanJoyAxis(JOY *joy);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
