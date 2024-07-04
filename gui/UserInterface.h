/*	UserInterface.h: Class for GUI rendering.
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
#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
#define SCHR_ERROR     127
#define SCHR_NAVIGATOR 128
#define SCHR_SCROLL_UP 129
#define SCHR_SCROLL_DW 130
#define SCHR_HOTKEY    131
#define SCHR_SHIFT     132
#define SCHR_BROWSE    133
#define SCHR_DIRECTORY 134
#define SCHR_CHECK     135
#define SCHR_RADIO     136
#define SCHR_LOCKER    137
#define SCHR_STOP      138
#define SCHR_LAST      144
//-----------------------------------------------------------------------------
#define GUI_CONST_BORDER     8
#define GUI_CONST_ITEM_SIZE  11
#define GUI_CONST_SEPARATOR  5
#define GUI_CONST_CHK_MARGIN 14
#define GUI_CONST_HOTKEYCHAR 10
#define GUI_CONST_KEY_REPEAT 50
#define GUI_CONST_TAPE_ITEMS 16
//-----------------------------------------------------------------------------
#define GUI_COLOR_SHADOW     16
#define GUI_COLOR_BORDER     17
#define GUI_COLOR_BACKGROUND 18
#define GUI_COLOR_FOREGROUND 19
#define GUI_COLOR_HIGHLIGHT  20
#define GUI_COLOR_DISABLED   21
#define GUI_COLOR_SEPARATOR  22
#define GUI_COLOR_CHECKED    23
#define GUI_COLOR_SMARTKEY   24
#define GUI_COLOR_HOTKEY     25
#define GUI_COLOR_DBG_BACK   26
#define GUI_COLOR_DBG_TEXT   27
#define GUI_COLOR_DBG_CURSOR 28
#define GUI_COLOR_DBG_BORDER 29
#define GUI_COLOR_STAT_TEXT  32
#define GUI_COLOR_STAT_PAUSE 33
#define GUI_COLOR_STATTAP_BG 34
#define GUI_COLOR_STATTAP_FG 35
//-----------------------------------------------------------------------------
#define STATUSBAR_ICON    10
#define STATUSBAR_SPACING 14
#define STATUSBAR_HEIGHT  20
//-----------------------------------------------------------------------------
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define SDL_PIXELFORMAT_DEFAULT SDL_PIXELFORMAT_ABGR8888
#define SDL_DEFAULT_MASK_QUAD 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#define DWORD_COLOR_ENTRY(R, G, B) SDL_FOURCC(R, G, B, 0xff)
#else
#define SDL_PIXELFORMAT_DEFAULT SDL_PIXELFORMAT_RGBA8888
#define SDL_DEFAULT_MASK_QUAD 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#define DWORD_COLOR_ENTRY(R, G, B) SDL_FOURCC(0xff, B, G, R)
#endif
//-----------------------------------------------------------------------------
typedef struct _GUI_MENU_ENTRY {
	TMenuItemType type;
	const char *text;
	const char *hotkey;
	WORD key;

	struct _GUI_MENU_ENTRY *submenu;
	bool (*callback) (_GUI_MENU_ENTRY *ptr);
	const char * (*detail) (_GUI_MENU_ENTRY *ptr);

	bool enabled; // enabled/disabled item
	bool state;   // checkbox/radio state
	WORD action;  // action value
} GUI_MENU_ENTRY;
//-----------------------------------------------------------------------------
class UserInterface : public sigslot::has_slots<>
{
	public:
		enum GUI_MENU_TYPE {
			GUI_TYPE_MENU,              // General menu
			GUI_TYPE_FILESELECTOR,      // File selector
			GUI_TYPE_TAPEBROWSER,       // Tape-browser
			GUI_TYPE_TAPE_POPUP,        // Tape-browser popup menu
			GUI_TYPE_DEBUGGER,          // Debugger dialog
			GUI_TYPE_POKE               // Poke dialog
		};

		typedef struct GUI_FILESELECTOR_DATA {
			TFileSelectType type;
			const char *title;
			char path[MAX_PATH];
			char search[22];
			int  count;
			char **dirEntries;
			char **extFilter;
			BYTE itemsOnPage;
			BYTE tag;
			sigslot::signal2<char *, BYTE *> callback;
		} GUI_FILESELECTOR_DATA;

		typedef struct GUI_TAPEDIALOG_DATA {
			int  count;
			char **entries;
			struct {
				SDL_Rect *rect;
				int leftMargin, count, hilite;
			} popup;
		} GUI_TAPEBROWSER_DATA;

		bool needRelease;
		BYTE uiSetChanges;
		BYTE uiQueryState;
		sigslot::signal0<> uiCallback;

		DWORD globalPalette[256];
		SDL_Texture *defaultTexture;
		SDL_Texture *statusTexture;
		SDL_Rect *statusRect;

		GUI_FILESELECTOR_DATA *fileSelector;
		GUI_TAPEDIALOG_DATA *tapeDialog;
		sigslot::signal2<char *, BYTE *> editBoxValidator;

		UserInterface();
		virtual ~UserInterface();
		void InitDefaultTexture(int width, int height);

		inline void SetLineHeight(BYTE l) { fontLineHeight = (l > 0) ? l : (fontHeight + 1); }
		inline bool InMenu() { return (menuStackLevel >= 0); }

		void AboutDialog();
		BYTE QueryDialog(const char *title, bool save);
		void MessageBox(const char *text, ...);
		BYTE EditBox(const char *title, const char *description, char *buffer, BYTE maxLength, bool decimal);

		void MenuOpen(GUI_MENU_TYPE type, void *data = NULL);
		void MenuClose();
		void MenuCloseAll();
		void MenuHandleKey(WORD key);

		void InitStatusBarTexture();
		void RedrawStatusBar();
		void SetLedState(int led);
		void SetIconState(int icon);
		void SetComputerModel(TComputerModel model);
		inline void SetStatusPercentage(int val) { statusPercentage = val; }
		inline void SetStatusFPS(int val) { statusFPS = val; }

	private:
		int ledState;
		int iconState;
		int statusPercentage;
		int statusFPS;
		char computerModel[8];

		BYTE *fontData;
		BYTE  fontWidth;
		BYTE  fontHeight;
		BYTE  fontLineHeight;
		BYTE  maxCharsOnScreen;

		DWORD frameLength;
		WORD  frameWidth;
		WORD  frameHeight;

		// based on SDL_Surface
		typedef struct GUI_SURFACE {
			DWORD format;
			int   w, h;
			int   pitch;
			BYTE *pixels;
		} GUI_SURFACE;

		GUI_SURFACE *icons;

		short menuStackLevel;
		struct GUI_MENU_STACK {
			GUI_MENU_TYPE type;
			void *data;
			int hilite;
			BYTE *frame;
		} menuStack[8];

		GUI_MENU_ENTRY *cMenu_data;
		SDL_Rect *cMenu_rect;
		int cMenu_leftMargin, cMenu_count, cMenu_hilite;

		GUI_SURFACE *LoadImgToSurface(const char *file);
		void BlitToSurface(GUI_SURFACE *src, const SDL_Rect *srcRect, GUI_SURFACE *dst, const SDL_Rect *dstRect);

		GUI_SURFACE *LockSurface(SDL_Texture *texture);
		void UnlockSurface(SDL_Texture *texture, GUI_SURFACE *surface);

		void PutPixel(GUI_SURFACE *s, int x, int y, BYTE col);
		void PrintChar(GUI_SURFACE *s, int x, int y, BYTE col, BYTE ch);
		void PrintText(GUI_SURFACE *s, int x, int y, BYTE col, const char *msg);
		void PrintTitle(GUI_SURFACE *s, int x, int y, int w, BYTE col, const char *msg);
		void PrintFormatted(GUI_SURFACE *s, int x, int y, BYTE col, const char *msg, ...);
		void PrintRightAlign(GUI_SURFACE *s, int x, int y, BYTE col, const char *msg, ...);
		void DrawLineH(GUI_SURFACE *s, int x, int y, int len, BYTE col);
		void DrawLineV(GUI_SURFACE *s, int x, int y, int len, BYTE col);
		void DrawRectangle(GUI_SURFACE *s, int x, int y, int w, int h, BYTE col);
		void DrawOutline(GUI_SURFACE *s, int x, int y, int w, int h, BYTE col);
		void DrawOutlineRounded(GUI_SURFACE *s, int x, int y, int w, int h, BYTE col);
		void DrawDialogWithBorder(GUI_SURFACE *s, int x, int y, int w, int h);
		void DrawDebugFrame(GUI_SURFACE *s, int x, int y, int w, int h);
		void PrintCheck(GUI_SURFACE *s, int x, int y, BYTE col, BYTE ch, bool state);

		void DrawMenuItems(GUI_SURFACE *s = NULL);
		void DrawMenu(void *data);
		void DrawFileSelectorItems(GUI_SURFACE *s = NULL);
		void DrawFileSelector(bool update = true);
		void DrawTapeDialogItems(GUI_SURFACE *s = NULL);
		void DrawTapeDialog(bool update = true);
		void DrawDebugWidgetDisass(GUI_SURFACE *s, SDL_Rect *r, bool full);
		void DrawDebugWidgetRegs(GUI_SURFACE *s, SDL_Rect *r);
		void DrawDebugWidgetStack(GUI_SURFACE *s, SDL_Rect *r);
		void DrawDebugWidgetBreaks(GUI_SURFACE *s, SDL_Rect *r);
		void DrawDebugWindow();
		void KeyhandlerMenu(WORD key);
		void KeyhandlerFileSelector(WORD key);
		void KeyhandlerFileSelectorCallback(char *fileName);
		int  KeyhandlerFileSelectorSearch(int from = 0);
		bool KeyhandlerFileSelectorSearchClean();
		void KeyhandlerTapeDialog(WORD key);
		void KeyhandlerDebugWindow(WORD key);
};
//-----------------------------------------------------------------------------
#endif
