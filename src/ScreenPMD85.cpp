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
#include "Emulator.h"
//-----------------------------------------------------------------------------
ScreenPMD85::ScreenPMD85(TDisplayMode dispMode, int border)
{
	scanlinerTexture = NULL;
	screenTexture = NULL;
	screenRect = NULL;
	palette = NULL;

	blinkState = false;
	blinkingEnabled = false;
	halfPass = HP_OFF;
	lcdMode = false;

	scanlinerMode = 0;
	borderSize = (border * BORDER_MULTIPLIER);

	InitPalette();
	InitScanliners();
	SetColorProfile(CP_STANDARD);

	displayModeMutex = SDL_CreateMutex();
	InitVideoMode(dispMode, false);
}
//-----------------------------------------------------------------------------
ScreenPMD85::~ScreenPMD85()
{
	SDL_LockMutex(displayModeMutex);
	ReleaseVideoMode();

	if (scanlinerTexture) {
		SDL_DestroyTexture(scanlinerTexture);
		scanlinerTexture = NULL;
	}

	SDL_UnlockMutex(displayModeMutex);
	SDL_DestroyMutex(displayModeMutex);
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

	InitVideoMode(dm, width384mode);
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetWidth384(bool mode384)
{
	if (width384mode == mode384)
		return;

	InitVideoMode(dispMode, mode384);
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetHalfPassMode(THalfPassMode hp)
{
	if (halfPass != hp) {
		halfPass = hp;
		PrepareScanliner();
	}
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetLcdMode(bool state)
{
	if (lcdMode != state) {
		lcdMode = state;
		PrepareScanliner();
	}
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
	if (SDL_TryLockMutex(displayModeMutex) != 0)
		return;

	PrepareScreen();
	SDL_RenderCopy(gdc.renderer, screenTexture, NULL, screenRect);

	if (scanlinerMode)
		SDL_RenderCopy(gdc.renderer, scanlinerTexture, NULL, screenRect);

	GUI->RedrawStatusBar();
	if (GUI->InMenu())
		SDL_RenderCopy(gdc.renderer, GUI->defaultTexture, NULL, screenRect);

	SDL_RenderPresent(gdc.renderer);
	SDL_UnlockMutex(displayModeMutex);
}
//---------------------------------------------------------------------------
void ScreenPMD85::FillBuffer(BYTE *videoRam)
{
	if (SDL_TryLockMutex(displayModeMutex) != 0 || videoRam == NULL)
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
	SDL_UnlockMutex(displayModeMutex);
}
//---------------------------------------------------------------------------
void ScreenPMD85::InitVideoMode(TDisplayMode reqDispMode, bool reqWidth384)
{
	SDL_LockMutex(displayModeMutex);
	ReleaseVideoMode();

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

		debug("Screen", "Full-screen mode: %dx%d -> viewport: %dx%d",
				screenWidth, screenHeight, screenRect->w, screenRect->h);

		SDL_SetWindowFullscreen(gdc.window, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else {
		screenRect->x = borderSize;
		screenRect->y = borderSize;
		screenRect->w = screenWidth;
		screenRect->h = screenHeight;

		screenWidth  += (borderSize * 2);
		screenHeight += (borderSize * 2) + STATUSBAR_HEIGHT;

		debug("Screen", "Windowed mode: %dx%d -> viewport: %dx%d",
				screenWidth, screenHeight, screenRect->w, screenRect->h);

		SDL_SetWindowFullscreen(gdc.window, 0);
		SDL_SetWindowSize(gdc.window, screenWidth, screenHeight);
	}

	SDL_Event event;
	int waitForResize = WEAK_REFRESH_TIME;
	while (--waitForResize > 0) {
		if (SDL_PollEvent(&event) &&
			event.type == SDL_WINDOWEVENT &&
			event.window.windowID == gdc.windowID &&
			event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				break;

		SDL_Delay(1);
	}

	if (SDL_RenderSetLogicalSize(gdc.renderer, screenWidth, screenHeight) != 0)
		error("Screen", "Unable to change screen resolution\n%s", SDL_GetError());
	SDL_RenderSetViewport(gdc.renderer, NULL);

	screenTexture = SDL_CreateTexture(gdc.renderer, SDL_PIXELFORMAT_DEFAULT,
			SDL_TEXTUREACCESS_STREAMING, bufferWidth, bufferHeight);
	if (!screenTexture)
		error("Screen", "Unable to create screen texture\n%s", SDL_GetError());

	GUI->statusRect = new SDL_Rect(*screenRect);
	GUI->statusRect->x += STATUSBAR_SPACING;
	GUI->statusRect->y += screenRect->h + (borderSize - STATUSBAR_HEIGHT / 2);
	GUI->statusRect->w -= (2 * STATUSBAR_SPACING);
	GUI->statusRect->h  = STATUSBAR_HEIGHT;

	GUI->InitStatusBarTexture();
	GUI->InitDefaultTexture(bufferWidth, bufferHeight);

	PrepareScanliner();
	PrepareScreen(true);

	SDL_RenderPresent(gdc.renderer);
	SDL_UnlockMutex(displayModeMutex);
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareScreen(bool clear)
{
	if (clear) {
		SDL_SetRenderDrawColor(gdc.renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(gdc.renderer, NULL);
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
void ScreenPMD85::PrepareScanliner()
{
	int reqDispMode = (int) dispMode;
	if (!reqDispMode) // DM_FULLSCREEN
		reqDispMode = GetMultiplier();

	--reqDispMode;
	if (reqDispMode != scanlinerMode) {
		scanlinerMode = reqDispMode;

		if (scanlinerTexture)
			SDL_DestroyTexture(scanlinerTexture);
		scanlinerTexture = NULL;

		if (!scanlinerMode || (halfPass == HP_OFF && !lcdMode)) {
			scanlinerMode = 0;
			return;
		}
		else {
			scanlinerTexture = SDL_CreateTexture(gdc.renderer,
					SDL_PIXELFORMAT_DEFAULT, SDL_TEXTUREACCESS_STREAMING,
					screenRect->w, screenRect->h);
			if (!scanlinerTexture)
				warning("Screen", "Unable to create scanliner texture\n%s", SDL_GetError());

			SDL_SetTextureBlendMode(scanlinerTexture, SDL_BLENDMODE_BLEND);
		}
	}

	int pitch, q = (lcdMode ? 5 : (int) halfPass);
	DWORD *pixels, *sclGrading;
	scanlinerMethod scanlinerFn;

	switch (reqDispMode) {
		case 1:
			scanlinerFn = &point2x;
			sclGrading = ((DWORD *) &scanliner->x2) + q * 4;
			break;
		case 2:
			scanlinerFn = &point3x;
			sclGrading = ((DWORD *) &scanliner->x3) + q * 9;
			break;
		case 3:
			scanlinerFn = &point4x;
			sclGrading = ((DWORD *) &scanliner->x4) + q * 16;
			break;
		default:
			warning("Screen", "Invalid size for scanline blitter");
			return;
	}

	SDL_LockTexture(scanlinerTexture, NULL, (void **) &pixels, &pitch);
	(*scanlinerFn) (pixels, pitch >> 2, sclGrading, bufferWidth, bufferHeight);
	SDL_UnlockTexture(scanlinerTexture);
}
//-----------------------------------------------------------------------------
scanlinerMethodPrototype(point2x)
{
	int i;
	DWORD *c, *p;

	while (h--) {
		p = dst;
		for (i = 0; i < w; ++i) {
			c = scl;
			*p++ = *c++;
			*p-- = *c++;
			p += pitch;

			*p++ = *c++;
			*p++ = *c;
			p -= pitch;
		}

		dst += pitch * 2;
	}
}
//-----------------------------------------------------------------------------
scanlinerMethodPrototype(point3x)
{
	int i;
	DWORD *c, *p;

	while (h--) {
		p = dst;
		for (i = 0; i < w; ++i) {
			c = scl;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 2;

			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 2;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c;
			p -= (pitch * 2);
		}

		dst += pitch * 3;
	}
}
//-----------------------------------------------------------------------------
scanlinerMethodPrototype(point4x)
{
	int i;
	DWORD *c, *p;

	while (h--) {
		p = dst;
		for (i = 0; i < w; ++i) {
			c = scl;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 3;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 3;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 3;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c;
			p -= (pitch * 3);
		}

		dst += pitch * 4;
	}
}
//-----------------------------------------------------------------------------
void ScreenPMD85::InitScanliners()
{
	SDL_PixelFormat *fmt = SDL_AllocFormat(SDL_PIXELFORMAT_DEFAULT);

	// create a opacity grading palette entries for scanliners...
	DWORD _E = SDL_MapRGBA(fmt, 0, 0, 0,   0); // transparent
	DWORD _D = SDL_MapRGBA(fmt, 0, 0, 0,  64); // 75%
	DWORD _C = SDL_MapRGBA(fmt, 0, 0, 0, 128); // 50%
	DWORD _B = SDL_MapRGBA(fmt, 0, 0, 0, 192); // 25%
	DWORD _A = SDL_MapRGBA(fmt, 0, 0, 0, 255); // opaque

	SDL_FreeFormat(fmt);

	static const SCANLINER_DEF sc = {
// one point is overlaid with four-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//     75%          50%          25%          0%           LCD
//  | E | E |    | E | E |    | E | E |    | E | E |    | E | D |
//  | D | D |    | C | C |    | B | B |    | A | A |    | B | C |
		{
			_E, _E, _E, _E, // off
			_E, _E, _D, _D, // hp75
			_E, _E, _C, _C, // hp50
			_E, _E, _B, _B, // hp25
			_E, _E, _A, _A, // hp0
			_E, _D, _B, _C  // lcd
		},
// one point is overlaid with nine-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//       75%             50%             25%             0%              LCD
//  | E | E | E |   | E | E | E |   | E | E | E |   | E | E | E |   | E | E | D |
//  | E | E | E |   | D | D | D |   | C | C | C |   | B | B | B |   | E | D | E |
//  | D | D | D |   | C | C | C |   | B | B | B |   | A | A | A |   | A | B | A |
		{
			_E, _E, _E, _E, _E, _E, _E, _E, _E, // off
			_E, _E, _E, _E, _E, _E, _D, _D, _D, // hp75
			_E, _E, _E, _D, _D, _D, _C, _C, _C, // hp50
			_E, _E, _E, _C, _C, _C, _B, _B, _B, // hp25
			_E, _E, _E, _B, _B, _B, _A, _A, _A, // hp0
			_E, _E, _D, _E, _D, _E, _A, _B, _A  // lcd
		},
// one point is overlaid with 16-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//         75%                 50%                 25%                  0%
//  | E | E | E | E |   | E | E | E | E |   | E | E | E | E |   | E | E | E | E |
//  | E | E | E | E |   | E | E | E | E |   | D | D | D | D |   | C | C | C | C |
//  | E | E | E | E |   | D | D | D | D |   | C | C | C | C |   | B | B | B | B |
//  | D | D | D | D |   | C | C | C | C |   | B | B | B | B |   | A | A | A | A |
//         LCD
//  | E | E | E | D |
//  | E | D | D | E |
//  | E | D | D | E |
//  | A | B | B | A |
		{
			_E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, // off
			_E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _D, _D, _D, _D, // hp75
			_E, _E, _E, _E, _E, _E, _E, _E, _D, _D, _D, _D, _C, _C, _C, _C, // hp50
			_E, _E, _E, _E, _D, _D, _D, _D, _C, _C, _C, _C, _B, _B, _B, _B, // hp25
			_E, _E, _E, _E, _C, _C, _C, _C, _B, _B, _B, _B, _A, _A, _A, _A, // hp0
			_E, _E, _E, _D, _E, _D, _D, _E, _E, _D, _D, _E, _A, _B, _B, _A  // lcd
		}
	};

	scanliner = &sc;
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
