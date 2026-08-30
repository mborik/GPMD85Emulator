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
#include "Emulator.h"
#include "imgui/imgui_internal.h"
#include "basefont.h"
//-----------------------------------------------------------------------------
UserInterface *GUI;
//-----------------------------------------------------------------------------
UserInterface::UserInterface()
{
	debug("GUI", "Initializing...");

	ledState = 0;
	iconState = 0;
	statusFPS = 0;
	statusPercentage = 0;
	computerModel[0] = '\0';

	isMenuHovered = false;
	isEmulatorWindowFocused = false;
	triggerMachineMenuOpen = false;
	dialogAboutOpened = false;

	queryDialogTitle = NULL;
	queryDialogMessage = NULL;
	queryDialogSaveType = false;
	queryDialogShouldOpen = false;

	fileSelector = NULL;
	fileSelectorPath = new char[PATH_MAX];
	fileSelectorRecentPath = new char[PATH_MAX];
	strcpy(fileSelectorRecentPath, PathApplication);

	screenInstance = NULL;
	InvokeSettingsChange = 0;
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
}
//-----------------------------------------------------------------------------
void UserInterface::InitFont(float size, bool oversample)
{
	ImGuiIO& io = ImGui::GetIO();

	ImFontConfig config;
	config.OversampleH = (ImS8) oversample;
	config.OversampleV = (ImS8) oversample;
	config.PixelSnapH = true;
	config.EllipsisChar = 0x2026;
	config.GlyphOffset = ImVec2(0.0f, -1.0f);

	io.Fonts->AddFontFromMemoryCompressedTTF(
		GPMD85Emulator_font_compressed_data,
		GPMD85Emulator_font_compressed_size,
		size, &config
	);
}
//-----------------------------------------------------------------------------
float UserInterface::GetMonoTextWidth(int textLength, float padding)
{
	if (textLength <= 0)
		return 0.0f;

	float charWidth = ImGui::CalcTextSize("W").x;
	return (charWidth * textLength) + (padding * 2.0f);
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

	if (!screenInstance)
		return;

	ImVec2 screen_size = screenInstance->GetScreenSize();
	ImVec2 window_size = screenInstance->GetWindowSize();
	ImVec2 border_offset = screenInstance->GetBorderOffset();
	ImVec2 emulator_size = screenInstance->GetScreenSize() / screenInstance->GetMultiplier();

	window_size.y += ImGui::GetTextLineHeightWithSpacing() + STATUSBAR_HEIGHT;

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
	window->DrawList->AddImage(screenInstance->GetScreenTexture(), emuRect.Min, emuRect.Max);
	window->DrawList->AddImage(screenInstance->GetScalerTexture(), emuRect.Min, emuRect.Max);

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
void UserInterface::Execute(TGuiElementType type, bool forceOpen)
{
	switch (type) {
		case GE_SCALE: {
			float scale = ((float) Settings->GUI->fontScale + 1.0f) * 0.5f;
			ImGui::GetStyle().FontScaleMain = scale;
			break;
		}

		case GE_MACHINE:
			triggerMachineMenuOpen = true;
			break;

		case GE_ABOUT:
			dialogAboutOpened = true;
			ImGui::OpenPopup("About");
			break;

		case GE_DISKIMAGES:
			Settings->GUI->dialogDiskImagesOpened = !Settings->GUI->dialogDiskImagesOpened;
			break;

		case GE_TAPEBROWSER:
			Settings->GUI->dialogTapeBrowserOpened = forceOpen || !Settings->GUI->dialogTapeBrowserOpened;
			break;

		default:
			break;
	}
}
//-----------------------------------------------------------------------------
