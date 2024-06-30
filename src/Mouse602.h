/*	Mouse602.h: Class for Mouse 602 interface emulation (Vit Libovicky)
	Copyright (c) 2008-2017 Roman Borik <pmd85emu@gmail.com>
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
#ifndef Mouse602H
#define Mouse602H
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include <queue>
//---------------------------------------------------------------------------
#define MOUSE_MASK      0xFF
#define MOUSE_ADR       0x8C

#define M602_FREQ       1200
#define M602_INTERVAL   (CPU_FREQ / M602_FREQ)

#define M602_RBTN_MASK  128
#define M602_LBTN_MASK  64
#define M602_XAXIS_MASK 12
#define M602_YAXIS_MASK 3
//---------------------------------------------------------------------------
class Mouse602 : public PeripheralDevice, public sigslot::has_slots<>
{
public:
	Mouse602(int size, int offsetX, int offsetY);

	virtual void WriteToDevice(BYTE port, BYTE val, int ticks);
	virtual BYTE ReadFromDevice(BYTE port, int ticks);

	void SetMouseArea(int size, int offsetX, int offsetY);
	void SetMouseState(int x, int y, int leftBtn, int rightBtn, int middleBtn);
	void MouseService(int ticks, int dur);

	inline void SetHideCursor(bool hide) { hideCursor = hide; }
	inline bool GetHideCursor() { return hideCursor; }

protected:
	bool hideCursor;
	int  screenSize;
	int  offsetX;
	int  offsetY;

private:
	BYTE posState;
	BYTE btnState;
	int  mouseTicks;
	int  oldX, oldY;

	std::queue<BYTE> posQueue;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
