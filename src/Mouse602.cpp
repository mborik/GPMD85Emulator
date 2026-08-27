/*	Mouse602.cpp: Class for Mouse 602 interface emulation (Vit Libovicky)
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
#include "Mouse602.h"
//---------------------------------------------------------------------------
Mouse602::Mouse602(int size, int offsetX, int offsetY)
{
	SetMouseArea(size, offsetX, offsetY);

	posState = 0;
	btnState = 0;
	mouseTicks = 0;
}
//---------------------------------------------------------------------------
void Mouse602::SetMouseArea(int size, int offsetX, int offsetY)
{
	this->offsetX = offsetX;
	this->offsetY = offsetY;
	screenSize = size;
	oldX = oldY = -1;

	debug("Mouse602", "Mouse initialized [scale %d, offset %dx%d]", size, offsetX, offsetY);
}
//---------------------------------------------------------------------------
void Mouse602::WriteToDevice(BYTE port, BYTE val, int ticks)
{
	// not used
}
//---------------------------------------------------------------------------
BYTE Mouse602::ReadFromDevice(BYTE port, int ticks)
{
	return posState | btnState;
}
//---------------------------------------------------------------------------
void Mouse602::MouseService(int ticks, int dur)
{
	mouseTicks += dur;
	if (mouseTicks > M602_INTERVAL) {
		mouseTicks -= M602_INTERVAL;
		if (!posQueue.empty()) {
			posState = posQueue.front();
			posQueue.pop();
		}
		else
			posState = 0;
	}
}
//---------------------------------------------------------------------------
void Mouse602::SetMouseState(int x, int y, int leftBtn, int rightBtn, int middleBtn)
{
	int plus[4] = { 1, 3, 2, 0 };
	int minus[4] = { 2, 3, 1, 0 };
	int ix, iy, a, m, i;
	int *dx, *dy;
	bool xy;

	if (oldX < 0) {
		oldX = (x - offsetX) / screenSize;
		oldY = (y - offsetY) / screenSize;
		return;
	}

	a = (x - offsetX) / screenSize;
	if (a - oldX >= 0) {
		dx = plus;
		ix = a - oldX;
	}
	else {
		dx = minus;
		ix = oldX - a;
	}
	if (ix > 0)
		oldX = a;

	a = (y - offsetY) / screenSize;
	if (a - oldY >= 0) {
		dy = plus;
		iy = a - oldY;
	}
	else {
		dy = minus;
		iy = oldY - a;
	}
	if (iy > 0)
		oldY = a;

	while (ix > 0 || iy > 0) {
		m = 1;
		if (ix > 0 && iy > 0) {
			if (ix > iy) {
				m = ix / 2;
				xy = false;
			}
			else {
				m = iy / 2;
				xy = true;
			}
		}

		for (i = 0; i < 4; i++) {
			a = 0;
			if (ix > 0)
				a |= (dx[i] << 2);
			if (iy > 0)
				a |= dy[i];
			posQueue.push(a);
		}

		m--;
		if (ix > 0)
			ix--;
		if (iy > 0)
			iy--;

		while (m > 0) {
			for (i = 0; i < 4; i++) {
				a = 0;
				if (xy)
					a |= dy[i];
				else
					a |= (dx[i] << 2);
				posQueue.push(a);
			}
			m--;
			if (xy)
				iy--;
			else
				ix--;
		}
	}

	if (middleBtn)
		leftBtn = rightBtn = middleBtn;
	if (leftBtn < 0)
		btnState &= ~M602_LBTN_MASK;
	else if (leftBtn > 0)
		btnState |= M602_LBTN_MASK;
	if (rightBtn < 0)
		btnState &= ~M602_RBTN_MASK;
	else if (rightBtn > 0)
		btnState |= M602_RBTN_MASK;
}
//---------------------------------------------------------------------------
