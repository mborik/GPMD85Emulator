/*	UserInterface.cpp: Class for GUI rendering.
	Copyright (c) 2011 Martin Borik <mborik@users.sourceforge.net>

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
#include "Settings.h"
//-----------------------------------------------------------------------------
UserInterface::UserInterface()
{
	debug("[GUI] Initializing");

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
		error("Can't load font resource file");

	debug("[GUI] Font loaded");

	frameSave = NULL;
	cMenu_data = NULL;
	cMenu_rect = new SDL_Rect;

	fileSelector = new GUI_FILESELECTOR_DATA;
	fileSelector->dirEntries = NULL;
	fileSelector->extFilter = NULL;
	fileSelector->title = NULL;
	fileSelector->path[0] = '\0';
	fileSelector->tag = 0;
	fileSelector->callback.disconnect_all();

	menuStackLevel = -1;
	needRelease = false;
	needRedraw = false;
}
//-----------------------------------------------------------------------------
UserInterface::~UserInterface()
{
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
		if (menuStack[menuStackLevel].type == GUI_TYPE_FILESELECTOR)
			ScanDir(NULL, &fileSelector->dirEntries, &cMenu_count, uiSet->showHiddenFiles);

		delete fileSelector;
	}
	fileSelector = NULL;
}
//-----------------------------------------------------------------------------
void UserInterface::prepareDefaultSurface(int width, int height)
{
	frameLength = width * height;
	frameSave = (BYTE *) realloc(frameSave, sizeof(BYTE) * frameLength);

	defaultSurface->offset = 0;
	defaultSurface->locked = 1;
	defaultSurface->clip_rect.x = defaultSurface->clip_rect.y = 0;
	defaultSurface->clip_rect.w = defaultSurface->w = defaultSurface->pitch = width;
	defaultSurface->clip_rect.h = defaultSurface->h = height;
	defaultSurface->flags = SDL_SWSURFACE | SDL_PREALLOC;

	maxCharsOnScreen = (width - (2 * GUI_CONST_BORDER)) / fontWidth;
}
//-----------------------------------------------------------------------------
TSettings *UserInterface::uiSet = NULL;
void *UserInterface::uiFrm = NULL;
sigslot::signal0<> UserInterface::uiCallback;
BYTE UserInterface::uiSetChanges = 0;
BYTE UserInterface::uiQueryState = GUI_QUERY_CANCEL;
//-----------------------------------------------------------------------------
void UserInterface::putPixel(SDL_Surface *s, int x, int y, DWORD col)
{
	BYTE bps = s->format->BytesPerPixel, *d = (BYTE *) s->pixels;
	if (bps == 4)
		*((DWORD *) (d + (x * bps) + (y * s->pitch))) = col;
	else if (bps == 3)
		*((DWORD *) (d + (x * bps) + (y * s->pitch))) = (*((DWORD *) (d + (x * bps) + (y * s->pitch))) & 0xff000000) | (col & 0xffffff);
	else if (bps == 2)
		*((WORD *) (d + (x * bps) + (y * s->pitch))) = col;
	else
		*(d + x + (y * s->pitch)) = col;
}
//-----------------------------------------------------------------------------
void UserInterface::printChar(SDL_Surface *s, int x, int y, DWORD col, BYTE ch)
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
void UserInterface::printText(SDL_Surface *s, int x, int y, DWORD col, const char *msg)
{
	BYTE ch;
	DWORD c = col;
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
void UserInterface::printFormatted(SDL_Surface *s, int x, int y, DWORD col, const char *msg, ...)
{
	va_list va;
	va_start(va, msg);
	vsprintf(msgbuffer, msg, va);
	va_end(va);

	printText(s, x, y, col, msgbuffer);
}
//-----------------------------------------------------------------------------
void UserInterface::printRightAlign(SDL_Surface *s, int x, int y, DWORD col, const char *msg, ...)
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
void UserInterface::printTitle(SDL_Surface *s, int x, int y, int w, DWORD col, const char *msg)
{
	int i, len = (signed) strlen(msg),
		mx = x + ((w - (len * fontWidth)) / 2);

	for (i = 0; i < len; i++, mx += fontWidth)
		if (msg[i] > 0x20 && msg[i] < 0x80)
			printChar(s, mx, y, col, msg[i]);
}
//-----------------------------------------------------------------------------
void UserInterface::drawRectangle(SDL_Surface *s, int x, int y, int w, int h, DWORD col)
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
void UserInterface::drawLineH(SDL_Surface *s, int x, int y, int len, DWORD col)
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
void UserInterface::drawLineV(SDL_Surface *s, int x, int y, int len, DWORD col)
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
void UserInterface::drawOutline(SDL_Surface *s, int x, int y, int w, int h, DWORD col)
{
	drawLineH(s, x, y, w, col);
	drawLineH(s, x, y + h - 1, w, col);
	drawLineV(s, x, y, h, col);
	drawLineV(s, x + w - 1, y, h, col);
}
//-----------------------------------------------------------------------------
void UserInterface::drawOutlineRounded(SDL_Surface *s, int x, int y, int w, int h, DWORD col)
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
void UserInterface::printCheck(SDL_Surface *s, int x, int y, DWORD col, BYTE ch, bool state)
{
	if (ch == SCHR_RADIO)
		drawOutlineRounded(s, x - 1, y - 1, 8, 8, GUI_COLOR_SEPARATOR);
	else
		drawOutline(s, x - 1, y - 1, 8, 8, GUI_COLOR_SEPARATOR);

	if (state)
		printChar(s, x, y - 1, col, ch);
}
//-----------------------------------------------------------------------------
void UserInterface::drawMenuItems()
{
	GUI_MENU_ENTRY *ptr;
	SDL_Rect *r = new SDL_Rect(*cMenu_rect);
	const char *wrk;

	r->x++;
	r->y += (2 * GUI_CONST_BORDER);
	r->w -= 2;

	for (int mx, my, i = 0; i < cMenu_count; i++) {
		ptr = &cMenu_data[i + 1];
		my = r->y + 2;

		if (ptr->type == MI_SEPARATOR) {
			drawLineH(defaultSurface, r->x + 2, my, r->w - 4, GUI_COLOR_SEPARATOR);
			r->y += GUI_CONST_SEPARATOR;
		}
		else {
			mx = r->x + 7;

			drawRectangle(defaultSurface, r->x, r->y, r->w, GUI_CONST_ITEM_SIZE,
				(i == cMenu_hilite) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);
			printFormatted(defaultSurface, mx + cMenu_leftMargin, my,
				(ptr->enabled) ? GUI_COLOR_FOREGROUND : GUI_COLOR_DISABLED,
				"%s%s", ptr->text, ((ptr->type == MI_SUBMENU) ? " \200" :
					((ptr->type == MI_DIALOG) ? "\205" : "")));

			if (ptr->type == MI_CHECKBOX && ptr->action >= 0x8000) {
				printCheck(defaultSurface, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_HOTKEY : GUI_COLOR_DISABLED),
					SCHR_LOCKER, ptr->state);
			}
			else if (ptr->type == MI_CHECKBOX) {
				printCheck(defaultSurface, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_CHECKED : GUI_COLOR_DISABLED),
					SCHR_CHECK, ptr->state);
			}
			else if (ptr->type == MI_RADIO) {
				printCheck(defaultSurface, mx, my + 1,
					((ptr->enabled) ? GUI_COLOR_CHECKED : GUI_COLOR_DISABLED),
					SCHR_RADIO, ptr->state);
			}

			mx += r->w - 12;
			if ((wrk = ptr->hotkey) != NULL) {
				BYTE c = (ptr->enabled) ? GUI_COLOR_HOTKEY : GUI_COLOR_DISABLED;

				mx -= GUI_CONST_HOTKEYCHAR + (strlen(wrk) * 6);
				if (wrk[0] == '^') {
					wrk++;
					mx += GUI_CONST_HOTKEYCHAR - (GUI_CONST_HOTKEYCHAR - 6);

					printChar(defaultSurface, mx - GUI_CONST_HOTKEYCHAR, my, c, SCHR_HOTKEY);
					printChar(defaultSurface, mx, my, c, SCHR_SHIFT);
				}
				else
					printChar(defaultSurface, mx, my, c, SCHR_HOTKEY);

				printText(defaultSurface, mx + GUI_CONST_HOTKEYCHAR, my, c, wrk);
			}
			else if (ptr->detail) {
				wrk = ptr->detail(ptr);
				if (wrk) {
					mx -= (strlen(wrk) * 6) + 12;
					printFormatted(defaultSurface, mx, my, GUI_COLOR_DISABLED, "[%s]", wrk);
				}
			}

			r->y += GUI_CONST_ITEM_SIZE;
		}
	}

	delete r;
	needRedraw = true;
}
//-----------------------------------------------------------------------------
void UserInterface::drawMenu(void *data)
{
	GUI_MENU_ENTRY *ptr;
	int widthLeft = 0, widthRight = 0, height = 0, i, k;
	const char *wrk;

	cMenu_leftMargin = 0;
	cMenu_data = (GUI_MENU_ENTRY *) data;

	for (i = 0, ptr = &cMenu_data[1]; ptr->type != MENU_END; i++, ptr++) {
		if (i == cMenu_hilite && (ptr->type == MI_SEPARATOR || ptr->type == MI_FIXED))
			cMenu_hilite++;

		if (ptr->type == MI_SEPARATOR) {
			height += GUI_CONST_SEPARATOR;
			continue;
		}
		else if (ptr->type == MI_CHECKBOX || ptr->type == MI_RADIO)
			cMenu_leftMargin = GUI_CONST_CHK_MARGIN;

		height += GUI_CONST_ITEM_SIZE;

		k = (strlen(ptr->text) * fontWidth)
		  + ((ptr->type == MI_SUBMENU) ? (fontWidth + fontWidth) :
		  + ((ptr->type == MI_DIALOG)  ? fontWidth : 0));

		if (widthLeft < k)
			widthLeft = k;

		k = 0;
		if ((wrk = ptr->hotkey) != NULL) {
			if (wrk[0] == '^') {
				k += GUI_CONST_HOTKEYCHAR;
				wrk++;
			}

			k += (strlen(wrk) * fontWidth) + GUI_CONST_HOTKEYCHAR;
		}

		if (ptr->detail) {
			wrk = ptr->detail(ptr);
			if (wrk)
				k += ((strlen(wrk) + 2) * fontWidth);
		}

		if (widthRight < k)
			widthRight = k;
	}

	cMenu_count = (ptr - &cMenu_data[1]);

	k = cMenu_leftMargin + widthLeft + fontWidth + widthRight;
	if ((strlen(cMenu_data->text) * fontWidth) > k)
		k = strlen(cMenu_data->text) * fontWidth;

	cMenu_rect->w = GUI_CONST_BORDER + k + GUI_CONST_BORDER;
	cMenu_rect->h = (2 * GUI_CONST_BORDER) + height + GUI_CONST_BORDER;
	cMenu_rect->x = (defaultSurface->w - cMenu_rect->w) / 2;
	cMenu_rect->y = (defaultSurface->h - cMenu_rect->h) / 2;

	drawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y, cMenu_rect->w, cMenu_rect->h);
	printTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1, cMenu_rect->w, GUI_COLOR_BACKGROUND, cMenu_data->text);
	drawMenuItems();
}
//-----------------------------------------------------------------------------
void UserInterface::drawFileSelectorItems()
{
	int i, x, y, c, halfpage = (fileSelector->itemsOnPage / 2),
		itemCharWidth = (maxCharsOnScreen / 2) - 1,
		itemPixelWidth = (itemCharWidth * fontWidth) + GUI_CONST_HOTKEYCHAR - 1;

	SDL_Rect *r = new SDL_Rect(*cMenu_rect);
	char ptr[32], *wrk;

	r->x += GUI_CONST_BORDER;
	r->y += (3 * GUI_CONST_BORDER) + 2;
	r->w -= (2 * GUI_CONST_BORDER);

	printChar(defaultSurface, r->x + r->w, r->y + 1, (cMenu_leftMargin > 0)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_UP);

	for (i = cMenu_leftMargin; i < (cMenu_leftMargin + fileSelector->itemsOnPage); i++) {
		x = r->x + (((i - cMenu_leftMargin) >= halfpage) ? itemPixelWidth : 0);
		y = r->y + (((i - cMenu_leftMargin) % halfpage) * GUI_CONST_ITEM_SIZE);

		drawRectangle(defaultSurface, x - 2, y, itemPixelWidth, GUI_CONST_ITEM_SIZE,
			(i == cMenu_hilite) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);

		if (i >= cMenu_count)
			continue;

		strncpy(ptr, fileSelector->dirEntries[i], 31);
		if (ptr[0] == DIR_DELIMITER) {
			c = GUI_COLOR_HOTKEY;
			printChar(defaultSurface, x, y + 2, c, SCHR_DIRECTORY);
		}
		else if (ptr[0] == '\xA0') {
			if (fileSelector->extFilter) {
				c = GUI_COLOR_DISABLED;
				wrk = strrchr(ptr, '.');
				if (wrk) {
					wrk++;
					for (int j = 0; fileSelector->extFilter[j] != NULL; j++) {
						if (strcasecmp(wrk, fileSelector->extFilter[j]) == 0) {
							c = GUI_COLOR_FOREGROUND;
							break;
						}
					}
				}
			}
			else
				c = GUI_COLOR_FOREGROUND;
		}
		else {
			c = GUI_COLOR_HOTKEY;
			ptr[1] = SCHR_BROWSE;
			ptr[2] = '\0';
		}

		if (strlen(ptr) > (DWORD) itemCharWidth) {
			while (strlen(ptr) > (itemCharWidth - 1))
				ptr[strlen(ptr) - 1] = '\0';
			ptr[strlen(ptr)] = SCHR_BROWSE;
		}

		printText(defaultSurface, x + GUI_CONST_HOTKEYCHAR, y + 2, c, ptr + 1);
	}

	printChar(defaultSurface, r->x + r->w,
		r->y + ((halfpage - 1) * GUI_CONST_ITEM_SIZE) + 2, (i < cMenu_count)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_DW);

	delete r;
	needRedraw = true;
}
//-----------------------------------------------------------------------------
void UserInterface::drawFileSelector()
{
	cMenu_leftMargin = 0;
	cMenu_data = NULL;

	ScanDir(fileSelector->path, &fileSelector->dirEntries, &cMenu_count, uiSet->showHiddenFiles);
	if (cMenu_count <= 0) {
		menuClose();
		return;
	}

	cMenu_rect->w = GUI_CONST_BORDER + (maxCharsOnScreen * fontWidth) + GUI_CONST_BORDER;
	cMenu_rect->h = (3 * GUI_CONST_BORDER) + (17 * GUI_CONST_ITEM_SIZE) + GUI_CONST_BORDER + GUI_CONST_SEPARATOR;
	cMenu_rect->x = 1;
	cMenu_rect->y = (defaultSurface->h - cMenu_rect->h) / 2;

	fileSelector->itemsOnPage = (fileSelector->type == GUI_FS_SNAPSAVE) ? 30 : 32;

	drawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y,
		cMenu_rect->w, cMenu_rect->h);
	printTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1,
		cMenu_rect->w, GUI_COLOR_BACKGROUND, fileSelector->title);
	drawLineH(defaultSurface, cMenu_rect->x + (GUI_CONST_BORDER / 2),
		cMenu_rect->y + (3 * GUI_CONST_BORDER) +
		((fileSelector->itemsOnPage / 2) * GUI_CONST_ITEM_SIZE) + 4,
		cMenu_rect->w - GUI_CONST_BORDER, GUI_COLOR_SEPARATOR);

	int mx = cMenu_rect->x + cMenu_rect->w - GUI_CONST_BORDER - 1,
		my = cMenu_rect->y + cMenu_rect->h - 6 - fontHeight;

	printText(defaultSurface, mx - (6 * fontWidth) - GUI_CONST_HOTKEYCHAR, my,
		GUI_COLOR_FOREGROUND, "HOME \a\203\aH");

	if (fileSelector->type == GUI_FS_SNAPSAVE) {
		printText(defaultSurface, mx - (14 * fontWidth), my - fontLineHeight,
			GUI_COLOR_FOREGROUND, "ENTER NAME \aT\aA\aB");
	}

	mx = cMenu_rect->x + GUI_CONST_BORDER;
	if (fileSelector->type == GUI_FS_BASESAVE) {
		printText(defaultSurface, mx, my,
			GUI_COLOR_FOREGROUND, "\aT\aA\aB ENTER NAME");
	}
	else if (fileSelector->type == GUI_FS_SNAPLOAD) {
		printCheck(defaultSurface, mx, my + 1, GUI_COLOR_CHECKED,
			SCHR_CHECK, uiSet->Snapshot->dontRunOnLoad);
		printText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
			GUI_COLOR_FOREGROUND, "\a\203\aD DEBUG AFTER LOAD");
	}
	else if (fileSelector->type == GUI_FS_SNAPSAVE) {
		printCheck(defaultSurface, mx, my + 1, GUI_COLOR_CHECKED,
			SCHR_CHECK, uiSet->Snapshot->saveWithMonitor);
		printText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
			GUI_COLOR_FOREGROUND, "\a\203\aR SAVE WITH ROM");

		printCheck(defaultSurface, mx, my - fontLineHeight + 1,
			GUI_COLOR_CHECKED, SCHR_CHECK, uiSet->Snapshot->saveCompressed);
		printText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my - fontLineHeight,
			GUI_COLOR_FOREGROUND, "\a\203\aC SAVE COMPRESSED");
	}

	char *ptr, *path = new char[strlen(fileSelector->path) + 1];
	strcpy(path, fileSelector->path);
	ptr = path;
	if (strlen(ptr) > maxCharsOnScreen) {
		while (strlen(ptr) > (maxCharsOnScreen - 1))
			ptr++;
		*(--ptr) = SCHR_BROWSE;
	}

	printText(defaultSurface, cMenu_rect->x + GUI_CONST_BORDER,
		cMenu_rect->y + GUI_CONST_ITEM_SIZE + 1, GUI_COLOR_BORDER, ptr);

	delete [] path;

	drawFileSelectorItems();
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerMenu(WORD key)
{
	GUI_MENU_ENTRY *ptr = &cMenu_data[cMenu_hilite + 1];
	bool change = false;

	switch (key) {
		case SDLK_F4 | KM_ALT:
			ccb_exit(ptr);
			menuCloseAll();
			needRelease = true;
			return;

		case SDLK_ESCAPE:
			menuClose();
			needRelease = true;
			return;

		case SDLK_SPACE:
			if (ptr->enabled && ptr->type == MI_CHECKBOX
			 && ptr->action >= 0x8000)
				ptr->action |= 0x4000; // ... and continue in ENTER section...
			else
				break;

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (ptr->enabled) {
				needRelease = true;

				if (ptr->type == MI_SUBMENU && ptr->submenu)
					menuOpen(GUI_TYPE_MENU, ptr->submenu);
				else if (ptr->callback) {
					if (ptr->callback(ptr)) {
						if (uiSetChanges & PS_CLOSEALL)
							menuCloseAll();
						else
							menuClose();
						return;
					}
					else if (cMenu_data)
						change = true;
					else
						return;
				}
			}
			break;

		case SDLK_BACKSPACE:
		case SDLK_LEFT:
			if (menuStackLevel > 0)
				menuClose();

			needRelease = true;
			return;

		case SDLK_RIGHT:
			if (ptr->enabled && ptr->type == MI_SUBMENU && ptr->submenu)
				menuOpen(GUI_TYPE_MENU, ptr->submenu);

			needRelease = true;
			return;

		case SDLK_UP:
			cMenu_hilite--;
			if (cMenu_hilite < 0)
				cMenu_hilite += cMenu_count;

			while ((&cMenu_data[cMenu_hilite + 1])->type & (MI_FIXED | MI_SEPARATOR)) {
				cMenu_hilite--;
				if (cMenu_hilite < 0)
					cMenu_hilite += cMenu_count;
			}

			change = true;
			break;

		case SDLK_DOWN:
			cMenu_hilite++;
			if (cMenu_hilite >= cMenu_count)
				cMenu_hilite -= cMenu_count;

			while ((&cMenu_data[cMenu_hilite + 1])->type & (MI_FIXED | MI_SEPARATOR)) {
				cMenu_hilite++;
				if (cMenu_hilite >= cMenu_count)
					cMenu_hilite -= cMenu_count;
			}

			change = true;
			break;

		case SDLK_HOME:
		case SDLK_PAGEUP:
			cMenu_hilite = 0;
			needRelease = true;
			change = true;
			break;

		case SDLK_END:
		case SDLK_PAGEDOWN:
			cMenu_hilite = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			break;
	}

	if (change) {
		drawMenuItems();
		return;
	}

	for (ptr = &cMenu_data[1]; ptr->type != MENU_END; ptr++) {
		if (ptr->enabled && key == ptr->key) {
			if (ptr->type == MI_SUBMENU && ptr->submenu)
				menuOpen(GUI_TYPE_MENU, ptr->submenu);
			else if (ptr->callback) {
				if (ptr->callback(ptr)) {
					if (uiSetChanges & PS_CLOSEALL)
						menuCloseAll();
					else
						menuClose();
				}
				else
					drawMenuItems();
			}

			break;
		}
	}
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerFileSelector(WORD key)
{
	int i = cMenu_hilite, halfpage = fileSelector->itemsOnPage / 2;
	char *ptr = fileSelector->dirEntries[cMenu_hilite], *lastItem;
	bool change = false;
	BYTE b = 0;

	switch (key) {
		case SDLK_F4 | KM_ALT:
			ccb_exit(NULL);
			menuCloseAll();
			needRelease = true;
			return;

		case SDLK_ESCAPE:
			menuClose();
			needRelease = true;
			return;

		case SDLK_c | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPSAVE) {
				i = cMenu_hilite;
				uiSet->Snapshot->saveCompressed = !uiSet->Snapshot->saveCompressed;
				drawFileSelector();
				needRelease = true;
				change = true;
			}
			break;

		case SDLK_d | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPLOAD) {
				i = cMenu_hilite;
				uiSet->Snapshot->dontRunOnLoad = !uiSet->Snapshot->dontRunOnLoad;
				drawFileSelector();
				needRelease = true;
				change = true;
			}
			break;

		case SDLK_h | KM_ALT:
			cMenu_hilite = 0;
			strcpy(fileSelector->path, PathUserHome);
			drawFileSelector();
			needRelease = true;
			break;

		case SDLK_r | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPSAVE) {
				i = cMenu_hilite;
				uiSet->Snapshot->saveWithMonitor = !uiSet->Snapshot->saveWithMonitor;
				drawFileSelector();
				needRelease = true;
				change = true;
			}
			break;

		case SDLK_PERIOD | KM_ALT:
			uiSet->showHiddenFiles = !uiSet->showHiddenFiles;
			cMenu_hilite = 0;
			drawFileSelector();
			needRelease = true;
			break;

		case SDLK_TAB:
			if (fileSelector->type == GUI_FS_BASESAVE
			 || fileSelector->type == GUI_FS_SNAPSAVE) {
				char buffer[40];
				buffer[0] = '\0';

				if (*ptr == '\xA0')
					strcpy(buffer, ptr + 1);

				if (editBox("ENTER FILENAME:", buffer, 32, false) && strlen(buffer) > 0) {
					for (b = 0; b < strlen(buffer); b++) {
						switch (buffer[b]) {
						// multiplatform restricted characters in filenames;
						// filtering to avoid file portability problems;
							case ' ':  case '/':  case '\\': case '?':
							case '*':  case '%':  case ':':  case '|':
							case '"':  case '<':  case '>':
								buffer[b] = '_';
								break;

							default:
								break;
						}
					}

					if (fileSelector->extFilter) {
						if ((ptr = strrchr(buffer, '.'))) {
							for (b = 0; fileSelector->extFilter[b] != NULL; b++) {
								if (strcasecmp(ptr + 1, fileSelector->extFilter[b]) == 0) {
									keyhandlerFileSelectorCallback(buffer);
									return;
								}
							}
						}

						ptr = buffer + strlen(buffer);
						*ptr = '.';
						strcpy(ptr + 1, fileSelector->extFilter[0]);
					}

					keyhandlerFileSelectorCallback(buffer);
					return;
				}
			}
			break;

		case SDLK_BACKSPACE:
			ptr = fileSelector->dirEntries[0];
		//	select ".." and continue in next case...

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (*ptr == '\xA0') {
				lastItem = ptr + 1;
				if ((ptr = strrchr(lastItem, '.'))) {
					for (b = 0; fileSelector->extFilter[b] != NULL; b++) {
						if (strcasecmp(ptr + 1, fileSelector->extFilter[b]) == 0) {
							keyhandlerFileSelectorCallback(lastItem);
							return;
						}
					}
				}
			}
			else if (TestDir(fileSelector->path, ptr + 1, &lastItem)) {
				cMenu_hilite = 0;
				drawFileSelector();
				needRelease = true;

				if (lastItem) {
					for (i = 0; i < cMenu_count; i++) {
						ptr = fileSelector->dirEntries[i];
						if (strcmp(ptr + 1, lastItem) == 0) {
							change = true;
							break;
						}
					}
				}
				else
					return;
			}
			break;

		case SDLK_LEFT:
		case SDLK_PAGEUP:
			if (i > 0) {
				i -= (key == SDLK_LEFT) ? halfpage : fileSelector->itemsOnPage;
				if (i < 0)
					i = 0;
				change = true;
			}
			break;

		case SDLK_RIGHT:
		case SDLK_PAGEDOWN:
			if (i < (cMenu_count - 1)) {
				i += (key == SDLK_RIGHT) ? halfpage : fileSelector->itemsOnPage;
				if (i >= cMenu_count)
					i = (cMenu_count - 1);
				change = true;
			}
			break;

		case SDLK_UP:
			if (i > 0) {
				i--;
				change = true;
			}
			break;

		case SDLK_DOWN:
			if (i < (cMenu_count - 1)) {
				i++;
				change = true;
			}
			break;

		case SDLK_HOME:
			i = 0;
			needRelease = true;
			change = true;
			break;

		case SDLK_END:
			i = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			break;
	}

	if (change) {
		while (i < cMenu_leftMargin) {
			cMenu_leftMargin -= halfpage;
			if (cMenu_leftMargin < 0)
				cMenu_leftMargin = 0;
		}

		if (i >= (cMenu_leftMargin + fileSelector->itemsOnPage))
			cMenu_leftMargin = i - halfpage;

		cMenu_hilite = i;
		drawFileSelectorItems();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerFileSelectorCallback(char *fileName)
{
	BYTE ret = fileSelector->tag;

	char *ptr = fileSelector->path + strlen(fileSelector->path);
	*ptr = DIR_DELIMITER;
	*(ptr + 1) = '\0';
	strcat(fileSelector->path, fileName);

	if ((fileSelector->type == GUI_FS_BASESAVE
	  || fileSelector->type == GUI_FS_SNAPSAVE)
	  && FileExists(fileSelector->path)) {

		if (queryDialog("OVERWRITE?", false) != GUI_QUERY_YES) {
			*ptr = '\0';
			return;
		}
	}

	fileSelector->callback(fileSelector->path, &ret);
	if (ret == 0) {
		menuCloseAll();
		needRelease = true;
		uiSetChanges = PS_CLOSEALL;
	}
	else if (ret == 1) {
		menuClose();
		needRelease = true;
	}
	else
		*ptr = '\0';
}
//-----------------------------------------------------------------------------
BYTE UserInterface::queryDialog(const char *title, bool save)
{
	void *data = (save) ? gui_query_save : gui_query_confirm;
	const char *dialogTitle = new char[strlen(title) + 5];

	sprintf((char *) dialogTitle, "  %s  ", title);
	((GUI_MENU_ENTRY *) data)->text = dialogTitle;

	BYTE *bkm_frameSave = new BYTE[frameLength];
	memcpy(bkm_frameSave, defaultSurface->pixels, frameLength);

	GUI_MENU_ENTRY *bkm_data = cMenu_data;
	SDL_Rect *bkm_rect = new SDL_Rect(*cMenu_rect);
	int bkm_leftMargin = cMenu_leftMargin, bkm_count = cMenu_count,
	    bkm_hilite = cMenu_hilite;

	menuStackLevel++;
	cMenu_hilite = (save) ? 2 : 1;
	drawMenu(data);

	bool change;
	SDL_Event event;
	GUI_MENU_ENTRY *ptr;
	uiQueryState = GUI_QUERY_NONE;

	SDL_Delay(GPU_TIMER_INTERVAL);
	while (uiQueryState == GUI_QUERY_NONE) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					ptr = &cMenu_data[cMenu_hilite + 1];
					change = false;

					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							uiQueryState = GUI_QUERY_CANCEL;
							break;

						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							uiQueryState = ptr->action;
							break;

						case SDLK_UP:
							if (cMenu_hilite > 0) {
								cMenu_hilite--;
								change = true;
							}
							break;

						case SDLK_DOWN:
							if (cMenu_hilite < (cMenu_count - 1)) {
								cMenu_hilite++;
								change = true;
							}
							break;

						default:
							break;
					}

					if (change) {
						drawMenuItems();
						break;
					}

					for (ptr = &cMenu_data[1]; ptr->type != MENU_END; ptr++) {
						if (event.key.keysym.sym == ptr->key) {
							uiQueryState = ptr->action;
							break;
						}
					}

				default:
					break;
			}
		}
	}

	memcpy(defaultSurface->pixels, bkm_frameSave, frameLength);
	needRelease = true;
	needRedraw = true;

	SDL_Delay(GPU_TIMER_INTERVAL);

	((GUI_MENU_ENTRY *) data)->text = NULL;
	delete [] bkm_frameSave;
	delete [] dialogTitle;

	cMenu_rect->x = bkm_rect->x;
	cMenu_rect->y = bkm_rect->y;
	cMenu_rect->w = bkm_rect->w;
	cMenu_rect->h = bkm_rect->h;
	delete bkm_rect;

	cMenu_data = bkm_data;
	cMenu_leftMargin = bkm_leftMargin;
	cMenu_hilite = bkm_hilite;
	cMenu_count = bkm_count;
	menuStackLevel--;

	return uiQueryState;
}
//-----------------------------------------------------------------------------
void UserInterface::messageBox(const char *text, ...)
{
	va_list va;
	va_start(va, text);
	vsprintf(msgbuffer, text, va);
	va_end(va);

	BYTE *bkm_frameSave = new BYTE[frameLength];
	memcpy(bkm_frameSave, defaultSurface->pixels, frameLength);

	menuStackLevel++;

	unsigned i, w = 0, h = fontLineHeight, x, y;
	for (i = 0, x = 0; i < strlen(msgbuffer); i++) {
		if (x > (maxCharsOnScreen - 5)) {
			for (y = i; y <= strlen(msgbuffer); y++)
				msgbuffer[y + 1] = msgbuffer[y];

			msgbuffer[i] = '\n';
		}

		if (msgbuffer[i] == '\n') {
			h += fontLineHeight;
			if (w < (x * fontWidth))
				w = (x * fontWidth);

			x = 0;
			continue;
		}
		else if (msgbuffer[i] == '\a')
			continue;

		x++;
	}

	if (w < (x * fontWidth))
		w = (x * fontWidth);

	w = (4 * GUI_CONST_BORDER) + w;
	h = (2 * GUI_CONST_BORDER) + h;
	x = (defaultSurface->w - w) / 2;
	y = (defaultSurface->h - h) / 2;

	drawOutlineRounded(defaultSurface, x - 1, y - 1, w + 2, h + 2, GUI_COLOR_SHADOW);
	drawRectangle(defaultSurface, x + 1, y + 1, w - 2, h - 2, GUI_COLOR_BACKGROUND);
	drawOutlineRounded(defaultSurface, x, y, w, h, GUI_COLOR_BORDER);
	printText(defaultSurface, x + (2 * GUI_CONST_BORDER),
			y + GUI_CONST_BORDER + 1, GUI_COLOR_FOREGROUND, msgbuffer);

	needRedraw = true;
	SDL_Delay(GPU_TIMER_INTERVAL);

	i = 1;
	SDL_Event event;
	while (i) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_SPACE:
						case SDLK_ESCAPE:
						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							i = 0;
							break;

						default:
							break;
					}

				default:
					break;
			}
		}
	}

	memcpy(defaultSurface->pixels, bkm_frameSave, frameLength);
	SDL_Delay(GPU_TIMER_INTERVAL);

	delete [] bkm_frameSave;
	needRelease = true;
	needRedraw = true;
	menuStackLevel--;
}
//-----------------------------------------------------------------------------
BYTE UserInterface::editBox(const char *title, char *buffer, BYTE maxLength, bool decimal)
{
	if (maxLength <= 0 || maxLength > (maxCharsOnScreen - 2))
		maxLength = maxCharsOnScreen - 2;

	BYTE *bkm_frameSave = new BYTE[frameLength];
	memcpy(bkm_frameSave, defaultSurface->pixels, frameLength);

	menuStackLevel++;

	WORD i, len = strlen(buffer), cursor = len,
		w = (strlen(title) + 4) * fontWidth, h, x, y;

	if (w < ((maxLength + 1) * fontWidth))
		w = ((maxLength + 1) * fontWidth);

	w = (2 * GUI_CONST_BORDER) + w;
	h = (4 * GUI_CONST_BORDER);
	x = (defaultSurface->w - w) / 2;
	y = (defaultSurface->h - h) / 2;

	drawDialogWithBorder(defaultSurface, x, y, w, h);
	printTitle(defaultSurface, x, y + 1, w, GUI_COLOR_BACKGROUND, title);

	x += GUI_CONST_BORDER;
	y += (2 * GUI_CONST_BORDER);
	w -= (2 * GUI_CONST_BORDER);

	BYTE result = -1;
	SDL_Event event;
	bool atTheEnd = true, change = true;

	SDL_EnableUNICODE(1);
	SDL_Delay(GPU_TIMER_INTERVAL);
	while (result == (BYTE) -1) {
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYDOWN:
					switch (event.key.keysym.sym) {
						case SDLK_ESCAPE:
							result = 0;
							break;

						case SDLK_RETURN:
						case SDLK_KP_ENTER:
							// trim left
							cursor = 0;
							while (buffer[cursor] == ' ')
								cursor++;

							if (cursor > 0)
								for (i = 0; cursor <= len; i++, cursor++)
									buffer[i] = buffer[cursor];

							// trim right
							cursor = (len = strlen(buffer)) - 1;
							while (buffer[cursor] == ' ')
								cursor--;

							if (++cursor < len)
								buffer[cursor] = '\0';

							result = 1;
							break;

						case SDLK_LEFT:
							if (cursor > 0) {
								cursor--;
								change = true;
							}
							break;

						case SDLK_RIGHT:
							if (!atTheEnd) {
								cursor++;
								change = true;
							}
							break;

						case SDLK_BACKSPACE:
							if (cursor > 0) {
								cursor--;
								for (i = cursor; i < len; i++)
									buffer[i] = buffer[i + 1];
								change = true;
							}
							break;

						case SDLK_INSERT:
							if (!atTheEnd && len < maxLength) {
								for (i = len + 1; i > cursor; i--)
									buffer[i] = buffer[i - 1];
								buffer[cursor] = ' ';
								change = true;
							}
							break;

						case SDLK_DELETE:
							if (!atTheEnd) {
								for (i = cursor; i < len; i++)
									buffer[i] = buffer[i + 1];
								change = true;
							}
							break;

						case SDLK_HOME:
							if (cursor > 0) {
								cursor = 0;
								change = true;
							}
							break;

						case SDLK_END:
							if (!atTheEnd) {
								cursor = len;
								change = true;
							}
							break;

						default:
							if (event.key.keysym.unicode < 0x0020
							 || event.key.keysym.unicode > 0x007E)
								break;

							char c = (char) event.key.keysym.unicode;
							if (cursor < maxLength) {
								if (decimal && (c < '0' || c > '9'))
									break;

								buffer[cursor] = c;
								cursor++;
								if (atTheEnd)
									buffer[cursor] = '\0';

								change = true;
							}
							break;
					}

				default:
					break;
			}

			if (change) {
				drawRectangle(defaultSurface, x, y - 1, w, fontHeight + 1,
					GUI_COLOR_BACKGROUND);
				drawRectangle(defaultSurface, x + (cursor * fontWidth), y - 1,
					fontWidth, fontHeight + 1, GUI_COLOR_HIGHLIGHT);
				printText(defaultSurface, x, y, GUI_COLOR_FOREGROUND, buffer);

				len = strlen(buffer);
				atTheEnd = (cursor == len);

				needRedraw = true;
				change = false;
				break;
			}
		}
	}

	memcpy(defaultSurface->pixels, bkm_frameSave, frameLength);
	delete [] bkm_frameSave;

	SDL_Delay(GPU_TIMER_INTERVAL);
	SDL_EnableUNICODE(0);
	needRelease = true;
	needRedraw = true;

	menuStackLevel--;
	return result;
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
			drawMenu(data);
			break;

		case GUI_TYPE_FILESELECTOR:
			drawFileSelector();
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
	if (menuStack[menuStackLevel].type == GUI_TYPE_FILESELECTOR)
		ScanDir(NULL, &fileSelector->dirEntries, &cMenu_count, uiSet->showHiddenFiles);

	menuStack[menuStackLevel].data = NULL;

	menuStackLevel--;
	if (menuStackLevel >= 0) {
		cMenu_hilite = menuStack[menuStackLevel].hilite;
		memcpy(defaultSurface->pixels, frameSave, frameLength);

		switch (menuStack[menuStackLevel].type) {
			case GUI_TYPE_MENU:
				drawMenu(menuStack[menuStackLevel].data);
				break;

			default :
				break;
		}
	}
}
//-----------------------------------------------------------------------------
void UserInterface::menuCloseAll()
{
	if (menuStack[menuStackLevel].type == GUI_TYPE_FILESELECTOR)
		ScanDir(NULL, &fileSelector->dirEntries, &cMenu_count, uiSet->showHiddenFiles);

	for (int i = menuStackLevel; i >= 0; i--)
		menuStack[i].data = NULL;

	menuStackLevel = -1;
	memcpy(defaultSurface->pixels, frameSave, frameLength);
}
//-----------------------------------------------------------------------------
void UserInterface::menuHandleKey(WORD key)
{
	if (menuStackLevel >= 0) {
		switch (menuStack[menuStackLevel].type) {
			case GUI_TYPE_MENU:
				keyhandlerMenu(key);
				break;

			case GUI_TYPE_FILESELECTOR:
				keyhandlerFileSelector(key);
				break;

			default :
				break;
		}
	}
}
//-----------------------------------------------------------------------------
