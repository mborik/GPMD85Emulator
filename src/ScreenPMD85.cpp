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
	Renderer = NULL;
	ScreenBuffer = NULL;
	BlitRectDest = NULL;

	DisplayModeChanging = true;
	BlinkState = false;
	BlinkingEnabled = false;
	LCDmode = false;
	computerModel[0] = '\0';
	ledState = 0;
	iconState = 0;
	statusFPS = 0;
	statusPercentage = 0;
	borderSize = (border * BORDER_MULTIPLIER);

	InitPalette();
	InitScreenSize(dispMode, false);

	PrepareVideoMode();
	SDL_ShowWindow(gvi.window);

	SetColorProfile(CP_STANDARD);
	SetHalfPassMode(HP_OFF);

	DisplayModeChanging = false;
}
//-----------------------------------------------------------------------------
ScreenPMD85::~ScreenPMD85()
{
	DisplayModeChanging = true;
	SDL_RenderClear(Renderer);
	SDL_RenderPresent(Renderer);

	ReleaseVideoMode();

	if (BlitRectDest)
		delete BlitRectDest;
	BlitRectDest = NULL;
}
//-----------------------------------------------------------------------------
void ScreenPMD85::SetDisplayMode(TDisplayMode dispMode, int border)
{
	border *= BORDER_MULTIPLIER;
	if (borderSize != border) {
		if (DispMode != DM_FULLSCREEN) {
			DispMode = (TDisplayMode) -1;
			borderSize = border;
		}
	}

	if (DispMode == dispMode)
		return;

	DisplayModeChanging = true;

	SDL_Delay(WEAK_REFRESH_TIME);
	InitScreenSize(dispMode, Width384mode);
	ReleaseVideoMode();
	PrepareVideoMode();

	DisplayModeChanging = false;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetWidth384(bool mode384)
{
	if (Width384mode == mode384)
		return;

	DisplayModeChanging = true;

	SDL_Delay(WEAK_REFRESH_TIME);
	InitScreenSize(DispMode, mode384);

	ReleaseVideoMode();
	PrepareVideoMode();

	DisplayModeChanging = false;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetHalfPassMode(THalfPassMode halfPass)
{
	HalfPass = halfPass;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetLcdMode(bool lcdMode)
{
	LCDmode = lcdMode;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetColorProfile(TColorProfile ColProf)
{
	ColorProfile = ColProf;
	switch (ColProf) {
		case CP_MONO:
			PAttr[0] = WHITE;
			PAttr[1] = WHITE;
			PAttr[2] = WHITE;
			PAttr[3] = WHITE;
			BlinkingEnabled = false;
			break;

		case CP_STANDARD:
			PAttr[0] = WHITE;
			PAttr[1] = SILVER;
			PAttr[2] = WHITE;
			PAttr[3] = SILVER;
			BlinkingEnabled = true;
			break;

		case CP_COLOR:
			PAttr[0] = CAttr[0];
			PAttr[1] = CAttr[1];
			PAttr[2] = CAttr[2];
			PAttr[3] = CAttr[3];
			BlinkingEnabled = false;
			break;

		case CP_COLORACE:
			PAttr[0] = BLACK;
			PAttr[1] = RED;
			PAttr[2] = BLUE;
			PAttr[3] = FUCHSIA;
			PAttr[4] = LIME;
			PAttr[5] = YELLOW;
			PAttr[6] = AQUA;
			PAttr[7] = WHITE;
			BlinkingEnabled = false;
			break;
	}
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetColorAttr(int Index, TColor Attr)
{
	if (Index >= 0 && Index <= 3) {
		CAttr[Index] = Attr;
		if (ColorProfile == CP_COLOR) {
			PAttr[Index] = Attr;
			PAttr[Index + 4] = Attr;
		}
	}
}
//---------------------------------------------------------------------------
TColor ScreenPMD85::GetColorAttr(int Index)
{
	if (Index >= 0 && Index <= 3)
		return CAttr[Index];
	else
		return BLACK;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetLedState(int led)
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
void ScreenPMD85::SetIconState(int icon)
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
void ScreenPMD85::SetComputerModel(TComputerModel model)
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
void ScreenPMD85::RefreshDisplay()
{
	if (DisplayModeChanging)
		return;

	PrepareStatusBar();

	SDL_RenderCopy(Renderer, ScreenBuffer, NULL, BlitRectDest);

	if (GUI->isInMenu() && GUI->needRedraw) {
		// TODO SDL_RenderCopy GUI screen
		GUI->needRedraw = false;
	}

	RedrawStatusBar();

	SDL_RenderPresent(Renderer);
}
//---------------------------------------------------------------------------
void ScreenPMD85::FillBuffer(BYTE *videoRam)
{
	if (DisplayModeChanging || videoRam == NULL)
		return;

	bool colorace = (ColorProfile == CP_COLORACE);
	int i, w, h = bufferHeight, c2717 = (Width384mode ? 0 : 0x40);
	BYTE a[4] = { PAttr[0], PAttr[1], PAttr[2], PAttr[3] }, b, c, d, e;

	if (BlinkingEnabled && BlinkState)
		a[2] = a[3] = 0;

	DWORD *ptr;
	BYTE *dst;
	SDL_LockTexture(ScreenBuffer, NULL, (void **) &dst, &w);

	while (h--) {
		ptr = (DWORD *) dst;
		for (i = 0; i < 48; i++) {
			b = videoRam[i];
			d = (b & 0xC0) >> 6;

			if (colorace) {
				e = videoRam[i + ((h & 1) ? 64 : -64)];
				c = (e & 0xC0) >> 6;
				c = PAttr[d | c | ((d & c) ? 0 : 4)];
			}
			else if (c2717)
				c = a[d];
			else
				c = *a;

			for (d = 0x01; d != c2717; d <<= 1)
				*ptr++ = Palette[((b & d) ? c : 0)];
		}

		dst += w;
		videoRam += 64;
	}

	SDL_UnlockTexture(ScreenBuffer);
}
//---------------------------------------------------------------------------
void ScreenPMD85::InitScreenSize(TDisplayMode reqDispMode, bool reqWidth384)
{
	DispMode = reqDispMode;
	Width384mode = reqWidth384;
	FullScreenScaleMode = (TDisplayMode) -1;

	if (DispMode == DM_FULLSCREEN)
		reqDispMode = DM_QUADRUPLESIZE;

	while (true) {
		switch (reqDispMode) {
			default:
			case DM_NORMAL:
				Width = (reqWidth384) ? 384 : 288;
				Height = 256;
				break;

			case DM_DOUBLESIZE:
				Width = (reqWidth384) ? 768 : 576;
				Height = 512;
				break;

			case DM_TRIPLESIZE:
				Width = (reqWidth384) ? 1152 : 864;
				Height = 768;
				break;

			case DM_QUADRUPLESIZE:
				Width = (reqWidth384) ? 1536 : 1152;
				Height = 1024;
				break;
		}

		if (DispMode == DM_FULLSCREEN) {
			if (Width > gvi.w || Height + STATUSBAR_HEIGHT > gvi.h) {
				if (reqDispMode == DM_QUADRUPLESIZE)
					reqDispMode = DM_TRIPLESIZE;
				else if (reqDispMode == DM_TRIPLESIZE)
					reqDispMode = DM_DOUBLESIZE;
				else if (reqDispMode == DM_DOUBLESIZE)
					reqDispMode = DM_NORMAL;
				else {
					DispMode = DM_NORMAL;
					break;
				}

				continue;
			}
			else {
				FullScreenScaleMode = reqDispMode;
				break;
			}
		}
		else break;
	}

	bufferWidth = (reqWidth384) ? 384 : 288;
	bufferHeight = 256;

	if (BlitRectDest)
		delete BlitRectDest;
	BlitRectDest = new SDL_Rect;

	if (DispMode == DM_FULLSCREEN) {
		BlitRectDest->w = Width;
		BlitRectDest->h = Height;

		Height += STATUSBAR_HEIGHT;
		BlitRectDest->x = (gvi.w - Width) / 2;
		BlitRectDest->y = (gvi.h - Height) / 2;
	}
	else {
		BlitRectDest->x = borderSize;
		BlitRectDest->y = borderSize;
		BlitRectDest->w = Width;
		BlitRectDest->h = Height;

		Width  += (borderSize * 2);
		Height += (borderSize * 2) + STATUSBAR_HEIGHT;
	}
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareVideoMode()
{
	if (DispMode == DM_FULLSCREEN) {
		debug("Screen", "Full-screen mode: %dx%d", gvi.w, gvi.h);
		SDL_SetWindowFullscreen(gvi.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else {
		debug("Screen", "Windowed mode: %dx%d", Width, Height);
		SDL_SetWindowSize(gvi.window, Width, Height);
	}

	Renderer = SDL_CreateRenderer(gvi.window, -1,
			SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	if (!Renderer)
		error("Screen", "Unable to create screen buffer\n%s", SDL_GetError());

	ScreenBuffer = SDL_CreateTexture(Renderer, SDL_PIXELFORMAT_RGBA8888,
			SDL_TEXTUREACCESS_STREAMING, bufferWidth, bufferHeight);

	if (!ScreenBuffer)
		error("Screen", "Unable to create ScreenBuffer texture\n%s", SDL_GetError());

//	GUI->prepareDefaultSurface(bufferWidth, bufferHeight, Palette);

	PrepareStatusBar(true);
	SDL_RenderPresent(Renderer);
}
//-----------------------------------------------------------------------------
void ScreenPMD85::ReleaseVideoMode()
{
/*
	if (GUI->defaultSurface) {
		SDL_FreeSurface(GUI->defaultSurface);
		GUI->defaultSurface = NULL;
	}
*/
	if (ScreenBuffer) {
		SDL_DestroyTexture(ScreenBuffer);
		ScreenBuffer = NULL;
	}
	if (Renderer) {
		SDL_DestroyRenderer(Renderer);
		Renderer = NULL;
	}
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareStatusBar(bool clear)
{
	if (clear) {
		SDL_SetRenderDrawColor(Renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(Renderer);
	}

	SDL_Rect *r = new SDL_Rect(*BlitRectDest);
	SDL_SetRenderDrawColor(Renderer, 16, 16, 16, SDL_ALPHA_OPAQUE);

	int i = 0;
	if (borderSize > 0) {
		i = (BlitRectDest->h / 256) * 2;
		r->x -= i;
		r->y -= i;
		r->w += i * 2;
		r->h += i * 2;

		SDL_RenderDrawRect(Renderer, r);
	}
	else {
		int y = Height - STATUSBAR_HEIGHT;
		SDL_RenderDrawLine(Renderer, r->x, y, r->x + r->w, y);
	}

	delete r;
}
//-----------------------------------------------------------------------------
void ScreenPMD85::RedrawStatusBar()
{
/*
	SDL_Rect *r = new SDL_Rect(*BlitRectDest), *s = new SDL_Rect;

	r->x += r->w - (4 * STATUSBAR_SPACING);
	r->y += r->h + ((STATUSBAR_HEIGHT - STATUSBAR_ICON) / 2)
	             + (r->y / BORDER_MULTIPLIER)
	             + (BlitRectDest->h / 256);

	r->w = r->h = s->w = s->h = STATUSBAR_ICON;

//	control LEDs on right side...
	s->y = 0;
	s->x = (ledState & 1) ? STATUSBAR_ICON : 0;
	SDL_LowerBlit(GUI->icons, s, Screen, r);

	r->x += STATUSBAR_SPACING;
	s->x = (ledState & 2) ? (2 * STATUSBAR_ICON) : 0;
	SDL_LowerBlit(GUI->icons, s, Screen, r);

	r->x += STATUSBAR_SPACING;
	s->x = (ledState & 4) ? (3 * STATUSBAR_ICON) : 0;
	SDL_LowerBlit(GUI->icons, s, Screen, r);

//	tape/disk icon...
	r->x -= (4 * STATUSBAR_SPACING);
	if (iconState) {
		s->x = (iconState * STATUSBAR_ICON) + (3 * STATUSBAR_ICON);
		SDL_LowerBlit(GUI->icons, s, Screen, r);
	}
	else
		SDL_FillRect(Screen, r, 0);

	delete s;

	if (SDL_LockSurface(Screen) != 0) {
		delete r;
		return;
	}

	int tapProgressWidth = r->x - STATUSBAR_SPACING;
	static char status[24] = "";
	static BYTE pauseBlinker = 0;

//	status text, cpu meter and blinking pause...
	r->x = BlitRectDest->x + STATUSBAR_SPACING;
	r->y += 2;

	GUI->printText(Screen, r->x, r->y, 0, status);
	if (statusPercentage < 0) {
		sprintf(status, "PAUSED");
		GUI->printText(Screen, r->x, r->y, (pauseBlinker < 10) ? 111 : 0, status);
		if (pauseBlinker++ >= 16)
			pauseBlinker = 0;
	}
	else if (statusPercentage > 0) {
		sprintf(status, "%sFPS:%d CPU:%d%%", computerModel, statusFPS, statusPercentage);
		GUI->printText(Screen, r->x, r->y, 55, status);
	}

//	tape progress bar...
	r->x = (r->x + (20 * 6) + STATUSBAR_SPACING);
	r->y += 3;
	r->w = tapProgressWidth - r->x;
	r->h = 2;

	SDL_FillRect(Screen, r, *(TapeBrowser->ProgressBar->Active) ? SDL_MapRGB(Screen->format, 16, 24, 16) : 0);
	if (*(TapeBrowser->ProgressBar->Active)) {
		r->w = ((double) r->w / TapeBrowser->ProgressBar->Max) * TapeBrowser->ProgressBar->Position;
		SDL_FillRect(Screen, r, SDL_MapRGB(Screen->format, 40, 100, 50));
	}

	SDL_UnlockSurface(Screen);
	delete r;
*/
}
//-----------------------------------------------------------------------------
void ScreenPMD85::InitPalette()
{
	static DWORD stdpal[64] = {
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

		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // }
		DWORD_COLOR_ENTRY( 114,   6,   6 ),  //  |
		DWORD_COLOR_ENTRY(   6, 114,   6 ),  //  |
		DWORD_COLOR_ENTRY( 114, 114,   6 ),  //  |
		DWORD_COLOR_ENTRY(   6,   6, 114 ),  //  |
		DWORD_COLOR_ENTRY( 114,   6, 114 ),  //  |
		DWORD_COLOR_ENTRY(   6, 114, 114 ),  //  |
		DWORD_COLOR_ENTRY( 120, 120, 120 ),  //   } - 75% of bright
		DWORD_COLOR_ENTRY( 144, 144, 144 ),  //  |
		DWORD_COLOR_ENTRY( 185,  66,  66 ),  //  |
		DWORD_COLOR_ENTRY(  66, 185,  66 ),  //  |
		DWORD_COLOR_ENTRY( 185, 185,  66 ),  //  |
		DWORD_COLOR_ENTRY(  66,  66, 185 ),  //  |
		DWORD_COLOR_ENTRY( 185,  66, 185 ),  //  |
		DWORD_COLOR_ENTRY(  80, 185, 185 ),  //  |
		DWORD_COLOR_ENTRY( 191, 191, 191 ),  // }

		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // }
		DWORD_COLOR_ENTRY(  82,   9,   9 ),  //  |
		DWORD_COLOR_ENTRY(   9,  82,   9 ),  //  |
		DWORD_COLOR_ENTRY(  82,  82,   9 ),  //  |
		DWORD_COLOR_ENTRY(   9,   9,  82 ),  //  |
		DWORD_COLOR_ENTRY(  82,   9,  82 ),  //  |
		DWORD_COLOR_ENTRY(   9,  82,  82 ),  //  |
		DWORD_COLOR_ENTRY(  90,  90,  90 ),  //   } - 50% of bright
		DWORD_COLOR_ENTRY( 108, 108, 108 ),  //  |
		DWORD_COLOR_ENTRY( 134,  54,  54 ),  //  |
		DWORD_COLOR_ENTRY(  54, 134,  54 ),  //  |
		DWORD_COLOR_ENTRY( 134, 134,  54 ),  //  |
		DWORD_COLOR_ENTRY(  54,  54, 134 ),  //  |
		DWORD_COLOR_ENTRY( 134,  54, 134 ),  //  |
		DWORD_COLOR_ENTRY(  54, 134, 134 ),  //  |
		DWORD_COLOR_ENTRY( 144, 144, 144 ),  // }

		DWORD_COLOR_ENTRY(   0,   0,   0 ),  // }
		DWORD_COLOR_ENTRY(  60,   8,   8 ),  //  |
		DWORD_COLOR_ENTRY(   8,  60,   8 ),  //  |
		DWORD_COLOR_ENTRY(  60,  60,   8 ),  //  |
		DWORD_COLOR_ENTRY(   8,   8,  60 ),  //  |
		DWORD_COLOR_ENTRY(  60,   8,  60 ),  //  |
		DWORD_COLOR_ENTRY(   8,  60,  60 ),  //  |
		DWORD_COLOR_ENTRY(  68,  68,  68 ),  //   } - 25% of bright
		DWORD_COLOR_ENTRY(  81,  81,  81 ),  //  |
		DWORD_COLOR_ENTRY(  98,  44,  44 ),  //  |
		DWORD_COLOR_ENTRY(  44,  98,  44 ),  //  |
		DWORD_COLOR_ENTRY(  98,  98,  44 ),  //  |
		DWORD_COLOR_ENTRY(  44,  44,  98 ),  //  |
		DWORD_COLOR_ENTRY(  98,  44,  98 ),  //  |
		DWORD_COLOR_ENTRY(  44,  98,  98 ),  //  |
		DWORD_COLOR_ENTRY( 108, 108, 108 )   // }
	};

	// UserInterface colors:
	static DWORD guipal[16] = {
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
		DWORD_COLOR_ENTRY(   0,   0,   0 ),  //
		DWORD_COLOR_ENTRY( 224,  27,  76 )   //
	};

	memset(Palette, 0, sizeof(DWORD) * 256);
	memcpy(Palette, stdpal, sizeof(DWORD) * 64);
	for (int i = 80; i < 160; i += 16)
		memcpy(Palette + i, guipal, sizeof(guipal));
}
//-----------------------------------------------------------------------------
