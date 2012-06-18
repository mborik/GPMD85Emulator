/*	ScreenPMD85.cpp: Core of graphical output and screen generation
	Copyright (c) 2010-2012 Martin Borik <mborik@users.sourceforge.net>

	OpenGL screen initialization and rendering inspired by SimCoupe code
	Copyright (c) 1999-2006 Simon Owen <simon.owen@simcoupe.org>

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
	Screen = NULL;
	BlitSurface = NULL;
	BlitRectSrc = NULL;
	BlitRectDest = NULL;
	bufferScreen = NULL;

	RGBpalete(Palette);
	DisplayModeChanging = true;
	BlinkState = false;
	BlinkingEnabled = false;
	LCDmode = false;
	computerModel[0] = '\0';
	ledState = 0;
	iconState = 0;
	statusFPS = 0;
	statusPercentage = 0;
	borderSize = 0;
	if (dispMode != DM_FULLSCREEN && gvi.wm)
		borderSize = (border * BORDER_MULTIPLIER);

#ifdef OPENGL
	Texture[0] = Texture[1] = 0;
	TextureMainWidth = TextureMainHeight = 0;
	TextureStatusWidth = TextureStatusHeight = 0;
	PixelFormat = GL_NONE;
	DataType = GL_UNSIGNED_BYTE;
	Clamp = GL_CLAMP;
#endif

	InitScreenSize(dispMode, false);
	InitScreenBuffer();

	PrepareVideoMode();
	SetColorProfile(CP_STANDARD);
	SetHalfPassMode(HP_OFF);

	DisplayModeChanging = false;
}
//-----------------------------------------------------------------------------
ScreenPMD85::~ScreenPMD85()
{
	DisplayModeChanging = true;
	SDL_FillRect(Screen, BlitRectDest, 0);
	SDL_Flip(Screen);

	ReleaseVideoMode();

	if (BlitRectDest)
		delete BlitRectDest;
	BlitRectDest = NULL;

	if (BlitRectSrc)
		delete BlitRectSrc;
	BlitRectSrc = NULL;

	if (bufferScreen)
		free(bufferScreen);
	bufferScreen = NULL;
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
	SetScaler();

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
	InitScreenBuffer();

	ReleaseVideoMode();
	PrepareVideoMode();
	SetScaler();

	DisplayModeChanging = false;
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetHalfPassMode(THalfPassMode halfPass)
{
	HalfPass = halfPass;
	SetScaler();
}
//---------------------------------------------------------------------------
void ScreenPMD85::SetLcdMode(bool lcdMode)
{
	LCDmode = lcdMode;
	SetScaler();
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

		case CP_MULTICOLOR:
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
		if (ColorProfile == CP_COLOR)
			PAttr[Index] = Attr;
			PAttr[Index + 4] = Attr;
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

	if ((GUI->isInMenu() && GUI->needRedraw) || !GUI->isInMenu()) {
#ifdef OPENGL
		if (SDL_LockSurface(BlitSurface) != 0)
			return;

		GUI->needRedraw = false;

		glBindTexture(GL_TEXTURE_2D, Texture[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, PixelFormat,
			TextureMainWidth, TextureMainHeight, 0,
			PixelFormat, DataType, BlitSurface->pixels);

		SDL_UnlockSurface(BlitSurface);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(BlitRectDest->x, BlitRectDest->y);
			glTexCoord2f(1, 0);
			glVertex2f(BlitRectDest->x + BlitRectDest->w, BlitRectDest->y);
			glTexCoord2f(1, 1);
			glVertex2f(BlitRectDest->x + BlitRectDest->w,
			           BlitRectDest->y + BlitRectDest->h);
			glTexCoord2f(0, 1);
			glVertex2f(BlitRectDest->x, BlitRectDest->y + BlitRectDest->h);
		glEnd();

		RedrawStatusBar();

		glBindTexture(GL_TEXTURE_2D, Texture[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, PixelFormat,
			TextureStatusWidth, TextureStatusHeight, 0,
			PixelFormat, DataType, StatusBar->pixels);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(BlitRectDest->x, BlitRectDest->y + BlitRectDest->h);
			glTexCoord2f(1, 0);
			glVertex2f(BlitRectDest->x + TextureStatusWidth,
			           BlitRectDest->y + BlitRectDest->h);
			glTexCoord2f(1, 1);
			glVertex2f(BlitRectDest->x + TextureStatusWidth,
			           BlitRectDest->y + BlitRectDest->h + TextureStatusHeight);
			glTexCoord2f(0, 1);
			glVertex2f(BlitRectDest->x,
			           BlitRectDest->y + BlitRectDest->h + TextureStatusHeight);
		glEnd();
#else
		if (SDL_LockSurface(BlitSurface) != 0)
			return;

		GUI->needRedraw = false;

		(*Scaler)((BYTE *) BlitSurface->pixels, BlitSurface->pitch,
				bufferScreen, bufferWidth, bufferWidth, bufferHeight);

		SDL_UnlockSurface(BlitSurface);
		SDL_BlitSurface(BlitSurface, BlitRectSrc, Screen, BlitRectDest);
#endif
	}

#ifdef OPENGL
	SDL_GL_SwapBuffers();
#else
	RedrawStatusBar();
	SDL_Flip(Screen);
#endif
}
//---------------------------------------------------------------------------
void ScreenPMD85::FillBuffer(BYTE *videoRam)
{
	if (DisplayModeChanging || videoRam == NULL)
		return;

	bool multicol = (ColorProfile == CP_MULTICOLOR);
	WORD h = bufferHeight, w = bufferWidth;
	BYTE *dst = bufferScreen, *p;
	BYTE a[4], b, c, d, i;

#ifdef OPENGL
	dst = (BYTE *) BlitSurface->pixels;
	w = BlitSurface->pitch;
#endif

	a[0] = PAttr[0];
	a[1] = PAttr[1];
	a[2] = BlinkingEnabled ? ((BlinkState) ? PAttr[2] : 0) : PAttr[2];
	a[3] = BlinkingEnabled ? ((BlinkState) ? PAttr[3] : 0) : PAttr[3];

	while (h--) {
		p = dst;
		for (i = 0; i < 48; ++i) {
			d = ((b = *(videoRam + i)) & 0xC0) >> 6;
			if (multicol) {
				c = (*(videoRam + i + ((((h % 2) << 1) - 1) * 64)) & 0xC0) >> 6;
				c = PAttr[(d | c | ((d * c) ? 0 : 4))];
			}
			else if (Width384mode)
				c = a[0];
			else
				c = a[d];

#ifdef OPENGL
			for (d = 0x01; d != (Width384mode ? 0 : 0x40); d <<= 1) {
				*((SDL_Color *) p) = Palette[((b & d) ? c : 0)];
				p += BlitSurface->format->BytesPerPixel;
			}
#else
			*(p++) = (b & 0x01) ? c : 0;
			*(p++) = (b & 0x02) ? c : 0;
			*(p++) = (b & 0x04) ? c : 0;
			*(p++) = (b & 0x08) ? c : 0;
			*(p++) = (b & 0x10) ? c : 0;
			*(p++) = (b & 0x20) ? c : 0;
			if (Width384mode) {
				*(p++) = (b & 0x40) ? c : 0;
				*(p++) = (b & 0x80) ? c : 0;
			}
#endif
		}

		dst += w;
		videoRam += 64;
	}
}
//---------------------------------------------------------------------------
void ScreenPMD85::InitScreenSize(TDisplayMode reqDispMode, bool reqWidth384)
{
	DispMode = reqDispMode;
	Width384mode = reqWidth384;
	FullScreenScaleMode = (TDisplayMode) -1;

	if ((DispMode == DM_FULLSCREEN || !gvi.wm) && (gvi.w + gvi.h))
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

		if (DispMode == DM_FULLSCREEN || !gvi.wm) {
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

	if (BlitRectSrc)
		delete BlitRectSrc;
	BlitRectSrc = new SDL_Rect;

	BlitRectSrc->x = 0;
	BlitRectSrc->y = 0;
	BlitRectSrc->w = Width;
	BlitRectSrc->h = Height;

	if (BlitRectDest)
		delete BlitRectDest;
	BlitRectDest = new SDL_Rect;

	if (DispMode == DM_FULLSCREEN || !gvi.wm) {
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

#ifdef OPENGL
	TextureMainWidth = TextureMainHeight = 1;
	TextureStatusWidth = TextureStatusHeight = 1;
	while (TextureMainWidth < (unsigned) bufferWidth)
		TextureMainWidth <<= 1;
	while (TextureMainHeight < (unsigned) bufferHeight)
		TextureMainHeight <<= 1;
	while (TextureStatusWidth < (unsigned) BlitRectDest->w)
		TextureStatusWidth <<= 1;
	while (TextureStatusHeight < (unsigned) (STATUSBAR_HEIGHT - (borderSize / BORDER_MULTIPLIER)))
		TextureStatusHeight <<= 1;

	BlitRectDest->w = (WORD) ((float) BlitRectDest->w * (((float) TextureMainWidth) / bufferWidth));
	BlitRectDest->h = (WORD) ((float) BlitRectDest->h * (((float) TextureMainHeight) / bufferHeight));
#endif
}
//-----------------------------------------------------------------------------
void ScreenPMD85::InitScreenBuffer()
{
#ifndef OPENGL
	int size = bufferWidth * bufferHeight;
	bufferScreen = (BYTE *) realloc(bufferScreen, sizeof(BYTE) * size);
	if (!bufferScreen)
		error("Screen", "Unable to allocate memory for screen data buffer");

	memset(bufferScreen, 0, sizeof(BYTE) * size);
#endif
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareVideoMode()
{
	DWORD iniflags = SDL_DOUBLEBUF |
	       (gvi.hw ? SDL_HWSURFACE | SDL_HWPALETTE : SDL_SWSURFACE);

#ifdef OPENGL
	iniflags = SDL_HWSURFACE | SDL_OPENGL;

	static bool firstTime = true;
	if (firstTime) {
		debug("Screen", "OpenGL subsystem initialization...");

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
		if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1))
			warning("Screen", "OpenGL double buffer not present!");
		else
			iniflags |= SDL_DOUBLEBUF;

		PixelFormat = GL_RGB, DataType = GL_UNSIGNED_BYTE;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		// The first is PowerPC-only, as it's slow on Intel Mac Minis [Andrew Collier]
		if (glExtension("GL_APPLE_packed_pixel") && glExtension("GL_EXT_bgra"))
			PixelFormat = GL_BGR_EXT;
#endif

		// Store Mac textures locally if possible for an AGP transfer boost
		// Note: do this for ATI cards only at present, as both nVidia
		// and Intel seems to suffer a performance hit
		if (glExtension("GL_APPLE_client_storage") &&
			!memcmp(glGetString(GL_RENDERER), "ATI", 3))
				glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);

		// Try for edge-clamped textures, to avoid visible seams between filtered tiles (mainly OS X)
		if (glExtension("GL_SGIS_texture_edge_clamp"))
			Clamp = GL_CLAMP_TO_EDGE;

		firstTime = false;
	}
#endif

	if (DispMode == DM_FULLSCREEN || !gvi.wm) {
		debug("Screen", "Full-screen mode: %dx%d/%dbit", gvi.w, gvi.h, gvi.depth);
		Screen = SDL_SetVideoMode(gvi.w, gvi.h, gvi.depth, SDL_FULLSCREEN | iniflags);
	}
	else {
		debug("Screen", "Windowed mode: %dx%d/%dbit", Width, Height, gvi.depth);
		Screen = SDL_SetVideoMode(Width, Height, gvi.depth, iniflags);
	}

	if (!Screen)
		error("Screen", "Unable to create screen buffer\n%s", SDL_GetError());

#ifdef OPENGL
	BlitSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,
		TextureMainWidth, TextureMainHeight, 24, SDL_DEFAULT_MASK_QUAD);

	if (!BlitSurface)
		error("Screen", "Unable to create blitting surface\n%s", SDL_GetError());

	StatusBar = SDL_CreateRGBSurface(SDL_SWSURFACE,
		TextureStatusWidth, TextureStatusHeight, 24, SDL_DEFAULT_MASK_QUAD);

	if (!StatusBar)
		error("Screen", "Unable to create status bar surface\n%s", SDL_GetError());

	SDL_Rect *r = new SDL_Rect;
	r->x = 0;
	r->y = GetMultiplier() * 2;
	r->w = BlitRectSrc->w;
	r->h = 1;

	SDL_FillRect(BlitSurface, NULL, 0);
	SDL_FillRect(StatusBar, NULL, 0);
	SDL_FillRect(StatusBar, r, SDL_MapRGB(Screen->format, 12, 12, 12));
	delete r;

	GUI->defaultSurface = SDL_CreateRGBSurfaceFrom(BlitSurface->pixels,
		BlitSurface->w, BlitSurface->h,
		BlitSurface->format->BitsPerPixel, BlitSurface->pitch,
		BlitSurface->format->Rmask, BlitSurface->format->Gmask,
		BlitSurface->format->Bmask, BlitSurface->format->Amask);
	GUI->prepareDefaultSurface(bufferWidth, bufferHeight, Palette);

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glGenTextures(2, Texture);

	glBindTexture(GL_TEXTURE_2D, Texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, Texture[1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glViewport(0, 0, Screen->w, Screen->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, Screen->w, Screen->h, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
#else
	BlitSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,
		BlitRectSrc->w, BlitRectSrc->h, 8, SDL_DEFAULT_MASK_QUAD);

	if (!BlitSurface)
		error("Screen", "Unable to create blitting surface\n%s", SDL_GetError());

	SDL_SetPalette(BlitSurface, SDL_LOGPAL | SDL_PHYSPAL, Palette, 0, 256);

	GUI->defaultSurface = SDL_CreateRGBSurfaceFrom(bufferScreen,
		bufferWidth, bufferHeight, 8, bufferWidth,
		BlitSurface->format->Rmask, BlitSurface->format->Gmask,
		BlitSurface->format->Bmask, BlitSurface->format->Amask);
	GUI->prepareDefaultSurface(bufferWidth, bufferHeight, Palette);

	PrepareStatusBar();
#endif
}
//-----------------------------------------------------------------------------
void ScreenPMD85::ReleaseVideoMode()
{
	if (GUI->defaultSurface) {
		SDL_FreeSurface(GUI->defaultSurface);
		GUI->defaultSurface = NULL;
	}
	if (BlitSurface) {
		SDL_FreeSurface(BlitSurface);
		BlitSurface = NULL;
	}
	if (Screen) {
		SDL_FreeSurface(Screen);
		Screen = NULL;
	}

#ifdef OPENGL
	if (StatusBar) {
		SDL_FreeSurface(StatusBar);
		StatusBar = NULL;
	}

	glDeleteTextures(2, Texture);
#endif
}
//-----------------------------------------------------------------------------
void ScreenPMD85::PrepareStatusBar()
{
#ifndef OPENGL
	SDL_Rect *r = new SDL_Rect(*BlitRectDest);
	DWORD borderColor = SDL_MapRGB(Screen->format, 12, 12, 12);

	int i = 0;
	if (borderSize > 0) {
		i = (BlitRectSrc->h / 256) * 2;
		r->x -= i;
		r->y -= i;
		r->w += i * 2;
		r->h += i * 2;

		SDL_FillRect(Screen, r, borderColor);

		if (i > 1) {
			r->x++;
			r->y++;
			r->w -= 2;
			r->h -= 2;
			SDL_FillRect(Screen, r, 0);
		}
	}
	else {
		r->y = Height - STATUSBAR_HEIGHT;
		r->h = 1;
		SDL_FillRect(Screen, r, borderColor);
	}

	r->y += r->h + i;
	r->h = STATUSBAR_HEIGHT;
	SDL_FillRect(Screen, r, 0);

	delete r;
#endif
}
//-----------------------------------------------------------------------------
void ScreenPMD85::RedrawStatusBar()
{
	SDL_Rect *r = new SDL_Rect(*BlitRectDest), *s = new SDL_Rect;

#ifdef OPENGL
	SDL_Surface *surface = StatusBar;

	r->w = BlitRectSrc->w;
	r->h = BlitRectSrc->h;
	r->x = r->w - (4 * STATUSBAR_SPACING);
	r->y = ((STATUSBAR_HEIGHT - STATUSBAR_ICON) / 2)
	     + (r->y / BORDER_MULTIPLIER)
	     + (BlitRectSrc->h / 256);
#else
	SDL_Surface *surface = Screen;

	r->x += r->w - (4 * STATUSBAR_SPACING);
	r->y += r->h + ((STATUSBAR_HEIGHT - STATUSBAR_ICON) / 2)
	             + (r->y / BORDER_MULTIPLIER)
	             + (BlitRectSrc->h / 256);
#endif

	r->w = r->h = s->w = s->h = STATUSBAR_ICON;

//	control LEDs on right side...
	s->y = 0;
	s->x = (ledState & 1) ? STATUSBAR_ICON : 0;
	SDL_LowerBlit(GUI->icons, s, surface, r);

	r->x += STATUSBAR_SPACING;
	s->x = (ledState & 2) ? (2 * STATUSBAR_ICON) : 0;
	SDL_LowerBlit(GUI->icons, s, surface, r);

	r->x += STATUSBAR_SPACING;
	s->x = (ledState & 4) ? (3 * STATUSBAR_ICON) : 0;
	SDL_LowerBlit(GUI->icons, s, surface, r);

//	tape/disk icon...
	r->x -= (4 * STATUSBAR_SPACING);
	if (iconState) {
		s->x = (iconState * STATUSBAR_ICON) + (3 * STATUSBAR_ICON);
		SDL_LowerBlit(GUI->icons, s, surface, r);
	}
	else
		SDL_FillRect(surface, r, 0);

	delete s;

	if (SDL_LockSurface(surface) != 0) {
		delete r;
		return;
	}

	int tapProgressWidth = r->x - STATUSBAR_SPACING;
	static char status[24] = "";
	static BYTE pauseBlinker = 0;

//	status text, cpu meter and blinking pause...
#ifdef OPENGL
	r->x = STATUSBAR_SPACING;
#else
	r->x = BlitRectDest->x + STATUSBAR_SPACING;
#endif
	r->y += 2;

	GUI->printText(surface, r->x, r->y, 0, status);
	if (statusPercentage < 0) {
		sprintf(status, "PAUSED");
		GUI->printText(surface, r->x, r->y, (pauseBlinker < 10) ? 111 : 0, status);
		if (pauseBlinker++ >= 16)
			pauseBlinker = 0;
	}
	else if (statusPercentage > 0) {
		sprintf(status, "%sFPS:%d CPU:%d%%", computerModel, statusFPS, statusPercentage);
		GUI->printText(surface, r->x, r->y, 55, status);
	}

//	tape progress bar...
	r->x = (r->x + (20 * 6) + STATUSBAR_SPACING);
	r->y += 3;
	r->w = tapProgressWidth - r->x;
	r->h = 2;

	SDL_FillRect(surface, r, *(TapeBrowser->ProgressBar->Active) ? SDL_MapRGB(surface->format, 16, 24, 16) : 0);
	if (*(TapeBrowser->ProgressBar->Active)) {
		r->w = ((double) r->w / TapeBrowser->ProgressBar->Max) * TapeBrowser->ProgressBar->Position;
		SDL_FillRect(surface, r, SDL_MapRGB(surface->format, 40, 100, 50));
	}

	SDL_UnlockSurface(surface);
	delete r;
}
//-----------------------------------------------------------------------------
void ScreenPMD85::SetScaler()
{
#ifndef OPENGL
	TDisplayMode reqDispMode = DispMode;

	if (DispMode == DM_FULLSCREEN || !gvi.wm)
		reqDispMode = FullScreenScaleMode;

	switch (reqDispMode) {
		default:
		case DM_NORMAL:
			Scaler = &point1x;
			break;

		case DM_DOUBLESIZE:
			if (LCDmode)
				Scaler = &point2xLCD;
			else {
				switch (HalfPass) {
					case HP_OFF:
						Scaler = &point2x; break;
					case HP_75:
						Scaler = &point2xHP1; break;
					case HP_50:
						Scaler = &point2xHP2; break;
					case HP_25:
						Scaler = &point2xHP3; break;
					case HP_0:
						Scaler = &point2xHP4; break;
				}
			}
			break;

		case DM_TRIPLESIZE:
			if (LCDmode)
				Scaler = &point3xLCD;
			else {
				switch (HalfPass) {
					case HP_OFF:
						Scaler = &point3x; break;
					case HP_75:
						Scaler = &point3xHP1; break;
					case HP_50:
						Scaler = &point3xHP2; break;
					case HP_25:
						Scaler = &point3xHP3; break;
					case HP_0:
						Scaler = &point3xHP4; break;
				}
			}
			break;

		case DM_QUADRUPLESIZE:
			if (LCDmode)
				Scaler = &point4xLCD;
			else {
				switch (HalfPass) {
					case HP_OFF:
						Scaler = &point4x; break;
					case HP_75:
						Scaler = &point4xHP1; break;
					case HP_50:
						Scaler = &point4xHP2; break;
					case HP_25:
						Scaler = &point4xHP3; break;
					case HP_0:
						Scaler = &point4xHP4; break;
				}
			}
			break;
	}
}

//-----------------------------------------------------------------------------
// one point is drawed as-is without HalfPass or LCD emulation
//-----------------------------------------------------------------------------
scalerMethodPrototype(point1x)
{
	while (h--) {
		memcpy(dst, src, w);
		dst += dstPitch;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
// one point is zoomed to four-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//   normal        75%          50%          25%          0%           LCD
// | E | E |    | E | E |    | E | E |    | E | E |    | E | E |    | E | D |
// | E | E |    | D | D |    | C | C |    | B | B |    | A | A |    | B | C |
//-----------------------------------------------------------------------------
scalerMethodPrototype(point2x)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 2) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point2xHP1)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 2) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + dstPitch) = c + 16;
			*(p + dstPitch + 1) = c + 16;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point2xHP2)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 2) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + dstPitch) = c + 32;
			*(p + dstPitch + 1) = c + 32;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point2xHP3)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 2) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + dstPitch) = c + 48;
			*(p + dstPitch + 1) = c + 48;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point2xHP4)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 2) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + dstPitch) = c + 64;
			*(p + dstPitch + 1) = c + 64;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point2xLCD)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 2) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c + 16;
			*(p + dstPitch) = c + 48;
			*(p + dstPitch + 1) = c + 32;
		}
		dst += dstPitch * 2;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
// one point is zoomed to nine-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//      normal             75%               50%
//  | E | E | E |     | E | E | E |     | E | E | E |
//  | E | E | E |     | E | E | E |     | D | D | D |
//  | E | E | E |     | D | D | D |     | C | C | C |
//       25%               0%                LCD
//  | E | E | E |     | E | E | E |     | E | E | D |
//  | C | C | C |     | B | B | B |     | E | D | E |
//  | B | B | B |     | A | A | A |     | A | B | A |
//-----------------------------------------------------------------------------
scalerMethodPrototype(point3x)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 3) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
			*(p + dstPitch + 2) = c;
			*(p + dstPitch + dstPitch) = c;
			*(p + dstPitch + dstPitch + 1) = c;
			*(p + dstPitch + dstPitch + 2) = c;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point3xHP1)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 3) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
			*(p + dstPitch + 2) = c;
			*(p + dstPitch + dstPitch) = c + 16;
			*(p + dstPitch + dstPitch + 1) = c + 16;
			*(p + dstPitch + dstPitch + 2) = c + 16;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point3xHP2)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 3) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + dstPitch) = c + 16;
			*(p + dstPitch + 1) = c + 16;
			*(p + dstPitch + 2) = c + 16;
			*(p + dstPitch + dstPitch) = c + 32;
			*(p + dstPitch + dstPitch + 1) = c + 32;
			*(p + dstPitch + dstPitch + 2) = c + 32;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point3xHP3)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 3) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + dstPitch) = c + 32;
			*(p + dstPitch + 1) = c + 32;
			*(p + dstPitch + 2) = c + 32;
			*(p + dstPitch + dstPitch) = c + 48;
			*(p + dstPitch + dstPitch + 1) = c + 48;
			*(p + dstPitch + dstPitch + 2) = c + 48;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point3xHP4)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 3) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + dstPitch) = c + 48;
			*(p + dstPitch + 1) = c + 48;
			*(p + dstPitch + 2) = c + 48;
			*(p + dstPitch + dstPitch) = c + 64;
			*(p + dstPitch + dstPitch + 1) = c + 64;
			*(p + dstPitch + dstPitch + 2) = c + 64;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point3xLCD)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 3) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c + 16;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c + 16;
			*(p + dstPitch + 2) = c;
			*(p + dstPitch + dstPitch) = c + 64;
			*(p + dstPitch + dstPitch + 1) = c + 48;
			*(p + dstPitch + dstPitch + 2) = c + 64;
		}
		dst += dstPitch * 3;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
// one point is zoomed to sixteen-dot square with HalfPass or LCD emulation
// E = hilited dot, D = 75%, C = 50%, B = 25%, A = 0% of bright
//       normal                  75%                   50%
//  | E | E | E | E |     | E | E | E | E |     | E | E | E | E |
//  | E | E | E | E |     | E | E | E | E |     | E | E | E | E |
//  | E | E | E | E |     | E | E | E | E |     | D | D | D | D |
//  | E | E | E | E |     | D | D | D | D |     | C | C | C | C |
//         25%                    0%                   LCD
//  | E | E | E | E |     | E | E | E | E |     | E | E | E | D |
//  | D | D | D | D |     | C | C | C | C |     | E | D | D | E |
//  | C | C | C | C |     | B | B | B | B |     | E | D | D | E |
//  | B | B | B | B |     | A | A | A | A |     | A | B | B | A |
//-----------------------------------------------------------------------------
scalerMethodPrototype(point4x)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 4) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 3) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
			*(p + dstPitch + 2) = c;
			*(p + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch) = c;
			*(p + dstPitch + dstPitch + 1) = c;
			*(p + dstPitch + dstPitch + 2) = c;
			*(p + dstPitch + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch + dstPitch) = c;
			*(p + dstPitch + dstPitch + dstPitch + 1) = c;
			*(p + dstPitch + dstPitch + dstPitch + 2) = c;
			*(p + dstPitch + dstPitch + dstPitch + 3) = c;
		}
		dst += dstPitch * 4;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point4xHP1)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 4) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 3) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
			*(p + dstPitch + 2) = c;
			*(p + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch) = c;
			*(p + dstPitch + dstPitch + 1) = c;
			*(p + dstPitch + dstPitch + 2) = c;
			*(p + dstPitch + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch + dstPitch) = c + 16;
			*(p + dstPitch + dstPitch + dstPitch + 1) = c + 16;
			*(p + dstPitch + dstPitch + dstPitch + 2) = c + 16;
			*(p + dstPitch + dstPitch + dstPitch + 3) = c + 16;
		}
		dst += dstPitch * 4;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point4xHP2)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 4) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 3) = c;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c;
			*(p + dstPitch + 2) = c;
			*(p + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch) = c + 16;
			*(p + dstPitch + dstPitch + 1) = c + 16;
			*(p + dstPitch + dstPitch + 2) = c + 16;
			*(p + dstPitch + dstPitch + 3) = c + 16;
			*(p + dstPitch + dstPitch + dstPitch) = c + 32;
			*(p + dstPitch + dstPitch + dstPitch + 1) = c + 32;
			*(p + dstPitch + dstPitch + dstPitch + 2) = c + 32;
			*(p + dstPitch + dstPitch + dstPitch + 3) = c + 32;
		}
		dst += dstPitch * 4;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point4xHP3)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 4) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 3) = c;
			*(p + dstPitch) = c + 16;
			*(p + dstPitch + 1) = c + 16;
			*(p + dstPitch + 2) = c + 16;
			*(p + dstPitch + 3) = c + 16;
			*(p + dstPitch + dstPitch) = c + 32;
			*(p + dstPitch + dstPitch + 1) = c + 32;
			*(p + dstPitch + dstPitch + 2) = c + 32;
			*(p + dstPitch + dstPitch + 3) = c + 32;
			*(p + dstPitch + dstPitch + dstPitch) = c + 48;
			*(p + dstPitch + dstPitch + dstPitch + 1) = c + 48;
			*(p + dstPitch + dstPitch + dstPitch + 2) = c + 48;
			*(p + dstPitch + dstPitch + dstPitch + 3) = c + 48;
		}
		dst += dstPitch * 4;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point4xHP4)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 4) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 3) = c;
			*(p + dstPitch) = c + 32;
			*(p + dstPitch + 1) = c + 32;
			*(p + dstPitch + 2) = c + 32;
			*(p + dstPitch + 3) = c + 32;
			*(p + dstPitch + dstPitch) = c + 48;
			*(p + dstPitch + dstPitch + 1) = c + 48;
			*(p + dstPitch + dstPitch + 2) = c + 48;
			*(p + dstPitch + dstPitch + 3) = c + 48;
			*(p + dstPitch + dstPitch + dstPitch) = c + 64;
			*(p + dstPitch + dstPitch + dstPitch + 1) = c + 64;
			*(p + dstPitch + dstPitch + dstPitch + 2) = c + 64;
			*(p + dstPitch + dstPitch + dstPitch + 3) = c + 64;
		}
		dst += dstPitch * 4;
		src += srcPitch;
	}
}
//-----------------------------------------------------------------------------
scalerMethodPrototype(point4xLCD)
{
	static int i;
	static BYTE c;

	while (h--) {
		BYTE *p = dst;
		for (i = 0; i < w; ++i, p += 4) {
			c = *(src + i);
			*(p) = c;
			*(p + 1) = c;
			*(p + 2) = c;
			*(p + 3) = c + 16;
			*(p + dstPitch) = c;
			*(p + dstPitch + 1) = c + 16;
			*(p + dstPitch + 2) = c + 16;
			*(p + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch) = c;
			*(p + dstPitch + dstPitch + 1) = c + 16;
			*(p + dstPitch + dstPitch + 2) = c + 16;
			*(p + dstPitch + dstPitch + 3) = c;
			*(p + dstPitch + dstPitch + dstPitch) = c + 64;
			*(p + dstPitch + dstPitch + dstPitch + 1) = c + 48;
			*(p + dstPitch + dstPitch + dstPitch + 2) = c + 48;
			*(p + dstPitch + dstPitch + dstPitch + 3) = c + 64;
		}
		dst += dstPitch * 4;
		src += srcPitch;
	}
#endif
}
//-----------------------------------------------------------------------------
void ScreenPMD85::RGBpalete(SDL_Color *pal)
{
	static SDL_Color stdpal[64] = {
		{   0,   0,   0, 0 },  // 0 - black (dimmed dot)
		{ 160,   0,   0, 0 },  // 1 - maroon
		{   0, 160,   0, 0 },  // 2 - green
		{ 160, 160,   0, 0 },  // 3 - olive
		{   0,   0, 160, 0 },  // 4 - navy
		{ 160,   0, 160, 0 },  // 5 - purple
		{   0, 160, 160, 0 },  // 6 - teal
		{ 160, 160, 160, 0 },  // 7 - gray
		{ 191, 191, 191, 0 },  // 8 - silver (half bright)
		{ 255,  80,  80, 0 },  // 9 - red
		{  80, 255,  80, 0 },  // 10 - lime
		{ 255, 255,  80, 0 },  // 11 - yellow
		{  80,  80, 255, 0 },  // 12 - blue
		{ 255,  80, 255, 0 },  // 13 - fuchsia
		{  80, 255, 255, 0 },  // 14 - aqua
		{ 255, 255, 255, 0 },  // 15 - white (full bright)

		{   0,   0,   0, 0 },  // }
		{ 114,   6,   6, 0 },  //  |
		{   6, 114,   6, 0 },  //  |
		{ 114, 114,   6, 0 },  //  |
		{   6,   6, 114, 0 },  //  |
		{ 114,   6, 114, 0 },  //  |
		{   6, 114, 114, 0 },  //  |
		{ 120, 120, 120, 0 },  //   } - 75% of bright
		{ 144, 144, 144, 0 },  //  |
		{ 185,  66,  66, 0 },  //  |
		{  66, 185,  66, 0 },  //  |
		{ 185, 185,  66, 0 },  //  |
		{  66,  66, 185, 0 },  //  |
		{ 185,  66, 185, 0 },  //  |
		{  80, 185, 185, 0 },  //  |
		{ 191, 191, 191, 0 },  // }

		{   0,   0,   0, 0 },  // }
		{  82,   9,   9, 0 },  //  |
		{   9,  82,   9, 0 },  //  |
		{  82,  82,   9, 0 },  //  |
		{   9,   9,  82, 0 },  //  |
		{  82,   9,  82, 0 },  //  |
		{   9,  82,  82, 0 },  //  |
		{  90,  90,  90, 0 },  //   } - 50% of bright
		{ 108, 108, 108, 0 },  //  |
		{ 134,  54,  54, 0 },  //  |
		{  54, 134,  54, 0 },  //  |
		{ 134, 134,  54, 0 },  //  |
		{  54,  54, 134, 0 },  //  |
		{ 134,  54, 134, 0 },  //  |
		{  54, 134, 134, 0 },  //  |
		{ 144, 144, 144, 0 },  // }

		{   0,   0,   0, 0 },  // }
		{  60,   8,   8, 0 },  //  |
		{   8,  60,   8, 0 },  //  |
		{  60,  60,   8, 0 },  //  |
		{   8,   8,  60, 0 },  //  |
		{  60,   8,  60, 0 },  //  |
		{   8,  60,  60, 0 },  //  |
		{  68,  68,  68, 0 },  //   } - 25% of bright
		{  81,  81,  81, 0 },  //  |
		{  98,  44,  44, 0 },  //  |
		{  44,  98,  44, 0 },  //  |
		{  98,  98,  44, 0 },  //  |
		{  44,  44,  98, 0 },  //  |
		{  98,  44,  98, 0 },  //  |
		{  44,  98,  98, 0 },  //  |
		{ 108, 108, 108, 0 }   // }
	};

	// UserInterface colors:
	static SDL_Color guipal[16] = {
		{   0,   0,   0, 0 },  // window shadow
		{ 160,  24,  12, 0 },  // window border a title background
		{ 242, 238, 233, 0 },  // window background
		{   0,   0,   0, 0 },  // foreground, text
		{ 196, 215, 245, 0 },  // highlight background
		{ 160, 160, 160, 0 },  // disabled item, inactive text
		{ 200, 200, 200, 0 },  // checkbox/radio border, separator
		{   0, 160,   0, 0 },  // checkbox/radio active symbol
		{  80,  80, 255, 0 },  // smart-key
		{   0,   0, 160, 0 },  // hotkey/directory
		{   8,  32,  64, 0 },  // debugger background
		{ 233, 238, 242, 0 },  // debugger foreground
		{  32,  64, 128, 0 },  // debugger highlight cursor
		{  96, 112, 128, 0 },  // debugger border
		{   0,   0,   0, 0 },  //
		{ 224,  27,  76, 0 }   //
	};

	memset(pal, 0, sizeof(SDL_Color) * 256);
	memcpy(pal, stdpal, sizeof(SDL_Color) * 64);
	for (int i = 80; i < 160; i += 16)
		memcpy(pal + i, guipal, sizeof(guipal));
}
//-----------------------------------------------------------------------------
