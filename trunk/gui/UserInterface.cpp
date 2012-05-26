/*	UserInterface.cpp: Class for GUI rendering.
	Copyright (c) 2011-2012 Martin Borik <mborik@users.sourceforge.net>

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
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
UserInterface *GUI;
//-----------------------------------------------------------------------------
UserInterface::UserInterface()
{
	debug("GUI", "Initializing");

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
	FILE *f = fopen(LocateResource("base.fnt", false), "rb");
	FNT_HEADER *fnt = new FNT_HEADER;

	fread(fnt, sizeof(FNT_HEADER), 1, f);
	if (fnt->version == 0x300 && fnt->type == 0
	 && fnt->italic == 0 && fnt->underline == 0
	 && fnt->strikeout == 0 && fnt->weight == 400
	 && fnt->pitchfamily == 0x30 && fnt->width <= 8
	 && fnt->avgwidth == fnt->width && fnt->maxwidth == fnt->width
	 && fnt->firstchar == 32 && fnt->flags == 0x11 && fnt->coloroffset == 0) {

		int len = fnt->height * ((fnt->lastchar - fnt->firstchar) + 1);
		fontData = new BYTE[len];
		fontWidth = fnt->width;
		fontHeight = fnt->height;
		fontLineHeight = fnt->height + 1;

		fseek(f, fnt->bitsoffset, SEEK_SET);
		fread(fontData, sizeof(BYTE), len, f);
	}

	delete fnt;
	fclose(f);

	if (fontData == NULL)
		error("GUI", "Can't load font resource file");

	debug("GUI", "Font loaded");

	frameSave = NULL;
	globalPalette = NULL;

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
	tapeDialog->popup.frame = NULL;
	tapeDialog->popup.rect = NULL;

	uiSetChanges = 0;
	uiQueryState = GUI_QUERY_CANCEL;

	menuStackLevel = -1;
	needRelease = false;
	needRedraw = false;
}
//-----------------------------------------------------------------------------
UserInterface::~UserInterface()
{
	debug("GUI", "Uninitializing, freeing...");

	if (frameSave)
		free(frameSave);
	frameSave = NULL;

	if (fontData)
		delete [] fontData;
	fontData = NULL;

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
void UserInterface::prepareDefaultSurface(int width, int height, SDL_Color *palette)
{
	frameLength = defaultSurface->pitch * defaultSurface->h;
	frameSave = (BYTE *) realloc(frameSave, sizeof(BYTE) * frameLength);

	defaultSurface->locked = 1;
	defaultSurface->clip_rect.x = defaultSurface->clip_rect.y = 0;
	defaultSurface->clip_rect.w = frameWidth  = width;
	defaultSurface->clip_rect.h = frameHeight = height;

	globalPalette = (palette) ? palette : defaultSurface->format->palette->colors;
	maxCharsOnScreen = (frameWidth - (2 * GUI_CONST_BORDER)) / fontWidth;
}
//-----------------------------------------------------------------------------
void UserInterface::putPixel(SDL_Surface *s, int x, int y, BYTE col)
{
	SDL_PixelFormat *f = s->format;
	SDL_Color c = globalPalette[col];
	BYTE *d = ((BYTE *) s->pixels)
			+ (x * f->BytesPerPixel)
			+ (y * s->pitch);

	if (f->BytesPerPixel == 1)
		*d = col;
	else {
		DWORD b = SDL_MapRGB(f, c.r, c.g, c.b);
		if (f->BytesPerPixel == 2) {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			*(d++) = (b >> 8) & 0xFF;
			*(d++) = b & 0xFF;
#else
			*(d++) = b & 0xFF;
			*(d++) = (b >> 8) & 0xFF;
#endif
		}
		else {
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			*(d++) = (b >> 16) & 0xFF;
			*(d++) = (b >> 8) & 0xFF;
			*(d++) = b & 0xFF;
#else
			*(d++) = b & 0xFF;
			*(d++) = (b >> 8) & 0xFF;
			*(d++) = (b >> 16) & 0xFF;
#endif
		}
	}
}
//-----------------------------------------------------------------------------
void UserInterface::printChar(SDL_Surface *s, int x, int y, BYTE col, BYTE ch)
{
	BYTE *b = fontData + ((ch - 32) * fontHeight);

	if (s == NULL)
		s = defaultSurface;

	if (!s->locked) {
		needRedraw = true;
		return;
	}

	for (int mx, my = 0; my < fontHeight; my++)
		for (mx = 0; mx < fontWidth; mx++)
			if (b[my] & (0x80 >> mx))
				putPixel(s, x + mx, y + my, col);
}
//-----------------------------------------------------------------------------
void UserInterface::printText(SDL_Surface *s, int x, int y, BYTE col, const char *msg)
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

		printChar(s, mx, y, c, ch);

		mx += fontWidth;
		if (ch == SCHR_HOTKEY || ch == SCHR_SHIFT)
			mx += 4;

		c = col;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::printFormatted(SDL_Surface *s, int x, int y, BYTE col, const char *msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);

	printText(s, x, y, col, msgbuffer);
}
//-----------------------------------------------------------------------------
void UserInterface::printRightAlign(SDL_Surface *s, int x, int y, BYTE col, const char *msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);

	char *token = strtok(msgbuffer, "\n");
	while (token != NULL) {
		printText(s, x - (strlen(token) * fontWidth), y, col, token);
		y += fontLineHeight;

		token = strtok(NULL, "\n");
	}
}
//-----------------------------------------------------------------------------
void UserInterface::printTitle(SDL_Surface *s, int x, int y, int w, BYTE col, const char *msg)
{
	int i, len = (signed) strlen(msg),
		mx = x + ((w - (len * fontWidth)) / 2);

	for (i = 0; i < len; i++, mx += fontWidth)
		if ((BYTE) msg[i] > 0x20 && (BYTE) msg[i] < 0x80)
			printChar(s, mx, y, col, msg[i]);
}
//-----------------------------------------------------------------------------
void UserInterface::drawRectangle(SDL_Surface *s, int x, int y, int w, int h, BYTE col)
{
	if (s == NULL)
		s = defaultSurface;

	if (!s->locked) {
		needRedraw = true;
		return;
	}

	for (int mx, my = 0; my < h; my++)
		for (mx = 0; mx < w; mx++)
			putPixel(s, x + mx, y + my, col);
}
//-----------------------------------------------------------------------------
void UserInterface::drawLineH(SDL_Surface *s, int x, int y, int len, BYTE col)
{
	if (s == NULL)
		s = defaultSurface;

	if (!s->locked) {
		needRedraw = true;
		return;
	}

	for (int i = 0; i < len; i++)
		putPixel(s, x + i, y, col);
}
//-----------------------------------------------------------------------------
void UserInterface::drawLineV(SDL_Surface *s, int x, int y, int len, BYTE col)
{
	if (s == NULL)
		s = defaultSurface;

	if (!s->locked) {
		needRedraw = true;
		return;
	}

	for (int i = 0; i < len; i++)
		putPixel(s, x, y + i, col);
}
//-----------------------------------------------------------------------------
void UserInterface::drawOutline(SDL_Surface *s, int x, int y, int w, int h, BYTE col)
{
	drawLineH(s, x, y, w, col);
	drawLineH(s, x, y + h - 1, w, col);
	drawLineV(s, x, y, h, col);
	drawLineV(s, x + w - 1, y, h, col);
}
//-----------------------------------------------------------------------------
void UserInterface::drawOutlineRounded(SDL_Surface *s, int x, int y, int w, int h, BYTE col)
{
	if (s == NULL)
		s = defaultSurface;

	if (!s->locked) {
		needRedraw = true;
		return;
	}

	drawLineH(s, x + 1, y, w - 2, col);
	drawLineH(s, x + 1, y + h - 1, w - 2, col);
	drawLineV(s, x, y + 1, h - 2, col);
	drawLineV(s, x + w - 1, y + 1, h - 2, col);

	putPixel(s, x + 1, y + h - 2, col);
	putPixel(s, x + 1, y + 1, col);
	putPixel(s, x + w - 2, y + 1, col);
	putPixel(s, x + w - 2, y + h - 2, col);
}
//-----------------------------------------------------------------------------
void UserInterface::drawDialogWithBorder(SDL_Surface *s, int x, int y, int w, int h)
{
	if (x > 0 && y > 0)
		drawOutlineRounded(s, x - 1, y - 1, w + 2, h + 2, GUI_COLOR_SHADOW);

	drawRectangle(s, x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BACKGROUND);
	drawRectangle(s, x + 1, y + 1, w - 2, 9, GUI_COLOR_BORDER);
	drawOutlineRounded(s, x, y, w, h, GUI_COLOR_BORDER);
}
//-----------------------------------------------------------------------------
void UserInterface::drawDebugFrame(SDL_Surface *s, int x, int y, int w, int h)
{
	drawRectangle(s, x, y, w - 1, h - 1, GUI_COLOR_DBG_BACK);
	drawOutlineRounded(s, x - 1, y - 1, w + 1, h + 1, GUI_COLOR_DBG_BORDER);
}
//-----------------------------------------------------------------------------
void UserInterface::printCheck(SDL_Surface *s, int x, int y, BYTE col, BYTE ch, bool state)
{
	BYTE col2 = (menuStack[menuStackLevel].type != GUI_TYPE_DEBUGGER) ?
			GUI_COLOR_SEPARATOR : GUI_COLOR_DBG_BORDER;

	if (ch == SCHR_RADIO)
		drawOutlineRounded(s, x - 1, y - 1, 8, 8, col2);
	else
		drawOutline(s, x - 1, y - 1, 8, 8, col2);

	if (state)
		printChar(s, x, y - 1, col, ch);
}
//-----------------------------------------------------------------------------
void UserInterface::menuOpen(GUI_MENU_TYPE type, void *data)
{
	if (data == NULL) {
		switch (type) {
			case GUI_TYPE_MENU:
				data = gui_main_menu;
				break;

			case GUI_TYPE_SELECT:
				type = GUI_TYPE_MENU;
				data = gui_machine_menu;
				break;

			case GUI_TYPE_PERIPHERALS:
				type = GUI_TYPE_MENU;
				data = gui_pers_menu;
				break;

			case GUI_TYPE_MEMORY:
				type = GUI_TYPE_MENU;
				data = gui_mem_menu;
				break;

			case GUI_TYPE_DISKIMAGES:
				type = GUI_TYPE_MENU;
				data = gui_p32_images_menu;
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

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, GUI_CONST_KEY_REPEAT);

	if (menuStackLevel < 0) {
		uiSetChanges = 0;
		memcpy(frameSave, defaultSurface->pixels, frameLength);
	}
	else if (type == GUI_TYPE_TAPE_POPUP) {
		if (tapeDialog->popup.frame)
			delete [] tapeDialog->popup.frame;

		tapeDialog->popup.frame = new BYTE[frameLength];
		memcpy(tapeDialog->popup.frame, defaultSurface->pixels, frameLength);

		if (tapeDialog->popup.rect)
			delete tapeDialog->popup.rect;

		tapeDialog->popup.rect = new SDL_Rect(*cMenu_rect);
		tapeDialog->popup.count = cMenu_count;
		tapeDialog->popup.hilite = cMenu_hilite;
		tapeDialog->popup.leftMargin = cMenu_leftMargin;
	}
	else {
		menuStack[menuStackLevel].hilite = cMenu_hilite;
		memcpy(defaultSurface->pixels, frameSave, frameLength);
	}

	menuStackLevel++;
	menuStack[menuStackLevel].type = type;
	menuStack[menuStackLevel].data = data;
	menuStack[menuStackLevel].hilite = cMenu_hilite = 0;

	switch (type) {
		case GUI_TYPE_MENU:
		case GUI_TYPE_TAPE_POPUP:
			drawMenu(data);
			break;

		case GUI_TYPE_FILESELECTOR:
			fileSelector->search[0] = '\0';
			drawFileSelector();
			break;

		case GUI_TYPE_TAPEBROWSER:
			drawTapeDialog();
			break;

		case GUI_TYPE_DEBUGGER:
			drawDebugWindow();
			break;

		default:
			menuStack[menuStackLevel].data = NULL;
			menuStackLevel--;
			break;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::menuClose()
{
	menuStack[menuStackLevel].data = NULL;

	menuStackLevel--;
	if (menuStackLevel >= 0) {
		if (menuStack[menuStackLevel + 1].type == GUI_TYPE_TAPE_POPUP) {
			memcpy(defaultSurface->pixels, tapeDialog->popup.frame, frameLength);

			cMenu_rect->x = tapeDialog->popup.rect->x;
			cMenu_rect->y = tapeDialog->popup.rect->y;
			cMenu_rect->w = tapeDialog->popup.rect->w;
			cMenu_rect->h = tapeDialog->popup.rect->h;
			cMenu_count = tapeDialog->popup.count;
			cMenu_hilite = tapeDialog->popup.hilite;
			cMenu_leftMargin = tapeDialog->popup.leftMargin;

			if (tapeDialog->popup.rect)
				delete tapeDialog->popup.rect;
			if (tapeDialog->popup.frame)
				delete [] tapeDialog->popup.frame;

			tapeDialog->popup.frame = NULL;
			tapeDialog->popup.rect = NULL;
			tapeDialog->popup.count = -1;
			tapeDialog->popup.hilite = -1;
			tapeDialog->popup.leftMargin = -1;

			needRelease = true;
			needRedraw = true;
			return;
		}
		else {
			cMenu_hilite = menuStack[menuStackLevel].hilite;
			memcpy(defaultSurface->pixels, frameSave, frameLength);
		}

		switch (menuStack[menuStackLevel].type) {
			case GUI_TYPE_MENU:
				drawMenu(menuStack[menuStackLevel].data);
				break;

			case GUI_TYPE_TAPEBROWSER:
				drawTapeDialog();
				break;

			default :
				break;
		}
	}
}
//-----------------------------------------------------------------------------
void UserInterface::menuCloseAll()
{
	for (int i = menuStackLevel; i >= 0; i--) {
		if (menuStack[i].type == GUI_TYPE_TAPE_POPUP) {
			if (tapeDialog->popup.rect)
				delete tapeDialog->popup.rect;
			if (tapeDialog->popup.frame)
				delete [] tapeDialog->popup.frame;

			tapeDialog->popup.frame = NULL;
			tapeDialog->popup.rect = NULL;
			tapeDialog->popup.count = -1;
			tapeDialog->popup.hilite = -1;
			tapeDialog->popup.leftMargin = -1;
		}

		menuStack[i].data = NULL;
	}

	menuStackLevel = -1;
	memcpy(defaultSurface->pixels, frameSave, frameLength);
}
//-----------------------------------------------------------------------------
void UserInterface::menuHandleKey(WORD key)
{
	if (menuStackLevel >= 0) {
		switch (menuStack[menuStackLevel].type) {
			case GUI_TYPE_MENU:
			case GUI_TYPE_TAPE_POPUP:
				keyhandlerMenu(key);
				break;

			case GUI_TYPE_FILESELECTOR:
				keyhandlerFileSelector(key);
				break;

			case GUI_TYPE_TAPEBROWSER:
				keyhandlerTapeDialog(key);
				break;

			case GUI_TYPE_DEBUGGER:
				keyhandlerDebugWindow(key);
				break;

			default:
				break;
		}
	}
}
//-----------------------------------------------------------------------------
