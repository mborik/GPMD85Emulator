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
#include "TapeBrowser.h"
#include "imgui/imgui.h"
#include "imgui-mods/imgui_file_browser.h"
//-----------------------------------------------------------------------------
#define STATUSBAR_HEIGHT  48
//-----------------------------------------------------------------------------
#define SDL_PIXELFORMAT_DEFAULT SDL_PIXELFORMAT_ABGR8888
#define SDL_DEFAULT_MASK_QUAD 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#define DWORD_COLOR_ENTRY(R, G, B) SDL_FOURCC(R, G, B, 0xff)
//-----------------------------------------------------------------------------
class UserInterface
{
	public:
		DWORD globalPalette[256];

		BYTE InvokeSettingsChange;
		sigslot::signal<> ProcessSettingsCallback;
		sigslot::signal<TMenuQueryType> QueryDialogCallback;
		sigslot::signal<char *> FileSelectorCallback;

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
		void DrawTapeDialog();
		void DrawDebugWindow();
		void DrawEmulatorWindow();

		void QueryDialog(const char *title, const char *message, bool save);
		void MessageBox(const char *text, ...);
		void FileSelector(
			TFileSelectType type,
			const char *title, const char *recentFile,
			const std::vector<std::string> &filter = {".*"},
			bool fallbackToResourceDir = false
		);

		void Execute(TGuiElementType type, void *data = NULL);
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
		bool dialogTapeBrowserOpened;
		bool dialogDiskImagesOpened;
		bool dialogAboutOpened;

		TFileSelectType fileSelectorType;
		ImGui::FileBrowser *fileSelector;
		char *fileSelectorPath;
		char *fileSelectorRecentPath;

		const char *queryDialogTitle;
		const char *queryDialogMessage;
		bool queryDialogSaveType;
		bool queryDialogShouldOpen;

		std::vector<TTapeBrowser::TDialogItem> tapeDialogEntries;

		void SetButtonColor(int icon);
		void MachineMenuItem(const char *name, TComputerModel model);
		void AttributeMenuItems(bool enabled = false);
		void DiskImagesMenuItems(bool inMenu = false);
		void DrawTapeDialogContextMenu(
			ImGuiSelectionBasicStorage &selection,
			int &index, const TTapeBrowser::TDialogItem &item
		);

		void DrawDebugWidgetDisass(bool full);
		void DrawDebugWidgetRegs();
		void DrawDebugWidgetStack();
		void DrawDebugWidgetBreaks();
};
//-----------------------------------------------------------------------------
#endif
