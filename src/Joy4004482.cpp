/*	Joy4004482.cpp: Class for emulation of Joystick 4004/482 ZO Svazarmu
	Copyright (c) 2008-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011 Martin Borik <mborik@users.sourceforge.net>

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
#include "Joy4004482.h"
//---------------------------------------------------------------------------
Joy4004482::Joy4004482(IifGPIO *pio, TSettings::SetJoystick* settings) :
	pio(pio),
	settings(settings),
	joy{{ NULL, NULL, false, 0 }, { NULL, NULL, false, 0 }},
	joyCnt(0),
	sameDev(false)
{
}
//---------------------------------------------------------------------------
Joy4004482::~Joy4004482()
{
	pio->OnCpuReadA.disconnect(this);
	pio->OnCpuReadB.disconnect(this);

	if (deviceList) {
		for (int ii = 0; ii < deviceCount; ii++) {
			if (deviceList[ii])
				SDL_GameControllerClose(deviceList[ii]);
		}
		free(deviceList);
	}
}
//---------------------------------------------------------------------------
void Joy4004482::Connect()
{
	if (joyCnt > 0) {
		pio->OnCpuReadA.disconnect(this);
		pio->OnCpuReadB.disconnect(this);
	}

	joyCnt = 0;
	if (settings->GPIO0->connected && settings->GPIO1->connected) {
		pio->OnCpuReadA.connect(this, &Joy4004482::ReadJoy0);
		pio->OnCpuReadB.connect(this, &Joy4004482::ReadJoy1);

		joy[0].map = settings->GPIO0;
		joy[1].map = settings->GPIO1;
		joyCnt = 2;

		if (strcasecmp(settings->GPIO0->guid, settings->GPIO1->guid) == 0)
			sameDev = true;
	}
	else if (settings->GPIO0->connected) {
		pio->OnCpuReadA.connect(this, &Joy4004482::ReadJoy0);
		joy[0].map = settings->GPIO0;
		joyCnt = 1;
	}
	else if (settings->GPIO1->connected) {
		pio->OnCpuReadB.connect(this, &Joy4004482::ReadJoy1);
		joy[0].map = settings->GPIO1;
		joyCnt = 1;
	}

	debug("Joystick", "Connected %d joystick(s)%s", joyCnt,
		sameDev ? " on same device" : "");
}
//---------------------------------------------------------------------------
int Joy4004482::GetControllers(SDL_GameController ***controllers, bool refresh)
{
	if (refresh || deviceList == NULL) {
		if (deviceList) {
			for (int ii = 0; ii < deviceCount; ii++) {
				if (deviceList[ii])
					SDL_GameControllerClose(deviceList[ii]);
			}
			free(deviceList);

			for (int ii = 0; ii < joyCnt; ii++) {
				joy[ii].initialized = false;
				if (joy[ii].controller)
					joy[ii].controller = NULL;
			}
		}

		deviceCount = 0;
		deviceList = NULL;
		for (int ii = 0; ii < SDL_NumJoysticks(); ii++) {
			if (SDL_IsGameController(ii)) {
				SDL_GameController *controller = SDL_GameControllerOpen(ii);
				deviceList = (SDL_GameController **) realloc(deviceList, (deviceCount + 1) * sizeof(SDL_GameController *));
				deviceList[deviceCount++] = controller;

				debug("Joystick", "Detected controller %d: %s [%s]", deviceCount,
					SDL_GameControllerName(controller), SDL_GameControllerGetSerial(controller));
			}
		}
	}

	if (controllers)
		*controllers = deviceList;
	return deviceCount;
}
//---------------------------------------------------------------------------
void Joy4004482::ReadJoy0()
{
	if (pio) {
		BYTE value = 0xFF;

		if (pio->ReadByte(PP_PortC) & 0x10)
			value = joy[0].value;

		pio->WriteByte(PP_PortA, value);
	}
}
//---------------------------------------------------------------------------
void Joy4004482::ReadJoy1()
{
	if (pio) {
		BYTE value = 0xFF;

		if (pio->ReadByte(PP_PortC) & 0x01)
			value = joy[joyCnt - 1].value;

		pio->WriteByte(PP_PortB, value);
	}
}
//---------------------------------------------------------------------------
void Joy4004482::ScanJoy(BYTE *KeyBuffer)
{
	if (KeyBuffer == NULL)
		return;

	for (int ii = 0; ii < joyCnt; ii++) {
		if (!joy[ii].map->connected || joy[ii].map->type == JT_NONE) {
			joy[ii].value = 0xFF;
			continue;
		}
		if (joy[ii].map->type == JT_KEYS) {
			ScanJoyKey(&joy[ii], KeyBuffer);
			continue;
		}

		if (ii > 0 && sameDev)
			continue;
		else if (!(joy[ii].controller && joy[ii].initialized)) {
			SDL_GameController **controllers;
			int devCount = GetControllers(&controllers, false);

			if (!devCount)
				continue;

			if (joy[ii].map->guid && joy[ii].map->guid[0] != '\0') {
				for (int jj = 0; jj < devCount; jj++) {
					if (strcasecmp(joy[ii].map->guid, SDL_GameControllerGetSerial(controllers[jj])) == 0) {
						joy[ii].controller = controllers[jj];
						break;
					}
				}
			}
			if (!joy[ii].controller) {
				joy[ii].controller = controllers[0];
				const char *guid = SDL_GameControllerGetSerial(joy[ii].controller);
				joy[ii].map->guid = new char[strlen(guid) + 1];
				strcpy(joy[ii].map->guid, guid);
			}

			joy[ii].initialized = (SDL_GameControllerEventState(SDL_ENABLE) == SDL_ENABLE);
		}

		if (joy[ii].map->type == JT_POV || joy[ii].map->type == JT_BUTTONS) {
			ScanJoyButtons(&joy[ii], false);
			continue;
		}

		ScanJoyAxis(&joy[ii]);
		ScanJoyButtons(&joy[ii], true);
	}
}
//---------------------------------------------------------------------------
void Joy4004482::ScanJoyKey(JOY *joy, BYTE *keyBuffer)
{
	BYTE value = 0xFF;

	if (keyBuffer[joy->map->ctrlUp])
		value &= JOY_MASK_UP;
	if (keyBuffer[joy->map->ctrlDown])
		value &= JOY_MASK_DOWN;
	if (keyBuffer[joy->map->ctrlLeft])
		value &= JOY_MASK_LEFT;
	if (keyBuffer[joy->map->ctrlRight])
		value &= JOY_MASK_RIGHT;
	if (keyBuffer[joy->map->ctrlFire])
		value &= JOY_MASK_FIRE;

	joy->value = value;
}
//---------------------------------------------------------------------------
void Joy4004482::ScanJoyButtons(JOY *joy, bool onlyFire)
{
	BYTE value;

	static const SDL_GameControllerButton mapping[2][4] = {
		{ SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT },
		{ SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_B }
	};

	if (onlyFire)
		value = (BYTE) (joy->value | ~JOY_MASK_FIRE);
	else {
		value = 0xFF;
		const SDL_GameControllerButton *dirmap = mapping[joy->map->type == JT_BUTTONS ? 1 : 0];

		if (SDL_GameControllerGetButton(joy->controller, dirmap[0]))
			value &= JOY_MASK_UP;
		if (SDL_GameControllerGetButton(joy->controller, dirmap[1]))
			value &= JOY_MASK_DOWN;
		if (SDL_GameControllerGetButton(joy->controller, dirmap[2]))
			value &= JOY_MASK_LEFT;
		if (SDL_GameControllerGetButton(joy->controller, dirmap[3]))
			value &= JOY_MASK_RIGHT;
	}

	if (joy->map->type == JT_BUTTONS) {
		if (SDL_GameControllerGetButton(joy->controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
			|| SDL_GameControllerGetButton(joy->controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
			|| SDL_GameControllerGetButton(joy->controller, SDL_CONTROLLER_BUTTON_LEFTSTICK)
			|| SDL_GameControllerGetButton(joy->controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK))
				value &= JOY_MASK_FIRE;
	}
	else {
		if (SDL_GameControllerGetButton(joy->controller, SDL_CONTROLLER_BUTTON_A))
			value &= JOY_MASK_FIRE;
	}

	joy->value = value;
}
//---------------------------------------------------------------------------
void Joy4004482::ScanJoyAxis(JOY *joy)
{
	BYTE value = 0xFF;

	int x = SDL_GameControllerGetAxis(joy->controller,
		(SDL_GameControllerAxis) ((int) SDL_CONTROLLER_AXIS_LEFTX + (joy->map->axis * 2)));
	int y = -SDL_GameControllerGetAxis(joy->controller,
		(SDL_GameControllerAxis) ((int) SDL_CONTROLLER_AXIS_LEFTY + (joy->map->axis * 2)));

	double p = sqrt((double) (x * x + y * y));
	int q = -1;
	int r = (int) (p / (JOY_AXES_RANGE / 2.0f) * 100.0f);
	if (r > joy->map->sensitivity) {
		p = acos(y / p);
		if (x < 0)
			p = (2 * M_PI) - p;
		q = (int) ((p / M_PI_4) + 0.5f);
	}

	switch (q) {
		case 0: // 0 - Up
			value &= JOY_MASK_UP;
			break;
		case 1: // 45 - Up-Right
			value &= JOY_MASK_UP;
			value &= JOY_MASK_RIGHT;
			break;
		case 2: // 90 - Right
			value &= JOY_MASK_RIGHT;
			break;
		case 3: // 135 - Down-Right
			value &= JOY_MASK_DOWN;
			value &= JOY_MASK_RIGHT;
			break;
		case 4: // 180 - Down
			value &= JOY_MASK_DOWN;
			break;
		case 5: // 225 - Down-Left
			value &= JOY_MASK_DOWN;
			value &= JOY_MASK_LEFT;
			break;
		case 6: // 270 - Left
			value &= JOY_MASK_LEFT;
			break;
		case 7: // 315 - Up-Left
			value &= JOY_MASK_UP;
			value &= JOY_MASK_LEFT;
			break;
		default:
			value = 0xFF;
			break;
	}

	joy->value = value;
}
//---------------------------------------------------------------------------
