/*	UserInterface.h: Class for GUI rendering.
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
#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_
//-----------------------------------------------------------------------------
#include "globals.h"
#include "Settings.h"
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
#define GUI_COLOR_SHADOW     80
#define GUI_COLOR_BORDER     81
#define GUI_COLOR_BACKGROUND 82
#define GUI_COLOR_FOREGROUND 83
#define GUI_COLOR_HIGHLIGHT  84
#define GUI_COLOR_DISABLED   85
#define GUI_COLOR_SEPARATOR  86
#define GUI_COLOR_CHECKED    87
#define GUI_COLOR_SMARTKEY   88
#define GUI_COLOR_HOTKEY     89
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
			GUI_TYPE_SELECT,            // Select machine
			GUI_TYPE_PERIPHERALS,       // Peripherals options
			GUI_TYPE_MEMORY,            // ROM files dialog
			GUI_TYPE_FILESELECTOR,      // File selector
			GUI_TYPE_DISKIMAGES,        // Disk images dialog
			GUI_TYPE_TAPEBROWSER,       // Tape-browser
			GUI_TYPE_DEBUGGER,          // Debugger dialog
			GUI_TYPE_MEMORYBLOCK,       // Memory block dialog
			GUI_TYPE_POKE               // Poke dialog
		};

		typedef struct GUI_FILESELECTOR_DATA {
			TFileSelectType type;
			const char *title;
			char path[MAX_PATH];
			int  count;
			char **dirEntries;
			char **extFilter;
			BYTE itemsOnPage;
			BYTE tag;
			sigslot::signal2<char *, BYTE *> callback;
		} GUI_FILESELECTOR_DATA;

		typedef struct GUI_TAPEBROWSER_DATA {
			int  count;
			char **entries;
			bool hex;
		} GUI_TAPEBROWSER_DATA;

		static TSettings *uiSet;
		static BYTE uiSetChanges;
		static BYTE uiQueryState;
		static sigslot::signal0<> uiCallback;

		SDL_Surface *defaultSurface;
		GUI_FILESELECTOR_DATA *fileSelector;
		GUI_TAPEBROWSER_DATA *tapeBrowser;
		sigslot::signal2<char *, BYTE *> editBoxValidator;

		bool needRedraw;
		bool needRelease;

		UserInterface();
		virtual ~UserInterface();
		void prepareDefaultSurface(int width, int height);

		inline void setLineHeight(BYTE l) { fontLineHeight = (l > 0) ? l : (fontHeight + 1); }
		inline bool isInMenu() { return (menuStackLevel >= 0); }

		void printText(SDL_Surface *s, int x, int y, DWORD col, const char *msg);
		BYTE queryDialog(const char *title, bool save);
		void messageBox(const char *text, ...);
		BYTE editBox(const char *title, char *buffer, BYTE maxLength, bool decimal);

		void menuOpen(GUI_MENU_TYPE type, void *data = NULL);
		void menuClose();
		void menuCloseAll();
		void menuHandleKey(WORD key);

	private:
		BYTE *fontData;
		BYTE  fontWidth;
		BYTE  fontHeight;
		BYTE  fontLineHeight;
		BYTE  maxCharsOnScreen;

		BYTE *frameSave;
		DWORD frameLength;

		short menuStackLevel;
		struct GUI_MENU_STACK {
			GUI_MENU_TYPE type;
			void *data;
			int hilite;
		} menuStack[8];

		GUI_MENU_ENTRY *cMenu_data;
		SDL_Rect *cMenu_rect;
		int cMenu_leftMargin, cMenu_count, cMenu_hilite;

		void putPixel(SDL_Surface *s, int x, int y, DWORD col);
		void printChar(SDL_Surface *s, int x, int y, DWORD col, BYTE ch);
		void printTitle(SDL_Surface *s, int x, int y, int w, DWORD col, const char *msg);
		void printFormatted(SDL_Surface *s, int x, int y, DWORD col, const char *msg, ...);
		void printRightAlign(SDL_Surface *s, int x, int y, DWORD col, const char *msg, ...);
		void drawRectangle(SDL_Surface *s, int x, int y, int w, int h, DWORD col);
		void drawLineH(SDL_Surface *s, int x, int y, int len, DWORD col);
		void drawLineV(SDL_Surface *s, int x, int y, int len, DWORD col);
		void drawOutline(SDL_Surface *s, int x, int y, int w, int h, DWORD col);
		void drawOutlineRounded(SDL_Surface *s, int x, int y, int w, int h, DWORD col);
		void drawDialogWithBorder(SDL_Surface *s, int x, int y, int w, int h);
		void printCheck(SDL_Surface *s, int x, int y, DWORD col, BYTE ch, bool state);
		void drawMenuItems();
		void drawMenu(void *data);
		void drawFileSelectorItems();
		void drawFileSelector(bool update = true);
		void drawTapeBrowserItems();
		void drawTapeBrowser(bool update = true);
		void keyhandlerMenu(WORD key);
		void keyhandlerFileSelector(WORD key);
		void keyhandlerFileSelectorCallback(char *fileName);
		void keyhandlerTapeBrowser(WORD key);
};
//-----------------------------------------------------------------------------
#endif
