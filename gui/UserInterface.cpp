/*	UserInterface.cpp: Class for GUI rendering.
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
#include "CommonUtils.h"
#include "UserInterface.h"
#include "UserInterfaceData.h"
#include "Emulator.h"
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
UserInterface *GUI;
//-----------------------------------------------------------------------------
UserInterface::UserInterface()
{
	debug("GUI", "Initializing...");

	tapeDialog = new GUI_TAPEDIALOG_DATA;
	tapeDialog->entries = NULL;
	tapeDialog->count = 0;
	tapeDialog->popup.rect = NULL;

	ledState = 0;
	iconState = 0;
	statusFPS = 0;
	statusPercentage = 0;
	computerModel[0] = '\0';

	isMenuHovered = false;
	isEmulatorWindowFocused = false;
	dialogAboutOpened = false;
	dialogDiskImagesOpened = false;

	queryDialogTitle = NULL;
	queryDialogMessage = NULL;
	queryDialogSaveType = false;
	queryDialogShouldOpen = false;

	fileSelector = NULL;
	fileSelectorPath = new char[PATH_MAX];
	fileSelectorRecentPath = new char[PATH_MAX];
	strcpy(fileSelectorRecentPath, PathApplication);

	uiSetChanges = 0;
	uiQueryState = GUI_QUERY_CANCEL;
}
//-----------------------------------------------------------------------------
UserInterface::~UserInterface()
{
	debug("GUI", "Uninitializing, freeing...");

	if (fileSelector) {
		delete fileSelector;
		fileSelector = NULL;
	}
	if (fileSelectorPath) {
		delete[] fileSelectorPath;
		fileSelectorPath = NULL;
	}
	if (fileSelectorRecentPath) {
		delete[] fileSelectorRecentPath;
		fileSelectorRecentPath = NULL;
	}
	if (tapeDialog) {
		TapeBrowser->FreeFileList(&tapeDialog->entries, &tapeDialog->count);
		delete tapeDialog;
		tapeDialog = NULL;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DrawEmulatorWindow()
{
	static ImGuiWindowFlags window_flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImVec2 screen_size = Emulator->video->GetScreenSize();
	ImVec2 window_size = Emulator->video->GetWindowSize();
	ImVec2 border_offset = Emulator->video->GetBorderOffset();
	ImVec2 emulator_size = Emulator->video->GetScreenSize() / Emulator->video->GetMultiplier();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
	ImGui::Begin(PACKAGE_NAME, NULL, window_flags);

	isEmulatorWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootWindow);
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	ImRect winRect(
		window->DC.CursorPos,
		window->DC.CursorPos + window_size
	);
	ImRect emuRect(
		window->DC.CursorPos + border_offset,
		window->DC.CursorPos + border_offset + screen_size
	);

	window->DrawList->AddRectFilled(winRect.Min, winRect.Max, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 1.00f)));
	window->DrawList->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest);
	window->DrawList->AddImage(Emulator->video->GetScreenTexture(), emuRect.Min, emuRect.Max);
	window->DrawList->AddImage(Emulator->video->GetScalerTexture(), emuRect.Min, emuRect.Max);

	ImGui::InvisibleButton("Screen", screen_size + (border_offset * 2), ImGuiButtonFlags_MouseButtonMask_);
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(Settings->Mouse->hideCursor ? ImGuiMouseCursor_None : ImGuiMouseCursor_Arrow);

		int leftBtn = 0, rightBtn = 0, middleBtn = 0;
		ImVec2 mousePos = ImGui::GetMousePos();
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			leftBtn = 1;
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			leftBtn = -1;
		if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
			rightBtn = 1;
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
			rightBtn = -1;
		if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			middleBtn = 1;
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Middle))
			middleBtn = -1;

		Emulator->ActionMouseState(
			(int) mousePos.x, (int) mousePos.y,
			leftBtn, rightBtn, middleBtn
		);
	}
	else
		ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

	RedrawStatusBar(border_offset.x);

	ImGui::End();
	ImGui::PopStyleVar(1);
}
//-----------------------------------------------------------------------------
void UserInterface::MenuOpen(GUI_MENU_TYPE type, void *data)
{
	switch (type) {
		case GUI_TYPE_ABOUT:
			dialogAboutOpened = true;
			ImGui::OpenPopup("About");
			break;

		case GUI_TYPE_DISKIMAGES:
			dialogDiskImagesOpened = !dialogDiskImagesOpened;
			break;

		default:
			break;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::MenuClose()
{
}
//-----------------------------------------------------------------------------
void UserInterface::MenuCloseAll()
{
	ImGui::ClosePopupToLevel(0, true);
}
//-----------------------------------------------------------------------------
