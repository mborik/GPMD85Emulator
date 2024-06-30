/*	Mouse602.cpp: Class for Mouse 602 interface emulation (Vit Libovicky)
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
#include "Mouse602.h"
//---------------------------------------------------------------------------
Mouse602::Mouse602(int size, int offsetX, int offsetY)
{
	SetMouseArea(size, offsetX, offsetY);
	hideCursor = true;

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
