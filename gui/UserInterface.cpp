/*	UserInterface.cpp: Class for GUI rendering.
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
#include "CommonUtils.h"
#include "UserInterface.h"
#include "UserInterfaceData.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
UserInterface *GUI;
//-----------------------------------------------------------------------------
UserInterface::UserInterface()
{
	debug("GUI", "Initializing, loading resources...");

#pragma pack(push, 1)
	struct FNT_HEADER {
		WORD  version;
		DWORD filesize;
		char  copyright[60];
		WORD  type;
		WORD  pointsize;
		WORD  vertres;
		WORD  hortres;
		WORD  ascent;
		WORD  internal_leading;
		WORD  external_leading;
		BYTE  italic;
		BYTE  underline;
		BYTE  strikeout;
		WORD  weight;
		BYTE  charset;
		WORD  width;
		WORD  height;
		BYTE  pitchfamily;
		WORD  avgwidth;
		WORD  maxwidth;
		BYTE  firstchar;
		BYTE  lastchar;
		BYTE  defchar;
		BYTE  breakchar;
		WORD  widthbytes;
		DWORD deviceoffset;
		DWORD faceoffset;
		DWORD bitspointer;
		DWORD bitsoffset;
		BYTE  mbz1;
		DWORD flags;
		WORD  aspace;
		WORD  bspace;
		WORD  cspace;
		DWORD coloroffset;
	};
#pragma pack(pop)

	fontData = NULL;
	FILE *f = fopen(LocateResource("base.fnt"), "rb");
	FNT_HEADER *fnt = new FNT_HEADER;

	if (fread(fnt, sizeof(FNT_HEADER), 1, f) == 1
		&& fnt->version == 0x300 && fnt->type == 0
		&& fnt->italic == 0 && fnt->underline == 0
		&& fnt->strikeout == 0 && fnt->weight == 400
		&& fnt->pitchfamily == 0x30 && fnt->width <= 8
		&& fnt->avgwidth == fnt->width && fnt->maxwidth == fnt->width
		&& fnt->firstchar == 32 && fnt->flags == 0x11 && fnt->coloroffset == 0) {

		size_t len = fnt->height * ((fnt->lastchar - fnt->firstchar) + 1);
		fontData = new BYTE[len];
		fontWidth = fnt->width;
		fontHeight = fnt->height;
		fontLineHeight = fnt->height + 1;

		fseek(f, fnt->bitsoffset, SEEK_SET);
		if (fread(fontData, sizeof(BYTE), len, f) < len)
			warning("GUI", "Possibly corrupted font resource file");
	}

	delete fnt;
	fclose(f);

	if (fontData == NULL)
		error("GUI", "Can't load font resource file");
	debug("GUI", "Font resource loaded");

	icons = LoadImgToSurface(LocateResource("statusbar.bmp"));
	if (icons == NULL)
		error("GUI", "Can't load status bar icons resource file");
	debug("GUI", "Status bar icons resource loaded");

	defaultTexture = NULL;
	statusTexture = NULL;
	statusRect = NULL;

	cMenu_data = NULL;
	cMenu_rect = new SDL_Rect;

	fileSelector = new GUI_FILESELECTOR_DATA;
	fileSelector->dirEntries = NULL;
	fileSelector->extFilter = NULL;
	fileSelector->title = NULL;
	fileSelector->path[0] = '\0';
	fileSelector->tag = 0;
	fileSelector->callback.disconnect_all();

	tapeDialog = new GUI_TAPEDIALOG_DATA;
	tapeDialog->entries = NULL;
	tapeDialog->count = 0;
	tapeDialog->popup.rect = NULL;

	ledState = 0;
	iconState = 0;
	statusFPS = 0;
	statusPercentage = 0;
	computerModel[0] = '\0';

	uiSetChanges = 0;
	uiQueryState = GUI_QUERY_CANCEL;

	menuStackLevel = -1;
	SDL_zero(menuStack);

	needRelease = false;
}
//-----------------------------------------------------------------------------
UserInterface::~UserInterface()
{
	debug("GUI", "Uninitializing, freeing...");

	if (fontData)
		delete [] fontData;
	fontData = NULL;

	if (icons) {
		free(icons->pixels);
		delete icons;
	}
	icons = NULL;

	if (cMenu_rect)
		delete cMenu_rect;
	cMenu_rect = NULL;

	if (fileSelector) {
		ScanDir(NULL, &fileSelector->dirEntries, &fileSelector->count);
		delete fileSelector;
		fileSelector = NULL;
	}

	if (tapeDialog) {
		TapeBrowser->FreeFileList(&tapeDialog->entries, &tapeDialog->count);
		delete tapeDialog;
		tapeDialog = NULL;
	}
}
//-----------------------------------------------------------------------------
UserInterface::GUI_SURFACE *UserInterface::LoadImgToSurface(const char *file)
{
	SDL_Surface *src = SDL_LoadBMP(file);
	if (!src)
		return NULL;

	SDL_Surface *cpy = SDL_ConvertSurfaceFormat(
			SDL_CreateRGBSurface(0, src->w, src->h, 32, 0, 0, 0, 0),
			SDL_PIXELFORMAT_DEFAULT, 0);

	SDL_SetColorKey(src, SDL_TRUE, SDL_MapRGB(src->format, 255, 0, 255));
	SDL_BlitSurface(src, NULL, cpy, NULL);
	SDL_FreeSurface(src);
	SDL_LockSurface(cpy);

	GUI_SURFACE *result = new GUI_SURFACE;

	result->format = cpy->format->format;
	result->w = cpy->w;
	result->h = cpy->h;
	result->pitch = cpy->pitch;

	DWORD length = cpy->pitch * cpy->h;
	result->pixels = (BYTE *) malloc(length);
	memcpy(result->pixels, cpy->pixels, length);

	SDL_UnlockSurface(cpy);
	SDL_FreeSurface(cpy);

	return result;
}
//-----------------------------------------------------------------------------
void UserInterface::BlitToSurface(GUI_SURFACE *src, const SDL_Rect *srcRect, GUI_SURFACE *dst, const SDL_Rect *dstRect)
{
	if (dst->format != src->format)
		return;

	int bpp = dst->pitch / dst->w;
	int w = SDL_min(srcRect->w, dstRect->w) * bpp;
	int h = SDL_min(srcRect->h, dstRect->h);

	BYTE *ptr1 = dst->pixels + (dstRect->y * dst->pitch) + (dstRect->x * bpp);
	BYTE *ptr2 = src->pixels + (srcRect->y * src->pitch) + (srcRect->x * bpp);

	for (; h > 0; --h, ptr1 += dst->pitch, ptr2 += src->pitch)
		memcpy(ptr1, ptr2, w);
}
//-----------------------------------------------------------------------------
void UserInterface::InitDefaultTexture(int width, int height)
{
	defaultTexture = SDL_CreateTexture(gdc.renderer, SDL_PIXELFORMAT_DEFAULT,
			SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!defaultTexture)
		error("GUI", "Unable to create defaultTexture\n%s", SDL_GetError());

	void *pixels;
	int pitch;

	SDL_LockTexture(defaultTexture, NULL, &pixels, &pitch);

	frameWidth  = width;
	frameHeight = height;
	frameLength = pitch * height;
	maxCharsOnScreen = (frameWidth - (2 * GUI_CONST_BORDER)) / fontWidth;

	memset(pixels, 0, frameLength);
	SDL_UnlockTexture(defaultTexture);

	SDL_SetTextureBlendMode(defaultTexture, SDL_BLENDMODE_BLEND);
	SDL_SetTextureAlphaMod(defaultTexture, 242);
}
//-----------------------------------------------------------------------------
UserInterface::GUI_SURFACE *UserInterface::LockSurface(SDL_Texture *texture)
{
	GUI_SURFACE *result = new GUI_SURFACE;

	SDL_QueryTexture(texture, &result->format, NULL, &result->w, &result->h);
	SDL_LockTexture(texture, NULL, (void **) &result->pixels, &result->pitch);

	return result;
}
//-----------------------------------------------------------------------------
void UserInterface::UnlockSurface(SDL_Texture *texture, GUI_SURFACE *surface)
{
	SDL_UnlockTexture(texture);
	delete surface;
}
//-----------------------------------------------------------------------------
void UserInterface::PutPixel(GUI_SURFACE *s, int x, int y, BYTE col)
{
	if (s && s->format == SDL_PIXELFORMAT_DEFAULT && x < s->w && y < s->h)
		*(((DWORD *) (s->pixels + y * s->pitch)) + x) = globalPalette[col];
}
//-----------------------------------------------------------------------------
void UserInterface::PrintChar(GUI_SURFACE *s, int x, int y, BYTE col, BYTE ch)
{
	BYTE *b = fontData + ((ch - 32) * fontHeight);

	for (int mx, my = 0; my < fontHeight; my++)
		for (mx = 0; mx < fontWidth; mx++)
			if (b[my] & (0x80 >> mx))
				PutPixel(s, x + mx, y + my, col);
}
//-----------------------------------------------------------------------------
void UserInterface::PrintText(GUI_SURFACE *s, int x, int y, BYTE col, const char *msg)
{
	BYTE ch;
	BYTE c = col;
	for (int i = 0, mx = x; i < (signed) strlen(msg); i++) {
		ch = msg[i];

//	UTF-8 filtering {
		if (ch > 0xFD)
			ch = SCHR_ERROR;
		else if (ch >= 0xFC) {
			i += 5;
			ch = SCHR_ERROR;
		}
		else if (ch >= 0xF8) {
			i += 4;
			ch = SCHR_ERROR;
		}
		else if (ch >= 0xF0) {
			i += 3;
			ch = SCHR_ERROR;
		}
		else if (ch >= 0xE0) {
			i += 2;
			ch = SCHR_ERROR;
		}
		else if (ch >= 0xC0) {
			i++;
			ch = SCHR_ERROR;
		}
//	} UTF-8 filtering

		else if (ch == 0x20 || ch == 0xA0) {
			mx += fontWidth;
			continue;
		}
		else if (ch == '\n') {
			mx = x;
			y += fontLineHeight;
			continue;
		}
		else if (ch == '\a') {
			if (col != GUI_COLOR_DISABLED)
				c = GUI_COLOR_SMARTKEY;

			continue;
		}
		else if (ch < 0x20 && ch > SCHR_LAST)
			ch = SCHR_ERROR;

		PrintChar(s, mx, y, c, ch);

		mx += fontWidth;
		if (ch == SCHR_HOTKEY || ch == SCHR_SHIFT)
			mx += 4;

		c = col;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::PrintFormatted(GUI_SURFACE *s, int x, int y, BYTE col, const char *msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);

	PrintText(s, x, y, col, msgbuffer);
}
//-----------------------------------------------------------------------------
void UserInterface::PrintRightAlign(GUI_SURFACE *s, int x, int y, BYTE col, const char *msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);

	char *token = strtok(msgbuffer, "\n");
	while (token != NULL) {
		PrintText(s, x - (strlen(token) * fontWidth), y, col, token);
		y += fontLineHeight;

		token = strtok(NULL, "\n");
	}
}
//-----------------------------------------------------------------------------
void UserInterface::PrintTitle(GUI_SURFACE *s, int x, int y, int w, BYTE col, const char *msg)
{
	int i, len = (signed) strlen(msg),
		mx = x + ((w - (len * fontWidth)) / 2);

	for (i = 0; i < len; i++, mx += fontWidth)
		if ((BYTE) msg[i] > 0x20 && (BYTE) msg[i] < 0x80)
			PrintChar(s, mx, y, col, msg[i]);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawLineH(GUI_SURFACE *s, int x, int y, int len, BYTE col)
{
	if (s && s->format == SDL_PIXELFORMAT_DEFAULT && y < s->h) {
		void *ptr = ((DWORD *) (s->pixels + y * s->pitch)) + x;
		SDL_memset4(ptr, globalPalette[col], len);
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DrawLineV(GUI_SURFACE *s, int x, int y, int len, BYTE col)
{
	for (int i = 0; i < len; i++)
		PutPixel(s, x, y + i, col);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawRectangle(GUI_SURFACE *s, int x, int y, int w, int h, BYTE col)
{
	for (int my = 0; my < h; my++)
		DrawLineH(s, x, y + my, w, col);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawOutline(GUI_SURFACE *s, int x, int y, int w, int h, BYTE col)
{
	DrawLineH(s, x, y, w, col);
	DrawLineH(s, x, y + h - 1, w, col);
	DrawLineV(s, x, y, h, col);
	DrawLineV(s, x + w - 1, y, h, col);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawOutlineRounded(GUI_SURFACE *s, int x, int y, int w, int h, BYTE col)
{
	DrawLineH(s, x + 1, y, w - 2, col);
	DrawLineH(s, x + 1, y + h - 1, w - 2, col);
	DrawLineV(s, x, y + 1, h - 2, col);
	DrawLineV(s, x + w - 1, y + 1, h - 2, col);

	PutPixel(s, x + 1, y + h - 2, col);
	PutPixel(s, x + 1, y + 1, col);
	PutPixel(s, x + w - 2, y + 1, col);
	PutPixel(s, x + w - 2, y + h - 2, col);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDialogWithBorder(GUI_SURFACE *s, int x, int y, int w, int h)
{
	if (x > 0 && y > 0)
		DrawOutlineRounded(s, x - 1, y - 1, w + 2, h + 2, GUI_COLOR_SHADOW);

	DrawRectangle(s, x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BACKGROUND);
	DrawRectangle(s, x + 1, y + 1, w - 2, 9, GUI_COLOR_BORDER);
	DrawOutlineRounded(s, x, y, w, h, GUI_COLOR_BORDER);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugFrame(GUI_SURFACE *s, int x, int y, int w, int h)
{
	DrawRectangle(s, x, y, w - 1, h - 1, GUI_COLOR_DBG_BACK);
	DrawOutlineRounded(s, x - 1, y - 1, w + 1, h + 1, GUI_COLOR_DBG_BORDER);
}
//-----------------------------------------------------------------------------
void UserInterface::PrintCheck(GUI_SURFACE *s, int x, int y, BYTE col, BYTE ch, bool state)
{
	BYTE col2 = (menuStack[menuStackLevel].type != GUI_TYPE_DEBUGGER) ?
			GUI_COLOR_SEPARATOR : GUI_COLOR_DBG_BORDER;

	if (ch == SCHR_RADIO)
		DrawOutlineRounded(s, x - 1, y - 1, 8, 8, col2);
	else
		DrawOutline(s, x - 1, y - 1, 8, 8, col2);

	if (state)
		PrintChar(s, x, y - 1, col, ch);
}
//-----------------------------------------------------------------------------
void UserInterface::MenuOpen(GUI_MENU_TYPE type, void *data)
{
	if (data == NULL) {
		switch (type) {
			case GUI_TYPE_MENU:
				data = gui_main_menu;
				break;

			case GUI_TYPE_FILESELECTOR:
			case GUI_TYPE_TAPEBROWSER:
			case GUI_TYPE_DEBUGGER:
				break;

			case GUI_TYPE_TAPE_POPUP:
				if (menuStack[menuStackLevel].type != GUI_TYPE_TAPEBROWSER)
					return;
				data = gui_tapebrowser_popup;
				break;

			default:
				return;
		}
	}

	needRelease = true;
	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	if (menuStackLevel < 0) {
		uiSetChanges = 0;
		memset(defaultSurface->pixels, 0, frameLength);
	}
	else {
		menuStack[menuStackLevel].frame = new BYTE[frameLength];
		memcpy(menuStack[menuStackLevel].frame, defaultSurface->pixels, frameLength);

		if (type == GUI_TYPE_TAPE_POPUP) {
			if (tapeDialog->popup.rect)
				delete tapeDialog->popup.rect;

			tapeDialog->popup.rect = new SDL_Rect(*cMenu_rect);
			tapeDialog->popup.count = cMenu_count;
			tapeDialog->popup.hilite = cMenu_hilite;
			tapeDialog->popup.leftMargin = cMenu_leftMargin;
		}
		else
			menuStack[menuStackLevel].hilite = cMenu_hilite;
	}

	UnlockSurface(defaultTexture, defaultSurface);

	menuStackLevel++;
	menuStack[menuStackLevel].type = type;
	menuStack[menuStackLevel].data = data;
	menuStack[menuStackLevel].hilite = cMenu_hilite = 0;
	menuStack[menuStackLevel].frame = NULL;

	switch (type) {
		case GUI_TYPE_MENU:
		case GUI_TYPE_TAPE_POPUP:
			DrawMenu(data);
			break;

		case GUI_TYPE_FILESELECTOR:
			fileSelector->search[0] = '\0';
			DrawFileSelector();
			break;

		case GUI_TYPE_TAPEBROWSER:
			DrawTapeDialog();
			break;

		case GUI_TYPE_DEBUGGER:
			DrawDebugWindow();
			break;

		default:
			menuStack[menuStackLevel].data = NULL;
			menuStackLevel--;
			break;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::MenuClose()
{
	needRelease = true;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	menuStack[menuStackLevel].data = NULL;
	menuStackLevel--;
	if (menuStackLevel >= 0) {
		memcpy(defaultSurface->pixels, menuStack[menuStackLevel].frame, frameLength);
		UnlockSurface(defaultTexture, defaultSurface);

		delete [] menuStack[menuStackLevel].frame;
		menuStack[menuStackLevel].frame = NULL;

		if (menuStack[menuStackLevel + 1].type == GUI_TYPE_TAPE_POPUP) {
			cMenu_rect->x = tapeDialog->popup.rect->x;
			cMenu_rect->y = tapeDialog->popup.rect->y;
			cMenu_rect->w = tapeDialog->popup.rect->w;
			cMenu_rect->h = tapeDialog->popup.rect->h;
			cMenu_count = tapeDialog->popup.count;
			cMenu_hilite = tapeDialog->popup.hilite;
			cMenu_leftMargin = tapeDialog->popup.leftMargin;

			if (tapeDialog->popup.rect)
				delete tapeDialog->popup.rect;

			tapeDialog->popup.rect = NULL;
			tapeDialog->popup.count = -1;
			tapeDialog->popup.hilite = -1;
			tapeDialog->popup.leftMargin = -1;

			needRelease = true;
			return;
		}
		else
			cMenu_hilite = menuStack[menuStackLevel].hilite;

		switch (menuStack[menuStackLevel].type) {
			case GUI_TYPE_MENU:
				DrawMenu(menuStack[menuStackLevel].data);
				break;

			case GUI_TYPE_TAPEBROWSER:
				DrawTapeDialog();
				break;

			default :
				break;
		}
	}
	else {
		memset(defaultSurface->pixels, 0, frameLength);
		UnlockSurface(defaultTexture, defaultSurface);
	}
}
//-----------------------------------------------------------------------------
void UserInterface::MenuCloseAll()
{
	needRelease = true;
	SDL_Delay(GUI_CONST_KEY_REPEAT);

	for (int i = menuStackLevel; i >= 0; i--) {
		if (menuStack[i].type == GUI_TYPE_TAPE_POPUP) {
			if (tapeDialog->popup.rect)
				delete tapeDialog->popup.rect;

			tapeDialog->popup.rect = NULL;
			tapeDialog->popup.count = -1;
			tapeDialog->popup.hilite = -1;
			tapeDialog->popup.leftMargin = -1;
		}

		if (menuStack[i].frame)
			delete [] menuStack[i].frame;
		menuStack[i].frame = NULL;
		menuStack[i].data = NULL;
	}

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);
	memset(defaultSurface->pixels, 0, frameLength);
	UnlockSurface(defaultTexture, defaultSurface);

	menuStackLevel = -1;
}
//-----------------------------------------------------------------------------
void UserInterface::MenuHandleKey(WORD key)
{
	if (menuStackLevel >= 0) {
		switch (menuStack[menuStackLevel].type) {
			case GUI_TYPE_MENU:
			case GUI_TYPE_TAPE_POPUP:
				KeyhandlerMenu(key);
				break;

			case GUI_TYPE_FILESELECTOR:
				KeyhandlerFileSelector(key);
				break;

			case GUI_TYPE_TAPEBROWSER:
				KeyhandlerTapeDialog(key);
				break;

			case GUI_TYPE_DEBUGGER:
				KeyhandlerDebugWindow(key);
				break;

			default:
				break;
		}
	}
}
//-----------------------------------------------------------------------------
