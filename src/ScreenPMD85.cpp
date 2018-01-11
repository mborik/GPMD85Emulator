/*	ScreenPMD85.cpp: Core of graphical output and screen generation
	Copyright (c) 2010-2018 Martin Borik <mborik@users.sourceforge.net>

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
#include "CommonUtils.h"
#include "ScreenPMD85.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
ScreenPMD85::ScreenPMD85(TDisplayMode dispMode, int border)
{
	screenTexture = NULL;
	screenRect = NULL;
	palette = NULL;

	displayModeChanging = true;
	blinkState = false;
	blinkingEnabled = false;
	lcdMode = false;
	borderSize = (border * BORDER_MULTIPLIER);

	InitPalette();
	InitScreenSize(dispMode, false);

	PrepareVideoMode();
	SetColorProfile(CP_STANDARD);
	SetHalfPassMode(HP_OFF);

	displayModeChanging = false;
}
//-----------------------------------------------------------------------------
ScreenPMD85::~ScreenPMD85()
{
	displayModeChanging = true;

	SDL_RenderClear(gdc.renderer);
	SDL_RenderPresent(gdc.renderer);

	ReleaseVideoMode();
}
//-----------------------------------------------------------------------------
void ScreenPMD85::SetDisplayMode(TDisplayMode dm, int border)
{
	border *= BORDER_MULTIPLIER;
	if (borderSize != border) {
		if (dispMode != DM_FULLSCREEN) {
			dispMode = (TDisplayMode) -1;
			borderSize = border;
		}
	}

	if (dispMode == dm)
		return;

	displayModeChanging = true;
	SDL_Delay(WEAK_REFRESH_TIME);

	ReleaseVideoMode();
	InitScreenSize(dm, width384mode);
	PrepareVideoMode();

	displayModeChanging = false;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetWidth384(bool mode384)
{
	if (width384mode == mode384)
		return;

	displayModeChanging = true;

	SDL_Delay(WEAK_REFRESH_TIME);

	ReleaseVideoMode();
	InitScreenSize(dispMode, mode384);
	PrepareVideoMode();

	displayModeChanging = false;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetHalfPassMode(THalfPassMode hp)
{
	halfPass = hp;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetLcdMode(bool state)
{
	lcdMode = state;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetColorProfile(TColorProfile cp)
{
	colorProfile = cp;
	switch (cp) {
		case CP_MONO:
			pAttr[0] = WHITE;
			pAttr[1] = WHITE;
			pAttr[2] = WHITE;
			pAttr[3] = WHITE;
			blinkingEnabled = false;
			break;

		case CP_STANDARD:
			pAttr[0] = WHITE;
			pAttr[1] = SILVER;
			pAttr[2] = WHITE;
			pAttr[3] = SILVER;
			blinkingEnabled = true;
			break;

		case CP_COLOR:
			pAttr[0] = cAttr[0];
			pAttr[1] = cAttr[1];
			pAttr[2] = cAttr[2];
			pAttr[3] = cAttr[3];
			blinkingEnabled = false;
			break;

		case CP_COLORACE:
			pAttr[0] = BLACK;
			pAttr[1] = RED;
			pAttr[2] = BLUE;
			pAttr[3] = FUCHSIA;
			pAttr[4] = LIME;
			pAttr[5] = YELLOW;
			pAttr[6] = AQUA;
			pAttr[7] = WHITE;
			blinkingEnabled = false;
			break;
	}
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetColorAttr(int idx, TColor attr)
{
	if (idx >= 0 && idx <= 3) {
		cAttr[idx] = attr;
		if (colorProfile == CP_COLOR) {
			pAttr[idx] = attr;
			pAttr[idx + 4] = attr;
		}
	}
}
//---------------------------------------------------------------------------
TColor ScreenPMD85::GetColorAttr(int idx)
{
	if (idx >= 0 && idx <= 3)
		return cAttr[idx];
	else
		return BLACK;
}
//---------------------------------------------------------------------------
void ScreenPMD85::RefreshDisplay()
{
	if (displayModeChanging)
		return;

	PrepareScreen();
	GUI->RedrawStatusBar();
	SDL_RenderCopy(gdc.renderer, screenTexture, NULL, screenRect);

	if (GUI->InMenu()) {
		SDL_RenderCopy(gdc.renderer, GUI->defaultTexture, NULL, screenRect);
	}

	SDL_RenderPresent(gdc.renderer);
}
//---------------------------------------------------------------------------
void ScreenPMD85::FillBuffer(BYTE *videoRam)
{
	if (displayModeChanging || videoRam == NULL)
		return;

	bool colorace = (colorProfile == CP_COLORACE);
	int i, w, h = bufferHeight, c2717 = (width384mode ? 0 : 0x40);
	BYTE a[4] = { pAttr[0], pAttr[1], pAttr[2], pAttr[3] }, b, c, d, e;

	if (blinkingEnabled && blinkState)
		a[2] = a[3] = 0;

	DWORD *ptr;
	BYTE *dst;
	SDL_LockTexture(screenTexture, NULL, (void **) &dst, &w);

	while (h--) {
		ptr = (DWORD *) dst;
		for (i = 0; i < 48; i++) {
			b = videoRam[i];
			d = (b & 0xC0) >> 6;

			if (colorace) {
				e = videoRam[i + ((h & 1) ? 64 : -64)];
				c = (e & 0xC0) >> 6;
				c = pAttr[d | c | ((d & c) ? 0 : 4)];
			}
			else if (c2717)
				c = a[d];
			else
				c = *a;

			for (d = 0x01; d != c2717; d <<= 1)
				*ptr++ = palette[((b & d) ? c : 0)];
		}

		dst += w;
		videoRam += 64;
	}

	SDL_UnlockTexture(screenTexture);
}
//---------------------------------------------------------------------------
void ScreenPMD85::InitScreenSize(TDisplayMode reqDispMode, bool reqWidth384)
{
	dispMode = reqDispMode;
	width384mode = reqWidth384;

	if (dispMode == DM_FULLSCREEN)
		reqDispMode = DM_QUADRUPLESIZE;

	while (true) {
		switch (reqDispMode) {
			default:
			case DM_NORMAL:
				screenWidth  = (reqWidth384) ? 384 : 288;
				screenHeight = 256;
				break;

			case DM_DOUBLESIZE:
				screenWidth  = (reqWidth384) ? 768 : 576;
				screenHeight = 512;
				break;

			case DM_TRIPLESIZE:
				screenWidth  = (reqWidth384) ? 1152 : 864;
				screenHeight = 768;
				break;

			case DM_QUADRUPLESIZE:
				screenWidth  = (reqWidth384) ? 1536 : 1152;
				screenHeight = 1024;
				break;
		}

		if (dispMode == DM_FULLSCREEN) {
			if (screenWidth > gdc.w || screenHeight + STATUSBAR_HEIGHT > gdc.h) {
				if (reqDispMode == DM_QUADRUPLESIZE)
					reqDispMode = DM_TRIPLESIZE;
				else if (reqDispMode == DM_TRIPLESIZE)
					reqDispMode = DM_DOUBLESIZE;
				else if (reqDispMode == DM_DOUBLESIZE)
					reqDispMode = DM_NORMAL;
				else {
					dispMode = DM_NORMAL;
					break;
				}

				continue;
			}
		}

		break;
	}

	bufferWidth  = (reqWidth384) ? 384 : 288;
	bufferHeight = 256;

	screenRect = new SDL_Rect;

	if (dispMode == DM_FULLSCREEN) {
		screenRect->w = screenWidth;
		screenRect->h = screenHeight;

		screenHeight += STATUSBAR_HEIGHT;
		screenRect->x = (gdc.w - screenWidth) / 2;
		screenRect->y = (gdc.h - screenHeight) / 2;

		screenWidth   = gdc.w;
		screenHeight  = gdc.h;
	}
	else {
		screenRect->x = borderSize;
		screenRect->y = borderSize;
		screenRect->w = screenWidth;
		screenRect->h = screenHeight;

		screenWidth  += (borderSize * 2);
		screenHeight += (borderSize * 2) + STATUSBAR_HEIGHT;
	}

	GUI->statusRect = new SDL_Rect(*screenRect);

	GUI->statusRect->x += STATUSBAR_SPACING;
	GUI->statusRect->y += screenRect->h + (borderSize - STATUSBAR_HEIGHT / 2);
	GUI->statusRect->w -= (2 * STATUSBAR_SPACING);
	GUI->statusRect->h  = STATUSBAR_HEIGHT;
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareVideoMode()
{
	if (dispMode == DM_FULLSCREEN) {
		debug("Screen", "Full-screen mode: %dx%d", gdc.w, gdc.h);
		SDL_SetWindowFullscreen(gdc.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else {
		debug("Screen", "Windowed mode: %dx%d", screenWidth, screenHeight);

		SDL_SetWindowFullscreen(gdc.window, 0);
		SDL_SetWindowSize(gdc.window, screenWidth, screenHeight);
	}

	if (SDL_RenderSetLogicalSize(gdc.renderer, screenWidth, screenHeight) != 0)
		error("Screen", "Unable to change screen resolution\n%s", SDL_GetError());

	screenTexture = SDL_CreateTexture(gdc.renderer, SDL_PIXELFORMAT_DEFAULT,
			SDL_TEXTUREACCESS_STREAMING, bufferWidth, bufferHeight);
	if (!screenTexture)
		error("Screen", "Unable to create screenTexture\n%s", SDL_GetError());

	GUI->InitStatusBarTexture();
	GUI->InitDefaultTexture(bufferWidth, bufferHeight);

	PrepareScreen(true);
	SDL_RenderPresent(gdc.renderer);
}
//-----------------------------------------------------------------------------
void ScreenPMD85::ReleaseVideoMode()
{
	if (GUI->statusRect) {
		delete GUI->statusRect;
		GUI->statusTexture = NULL;
	}
	if (GUI->statusTexture) {
		SDL_DestroyTexture(GUI->statusTexture);
		GUI->statusTexture = NULL;
	}
	if (GUI->defaultTexture) {
		SDL_DestroyTexture(GUI->defaultTexture);
		GUI->defaultTexture = NULL;
	}
	if (screenRect) {
		delete screenRect;
		screenTexture = NULL;
	}
	if (screenTexture) {
		SDL_DestroyTexture(screenTexture);
		screenTexture = NULL;
	}
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareScreen(bool clear)
{
	if (clear) {
		SDL_SetRenderDrawColor(gdc.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(gdc.renderer);
	}

	SDL_Rect *r = new SDL_Rect(*screenRect);
	SDL_SetRenderDrawColor(gdc.renderer, 16, 16, 16, SDL_ALPHA_OPAQUE);

	int i = 0;
	if (borderSize > 0) {
		i = GetMultiplier() * 2;
		r->x -= i;
		r->y -= i;
		r->w += i * 2;
		r->h += i * 2;

		SDL_RenderDrawRect(gdc.renderer, r);
	}
	else {
		int y = screenHeight - STATUSBAR_HEIGHT;
		SDL_RenderDrawLine(gdc.renderer, r->x, y, r->x + r->w, y);
	}

	delete r;
}
//-----------------------------------------------------------------------------
void ScreenPMD85::InitPalette()
{
	static DWORD stdpal[36] = {
		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // 0 - black (dimmed dot)
		DWORD_COLOR_ENTRY( 160,   0,   0 ),  // 1 - maroon
		DWORD_COLOR_ENTRY(   0, 160,   0 ),  // 2 - green
		DWORD_COLOR_ENTRY( 160, 160,   0 ),  // 3 - olive
		DWORD_COLOR_ENTRY(   0,   0, 160 ),  // 4 - navy
		DWORD_COLOR_ENTRY( 160,   0, 160 ),  // 5 - purple
		DWORD_COLOR_ENTRY(   0, 160, 160 ),  // 6 - teal
		DWORD_COLOR_ENTRY( 160, 160, 160 ),  // 7 - gray
		DWORD_COLOR_ENTRY( 191, 191, 191 ),  // 8 - silver (half bright)
		DWORD_COLOR_ENTRY( 255,  80,  80 ),  // 9 - red
		DWORD_COLOR_ENTRY(  80, 255,  80 ),  // 10 - lime
		DWORD_COLOR_ENTRY( 255, 255,  80 ),  // 11 - yellow
		DWORD_COLOR_ENTRY(  80,  80, 255 ),  // 12 - blue
		DWORD_COLOR_ENTRY( 255,  80, 255 ),  // 13 - fuchsia
		DWORD_COLOR_ENTRY(  80, 255, 255 ),  // 14 - aqua
		DWORD_COLOR_ENTRY( 255, 255, 255 ),  // 15 - white (full bright)

	// UserInterface colors:
		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // window shadow
		DWORD_COLOR_ENTRY( 160,  24,  12 ),  // window border a title background
		DWORD_COLOR_ENTRY( 242, 238, 233 ),  // window background
		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // foreground, text
		DWORD_COLOR_ENTRY( 196, 215, 245 ),  // highlight background
		DWORD_COLOR_ENTRY( 160, 160, 160 ),  // disabled item, inactive text
		DWORD_COLOR_ENTRY( 200, 200, 200 ),  // checkbox/radio border, separator
		DWORD_COLOR_ENTRY(   0, 160,   0 ),  // checkbox/radio active symbol
		DWORD_COLOR_ENTRY(  80,  80, 255 ),  // smart-key
		DWORD_COLOR_ENTRY(   0,   0, 160 ),  // hotkey/directory
		DWORD_COLOR_ENTRY(   8,  32,  64 ),  // debugger background
		DWORD_COLOR_ENTRY( 233, 238, 242 ),  // debugger foreground
		DWORD_COLOR_ENTRY(  32,  64, 128 ),  // debugger highlight cursor
		DWORD_COLOR_ENTRY(  96, 112, 128 ),  // debugger border
		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // (reserved)
		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // (reserved)
		DWORD_COLOR_ENTRY(  68,  68,  68 ),  // statusbar standard text
		DWORD_COLOR_ENTRY( 224,  27,  76 ),  // statusbar paused blinking text
		DWORD_COLOR_ENTRY(  16,  24,  16 ),  // statusbar tape background
		DWORD_COLOR_ENTRY(  40, 100,  50 ),  // statusbar tape foreground
	};

	palette = GUI->globalPalette;
	SDL_memset4(palette, 0, 256);
	memcpy(palette, stdpal, sizeof(stdpal));
}
//-----------------------------------------------------------------------------
