/*	StatusBar.cpp: Part of GUI rendering class: StatusBar rendering
	Copyright (c) 2011-2018 Martin Borik <mborik@users.sourceforge.net>

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
//-----------------------------------------------------------------------------
#include "UserInterface.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
void UserInterface::InitStatusBarTexture()
{
	statusTexture = SDL_CreateTexture(gdc.renderer, SDL_PIXELFORMAT_DEFAULT,
			SDL_TEXTUREACCESS_STREAMING, statusRect->w, statusRect->h);
	if (!statusTexture)
		error("GUI", "Unable to create statusTexture\n%s", SDL_GetError());

	void *pixels;
	int pitch;

	SDL_LockTexture(statusTexture, NULL, &pixels, &pitch);
	memset(pixels, 0, pitch * STATUSBAR_HEIGHT);
	SDL_UnlockTexture(statusTexture);
}
//-----------------------------------------------------------------------------
void UserInterface::RedrawStatusBar()
{
	SDL_Rect *r = new SDL_Rect, *s = new SDL_Rect;

	r->x = statusRect->w - (3 * STATUSBAR_SPACING);
	r->y = ((STATUSBAR_HEIGHT - STATUSBAR_ICON) / 2);
	r->w = r->h = s->w = s->h = STATUSBAR_ICON;

	GUI_SURFACE *statusSurface = LockSurface(statusTexture);

//	control LEDs on right side...
	s->y = 0;
	s->x = (ledState & 1) ? STATUSBAR_ICON : 0;
	BlitToSurface(icons, s, statusSurface, r);

	r->x += STATUSBAR_SPACING;
	s->x = (ledState & 2) ? (2 * STATUSBAR_ICON) : 0;
	BlitToSurface(icons, s, statusSurface, r);

	r->x += STATUSBAR_SPACING;
	s->x = (ledState & 4) ? (3 * STATUSBAR_ICON) : 0;
	BlitToSurface(icons, s, statusSurface, r);

//	tape/disk icon...
	r->x -= (4 * STATUSBAR_SPACING);
	if (iconState) {
		s->x = (iconState * STATUSBAR_ICON) + (3 * STATUSBAR_ICON);
		BlitToSurface(icons, s, statusSurface, r);
	}
	else
		DrawRectangle(statusSurface, r->x, r->y, r->w, r->h, 0);

	delete s;

	int tapProgressWidth = r->x - STATUSBAR_SPACING;
	static char status[24] = "";
	static BYTE pauseBlinker = 0;

//	status text, cpu meter and blinking pause...
	r->x = 0;
	r->y++;

	PrintText(statusSurface, r->x, r->y, 0, status);
	if (statusPercentage < 0) {
		sprintf(status, "PAUSED");
		PrintText(statusSurface, r->x, r->y,
				(pauseBlinker < 10) ? GUI_COLOR_STAT_PAUSE : 0, status);

		if (pauseBlinker++ >= 16)
			pauseBlinker = 0;
	}
	else if (statusPercentage > 0) {
		sprintf(status, "%sFPS:%d CPU:%d%%", computerModel, statusFPS, statusPercentage);
		PrintText(statusSurface, r->x, r->y, GUI_COLOR_STAT_TEXT, status);
	}

//	tape progress bar...
	r->x = (r->x + (20 * 6) + STATUSBAR_SPACING);
	r->y += 3;
	r->w = tapProgressWidth - r->x;

	TTapeBrowser::TProgressBar *progress = TapeBrowser->ProgressBar;
	DrawRectangle(statusSurface, r->x, r->y, r->w, 2, *progress->Active ? GUI_COLOR_STATTAP_BG : 0);
	if (*progress->Active) {
		DrawRectangle(statusSurface, r->x, r->y,
				((double) r->w / progress->Max) * progress->Position, 2, GUI_COLOR_STATTAP_FG);
	}

	delete r;

	UnlockSurface(statusTexture, statusSurface);
	SDL_RenderCopy(gdc.renderer, statusTexture, NULL, statusRect);
}
//-----------------------------------------------------------------------------
void UserInterface::SetLedState(int led)
{
	static int pullUpConstant = (TCYCLES_PER_FRAME / 16);
	static int pullUpYellow = 0;
	static int pullUpRed = 0;

	if (pullUpYellow > 0) {
		led |= LED_YELLOW;
		pullUpYellow--;
	}
	else
		pullUpYellow = (led & LED_YELLOW) ? pullUpConstant : 0;

	if (pullUpRed > 0) {
		led |= LED_RED;
		pullUpRed--;
	}
	else
		pullUpRed = (led & LED_RED) ? pullUpConstant : 0;

	ledState = led;
}
//---------------------------------------------------------------------------
void UserInterface::SetIconState(int icon)
{
	static int pullUpIcon = 0;

	if (iconState != icon) {
		if (iconState > 0 && iconState < 9 && icon == 0) {
			if (pullUpIcon == 0)
				pullUpIcon = 50;
			else
				pullUpIcon--;
		}

		if (pullUpIcon == 0 || icon > 0)
			iconState = icon;
	}
}
//---------------------------------------------------------------------------
void UserInterface::SetComputerModel(TComputerModel model)
{
	const char *modelName = NULL;

	switch (model) {
		case CM_V1:
			modelName = "M1"; break;
		case CM_V2:
			modelName = "M2"; break;
		case CM_V2A:
			modelName = "M2A"; break;
		case CM_V3:
			modelName = "M3"; break;
		case CM_ALFA:
			modelName = "\2141"; break;
		case CM_ALFA2:
			modelName = "\2142"; break;
		case CM_C2717:
			modelName = "C\215\216\217"; break;
		case CM_MATO:
			modelName = "Ma\213o"; break;
		default:
			break;
	}

	if (modelName)
		sprintf(computerModel, "%s: ", modelName);
	else
		computerModel[0] = '\0';
}
//---------------------------------------------------------------------------
