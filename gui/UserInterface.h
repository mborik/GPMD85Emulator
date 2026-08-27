/*	UserInterface.h: Class for GUI rendering.
	Copyright (c) 2011-2026 Martin Borik <martin@borik.net>

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
#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_
//-----------------------------------------------------------------------------
#include "globals.h"
#include "imgui/imgui.h"
#include "imgui-mods/imgui_file_browser.h"
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
#define STATUSBAR_HEIGHT  48
//-----------------------------------------------------------------------------
#define SDL_PIXELFORMAT_DEFAULT SDL_PIXELFORMAT_ABGR8888
#define SDL_DEFAULT_MASK_QUAD 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#define DWORD_COLOR_ENTRY(R, G, B) SDL_FOURCC(R, G, B, 0xff)
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
class UserInterface
{
	public:
		enum GUI_MENU_TYPE {
			GUI_TYPE_MENU,              // General menu
			GUI_TYPE_ABOUT,             // About modal dialog
			GUI_TYPE_DISKIMAGES,        // Disk images dialog
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
			sigslot::signal<char *, BYTE *> callback;
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
		sigslot::signal<> uiCallback;
		sigslot::signal<TMenuQueryType> uiQueryCallback;
		sigslot::signal<char *> uiFileSelectorCallback;

		DWORD globalPalette[256];
		SDL_Texture *defaultTexture;

		GUI_TAPEDIALOG_DATA *tapeDialog;
		sigslot::signal<char *, BYTE *> editBoxValidator;

		UserInterface();
		virtual ~UserInterface();

		inline bool InMenu() { return isMenuHovered; }
		inline bool InEmulatorWindow() { return isEmulatorWindowFocused; }
		inline bool InAnyWindowExceptEmulator() {
			return
				isMenuHovered ||
				isAnyPopupWindowFocused ||
				!isEmulatorWindowFocused;
		}

		bool OnMenuLeave();

		void DrawMenu();
		void DrawAboutDialog();
		void DrawQueryDialog();
		void DrawFileSelector();
		void DrawDiskImagesDialog();
		void DrawEmulatorWindow();

		void QueryDialog(const char *title, const char *message, bool save);
		void MessageBox(const char *text, ...);
		BYTE EditBox(const char *title, const char *description, char *buffer, BYTE maxLength, bool decimal);
		void FileSelector(
			TFileSelectType type,
			const char *title, const char *recentFile,
			const std::vector<std::string> &filter = {".*"},
			bool fallbackToResourceDir = false
		);

		void MenuOpen(GUI_MENU_TYPE type, void *data = NULL);
		void MenuClose();
		void MenuCloseAll();

		void RedrawStatusBar(float horizontalPadding = 0.0f);
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

		bool isMenuHovered;
		bool isAnyPopupWindowFocused;
		bool isEmulatorWindowFocused;
		bool dialogDiskImagesOpened;
		bool dialogAboutOpened;

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

		TFileSelectType fileSelectorType;
		ImGui::FileBrowser *fileSelector;
		char *fileSelectorPath;
		char *fileSelectorRecentPath;

		const char *queryDialogTitle;
		const char *queryDialogMessage;
		bool queryDialogSaveType;
		bool queryDialogShouldOpen;

		GUI_SURFACE *LockSurface(SDL_Texture *texture);
		void UnlockSurface(SDL_Texture *texture, GUI_SURFACE *surface);

		void SetButtonColor(int icon);
		void MachineMenuItem(const char *name, TComputerModel model);
		void AttributeMenuItems(bool enabled = false);
		void DiskImagesMenuItems(bool inMenu = false);

	/* OBSOLETE TBD { */
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

		void DrawTapeDialogItems(GUI_SURFACE *s = NULL);
		void DrawTapeDialog(bool update = true);
		void DrawDebugWidgetDisass(GUI_SURFACE *s, SDL_Rect *r, bool full);
		void DrawDebugWidgetRegs(GUI_SURFACE *s, SDL_Rect *r);
		void DrawDebugWidgetStack(GUI_SURFACE *s, SDL_Rect *r);
		void DrawDebugWidgetBreaks(GUI_SURFACE *s, SDL_Rect *r);
		void DrawDebugWindow();

		void KeyhandlerTapeDialog(WORD key);
		void KeyhandlerDebugWindow(WORD key);
	/* } */
};
//-----------------------------------------------------------------------------
#endif
