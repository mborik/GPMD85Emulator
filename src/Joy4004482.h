/*	Joy4004482.h: Class for emulation of Joystick 4004/482 ZO Svazarmu
	Copyright (c) 2009-2021 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2024 Martin Borik <martin@borik.net>

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
class Joy4004482
{
public:
	Joy4004482(IifGPIO *pio, TSettings::SetJoystick* settings);
	virtual ~Joy4004482();

	void Connect();
	int  GetControllers(SDL_GameController ***controllers, bool refresh = false);
	void ScanJoy(BYTE *keyBuffer);

protected:
	void ReadJoy0();
	void ReadJoy1();

private:
	typedef struct {
		TSettings::SetJoystickGPIO *map;
		SDL_GameController *controller;
		bool initialized;
		BYTE value;
	} JOY;

	IifGPIO *pio;
	TSettings::SetJoystick *settings;

	JOY joy[2];
	uint8_t joyCnt;
	bool sameDev;

	void ScanJoyKey(JOY *joy, BYTE *keyBuffer);
	void ScanJoyButtons(JOY *joy, bool onlyFire);
	void ScanJoyAxis(JOY *joy);

	SDL_GameController **deviceList = NULL;
	unsigned deviceCount = 0;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
