/*	Mouse602.h: Class for Mouse 602 interface emulation (Vit Libovicky)
	Copyright (c) 2008-2017 Roman Borik <pmd85emu@gmail.com>
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
class Mouse602 : public PeripheralDevice
{
public:
	Mouse602(int size, int offsetX = 0, int offsetY = 0);

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
