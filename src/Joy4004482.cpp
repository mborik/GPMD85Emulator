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
Joy4004482::Joy4004482(IifGPIO *pio, TSettings::SetJoystick* settings)
{
	this->pio = pio;

	sameDev = false;
	joyCnt = 1;
	if (settings->GPIO0->connected && settings->GPIO1->connected) {
		joyCnt = 2;

		pio->OnCpuReadA.connect(this, &Joy4004482::ReadJoy0);
		pio->OnCpuReadB.connect(this, &Joy4004482::ReadJoy1);

		joy[0].map = settings->GPIO0;
		joy[1].map = settings->GPIO1;

		if (settings->GPIO0->guid == settings->GPIO1->guid && settings->GPIO0->guid >= 0)
			sameDev = true;
	}
	else if (settings->GPIO0->connected) {
		pio->OnCpuReadA.connect(this, &Joy4004482::ReadJoy0);
		joy[0].map = settings->GPIO0;
	}
	else if (settings->GPIO1->connected) {
		pio->OnCpuReadB.connect(this, &Joy4004482::ReadJoy1);
		joy[0].map = settings->GPIO1;
	}
}
//---------------------------------------------------------------------------
Joy4004482::~Joy4004482()
{
	pio->OnCpuReadA.disconnect(this);
	pio->OnCpuReadB.disconnect(this);
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

	// TODO
}
//---------------------------------------------------------------------------
// void ScanJoyKey(JOY *joy, BYTE *keyBuffer);
void Joy4004482::ScanJoyKey(JOY *joy, BYTE *keyBuffer)
{
	if (joy->map->connected && joy->map->type == JT_KEYS) {
		BYTE value = 0xFF;

		if (joy->map->ctrlDown)
			value &= JOY_MASK_DOWN;
		if (joy->map->ctrlUp)
			value &= JOY_MASK_UP;
		if (joy->map->ctrlRight)
			value &= JOY_MASK_RIGHT;
		if (joy->map->ctrlLeft)
			value &= JOY_MASK_LEFT;
		if (joy->map->ctrlFire)
			value &= JOY_MASK_FIRE;

		joy->value = value;
	}
}
//---------------------------------------------------------------------------
// void ScanRealJoy(JOY *joy, bool onlyFire);
void Joy4004482::ScanRealJoy(JOY *joy, bool onlyFire)
{
	// TODO
}
//---------------------------------------------------------------------------
