/*	PeripheralDevice.h: Abstract class PeripheralDevice
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
#ifndef PeripheralDeviceH
#define PeripheralDeviceH
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
class PeripheralDevice {
public:
	int Tag;

	// Pure virtual methods:
	// It was implemented by interupt control class and called by processor.
	virtual void writeToDevice(BYTE port, BYTE value, int ticks) = 0;
	virtual BYTE readFromDevice(BYTE port, int ticks) = 0;
	virtual void resetDevice(int ticks) { return; };
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
