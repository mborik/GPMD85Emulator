/*	ScreenPMD85.cpp: Core of graphical output and screen generation
	Copyright (c) 2010-2026 Martin Borik <martin@borik.net>

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
//-----------------------------------------------------------------------------
#include "CommonUtils.h"
#include "ScreenPMD85.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
#define GL_GLEXT_PROTOTYPES
#ifdef IMGUI_IMPL_OPENGL_ES2
#  include "SDL_opengles2.h"
#else
#  include "SDL_opengl.h"
#endif
//-----------------------------------------------------------------------------
ScreenPMD85::ScreenPMD85(TDisplayMode dispMode, int border)
{
	screenPixelBuffer = NULL;
	scanlinerPixelBuffer = NULL;

	glGenTextures(1, &screenTexture);
	glGenTextures(1, &scanlinerTexture);

	screenRect = NULL;
	palette = NULL;

	blinkState = false;
	blinkingEnabled = false;
	halfPass = HP_50;
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

	if (scanlinerPixelBuffer) {
		delete[] scanlinerPixelBuffer;
		scanlinerPixelBuffer = NULL;
	}

	SDL_UnlockMutex(displayModeMutex);
	SDL_DestroyMutex(displayModeMutex);
}
//-----------------------------------------------------------------------------
void ScreenPMD85::ReleaseVideoMode()
{
	if (screenRect) {
		delete screenRect;
		screenRect = NULL;
	}
	if (screenPixelBuffer) {
		delete[] screenPixelBuffer;
		screenPixelBuffer = NULL;
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
		return static_cast<TColor>(cAttr[idx]);
	else
		return TColor::BLACK;
}
//---------------------------------------------------------------------------
void ScreenPMD85::RefreshDisplay()
{
	if (SDL_TryLockMutex(displayModeMutex) != 0)
		return;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bufferWidth, bufferHeight, GL_RGBA, GL_UNSIGNED_BYTE, screenPixelBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	SDL_UnlockMutex(displayModeMutex);
}
//---------------------------------------------------------------------------
void ScreenPMD85::FillBuffer(BYTE *videoRam, bool needRedraw)
{
	// test if there is something to draw and we're not locked in another thread...
	if (!(videoRam && needRedraw && SDL_TryLockMutex(displayModeMutex) == 0))
		return;

	bool colorace = (colorProfile == CP_COLORACE);
	int i, w = bufferWidth * sizeof(DWORD), h = bufferHeight, c2717 = (width384mode ? 0 : 0x40);
	BYTE a[4] = { pAttr[0], pAttr[1], pAttr[2], pAttr[3] }, b, c, d, e;

	if (blinkingEnabled && blinkState)
		a[2] = a[3] = 0;

	DWORD *ptr;
	BYTE *dst = screenPixelBuffer;

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

	SDL_UnlockMutex(displayModeMutex);
}
//---------------------------------------------------------------------------
void ScreenPMD85::InitVideoMode(TDisplayMode reqDispMode, bool reqWidth384)
{
	SDL_LockMutex(displayModeMutex);
	ReleaseVideoMode();

	// debug("Screen", "InitVideoMode: %d, %s", reqDispMode, reqWidth384 ? "true" : "false");

	dispMode = reqDispMode;
	width384mode = reqWidth384;

	if (dispMode == DM_FULLSCREEN)
		reqDispMode = DM_QUINTUPLESIZE;

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

			case DM_QUINTUPLESIZE:
				screenWidth  = (reqWidth384) ? 1920 : 1440;
				screenHeight = 1280;
				break;

		}

		if (dispMode == DM_FULLSCREEN) {
			if (screenWidth > gdc.w || screenHeight + (int) STATUSBAR_HEIGHT > gdc.h) {
				if (reqDispMode == DM_QUINTUPLESIZE)
					reqDispMode = DM_QUADRUPLESIZE;
				else if (reqDispMode == DM_QUADRUPLESIZE)
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

	windowWidth  = screenWidth + (borderSize * 2);
	windowHeight = screenHeight + (borderSize * 2);

	debug("Screen", "Windowed mode: %dx%d -> viewport: %dx%d",
			screenWidth, screenHeight, windowWidth, windowHeight);

	screenPixelBuffer = new BYTE[bufferWidth * bufferHeight * sizeof(DWORD)];
	if (!screenPixelBuffer)
		error("Screen", "Unable to create screen pixel buffer!");

	glBindTexture(GL_TEXTURE_2D, screenTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, bufferWidth, bufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, screenPixelBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);

	PrepareScanliner();
	PrepareScreen();

	SDL_UnlockMutex(displayModeMutex);
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareScreen()
{
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareScanliner()
{
	int reqDispMode = (int) dispMode;
	int multiplier = GetMultiplier();
	if (!reqDispMode) // DM_FULLSCREEN
		reqDispMode = multiplier;

	--reqDispMode;
	if (reqDispMode != scanlinerMode) {
		scanlinerMode = reqDispMode;

		if (scanlinerPixelBuffer)
			delete[] scanlinerPixelBuffer;
		scanlinerPixelBuffer = NULL;

		if (!scanlinerMode || (halfPass == HP_OFF && !lcdMode))
			scanlinerMode = 0;

		int scanlinerBufferSize = (screenWidth * screenHeight) * sizeof(DWORD);
		scanlinerPixelBuffer = new BYTE[scanlinerBufferSize];
		if (!scanlinerPixelBuffer)
			error("Screen", "Unable to create scanliner pixel buffer!");

		memset(scanlinerPixelBuffer, 0, scanlinerBufferSize);

		glBindTexture(GL_TEXTURE_2D, scanlinerTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scanlinerPixelBuffer);
	}
	else if (!reqDispMode)
		scanlinerMode = 0;

	if (scanlinerMode) {
		int q = (lcdMode ? 5 : (int) halfPass);
		DWORD *sclGrading;
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
			case 4:
				scanlinerFn = &point5x;
				sclGrading = ((DWORD *) &scanliner->x5) + q * 25;
				break;
			default:
				warning("Screen", "Invalid size for scanline blitter");
				return;
		}

		DWORD *pixels = (DWORD *) scanlinerPixelBuffer;
		(*scanlinerFn) (pixels, screenWidth, sclGrading, bufferWidth, bufferHeight);
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, scanlinerTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, scanlinerPixelBuffer);
	glBindTexture(GL_TEXTURE_2D, 0);
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
scanlinerMethodPrototype(point5x)
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
			*p++ = *c++;
			*p = *c++;
			p += pitch - 4;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 4;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 4;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p = *c++;
			p += pitch - 4;

			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c++;
			*p++ = *c;
			p -= (pitch * 4);
		}

		dst += pitch * 5;
	}
}
//-----------------------------------------------------------------------------
void ScreenPMD85::InitScanliners()
{
	// create a opacity grading palette entries for scanliners...
	DWORD _E = SDL_FOURCC(0, 0, 0,   0); // transparent
	DWORD _D = SDL_FOURCC(0, 0, 0,  64); // 75%
	DWORD _C = SDL_FOURCC(0, 0, 0, 128); // 50%
	DWORD _B = SDL_FOURCC(0, 0, 0, 192); // 25%
	DWORD _A = SDL_FOURCC(0, 0, 0, 255); // opaque

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
		},
// one point is overlaid with 25-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//           75%                     50%                     25%                      0%
//  | E | E | E | E | E |   | E | E | E | E | E |   | E | E | E | E | E |   | E | E | E | E | E |
//  | E | E | E | E | E |   | E | E | E | E | E |   | D | D | D | D | D |   | C | C | C | C | C |
//  | E | E | E | E | E |   | D | D | D | D | D |   | C | C | C | C | C |   | B | B | B | B | B |
//  | E | E | E | E | E |   | D | D | D | D | D |   | C | C | C | C | C |   | B | B | B | B | B |
//  | D | D | D | D | D |   | C | C | C | C | C |   | B | B | B | B | B |   | A | A | A | A | A |
//           LCD
//  | E | E | E | E | D |
//  | E | D | D | D | E |
//  | E | D | D | D | E |
//  | E | D | D | D | E |
//  | A | B | B | B | A |
		{
			_E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, // off
			_E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _D, _D, _D, _D, _D, _D, _D, _D, _D, _D, // hp75
			_E, _E, _E, _E, _E, _E, _E, _E, _E, _E, _D, _D, _D, _D, _D, _C, _C, _C, _C, _C, _B, _B, _B, _B, _B, // hp50
			_E, _E, _E, _E, _E, _D, _D, _D, _D, _D, _C, _C, _C, _C, _C, _B, _B, _B, _B, _B, _A, _A, _A, _A, _A, // hp25
			_E, _E, _E, _E, _E, _C, _C, _C, _C, _C, _B, _B, _B, _B, _B, _B, _B, _B, _B, _B, _A, _A, _A, _A, _A, // hp0
			_E, _E, _E, _E, _D, _E, _D, _D, _D, _E, _E, _D, _D, _D, _E, _E, _D, _D, _D, _E, _A, _B, _B, _B, _A  // lcd
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
