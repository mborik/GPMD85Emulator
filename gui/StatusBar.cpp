/*	StatusBar.cpp: Part of GUI rendering class: StatusBar rendering
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
#include "UserInterface.h"
#include "Emulator.h"
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
void UserInterface::RedrawStatusBar(float horizontalPadding)
{
	static const char *driveLetters[] = {"A", "B", "C", "D"};
	static BYTE pauseBlinker = 0;

	const ImGuiStyle& style = ImGui::GetStyle();
	const ImVec2 buttonSize = ImVec2(14.0f, 14.0f);
	const float statusWidth = 150.0f;
	const float iconsWidth = buttonSize.x * 6;
	float unscaleFactor = style.FontSizeBase * (1.0f / style.FontScaleMain);
	float width = ImGui::GetContentRegionAvail().x - (horizontalPadding * 2);
	float progressWidth = width - statusWidth - iconsWidth;
	float sameLine = horizontalPadding;
	const ImVec2 progressBarSize = ImVec2(progressWidth, 3.0f);

	ImGui::PushFont(NULL, unscaleFactor);
	ImGui::BeginGroup();
	if (horizontalPadding > 0.0f) {
		ImGui::SetNextItemWidth(horizontalPadding);
		ImGui::TextUnformatted("");
		ImGui::SameLine(sameLine);
	}

	sameLine += statusWidth;
	ImGui::SetNextItemWidth(statusWidth);

//	status text, cpu meter and blinking pause...
	if (statusPercentage < 0) {
		ImVec4 color(0.875f, 0.1f, 0.3f, (pauseBlinker < 10) ? 1.0f : 0.1f);
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted("PAUSED");
		ImGui::PopStyleColor();

		if (pauseBlinker++ >= 16)
			pauseBlinker = 0;
	}
	else if (statusPercentage > 0) {
		ImGui::TextDisabled("%sFPS:%d CPU:%d%%", computerModel, statusFPS, statusPercentage);
	}
	else {
		ImGui::TextUnformatted("");
	}

	ImGui::SameLine(sameLine);
	ImGui::SetNextItemWidth(progressWidth);
	sameLine += progressWidth + buttonSize.x;

	ImGui::BeginGroup();
	ImGui::Dummy(progressBarSize);

//	tape progress bar...
	TTapeBrowser::TProgressBar *progress = TapeBrowser->ProgressBar;
	if (progressWidth > 50.0f && !Settings->GUI->dialogTapeBrowserOpened && *progress->Active) {
		ImGui::PushID("##progress");
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.16f, 0.4f, 0.2f, 1.0f));
		ImGui::ProgressBar(((float) progress->Position / (float) progress->Max), progressBarSize, "");
		ImGui::PopStyleColor(2);
		ImGui::PopID();
	}

	ImGui::EndGroup();

	ImGui::SameLine(sameLine);
	sameLine += buttonSize.x + 8.0f;

	ImGui::BeginDisabled();

//	tape/disk icon...
	SetButtonColor(iconState);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
	ImGui::PushFont(NULL, 10.0f);
	ImGui::PushID("##device");

	switch (iconState) {
		case 1: case 5:
			ImGui::Button(driveLetters[0], buttonSize);
			break;
		case 2: case 6:
			ImGui::Button(driveLetters[1], buttonSize);
			break;
		case 3: case 7:
			ImGui::Button(driveLetters[2], buttonSize);
			break;
		case 4: case 8:
			ImGui::Button(driveLetters[3], buttonSize);
			break;
		case 9: case 10:
			ImGui::ArrowButtonEx("::tapeicon", ImGuiDir_Right, buttonSize, 0);
			break;
		default:
			ImGui::InvisibleButton("::noicon", buttonSize);
			break;
	}

	ImGui::PopID();
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

//	control LEDs on right side...
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

	for (BYTE mask = 1, btn = 1; btn < 4; mask <<= 1, btn++) {
		ImGui::SameLine(sameLine);
		sameLine += buttonSize.x + 2.0f;

		ImGui::PushID(btn);
		SetButtonColor((ledState & mask) ? btn + 10 : 0);
		ImGui::Button("", buttonSize);
		ImGui::PopStyleColor(3);
		ImGui::PopID();
	}

	ImGui::PopStyleVar(2);
	ImGui::EndDisabled();
	ImGui::EndGroup();
	ImGui::PopFont();
}
//-----------------------------------------------------------------------------
void UserInterface::SetButtonColor(int icon)
{
	static const ImVec4 blue(0.25f, 0.25f, 0.5f, 1.0f);
	static const ImVec4 red(0.75f, 0.0f, 0.0f, 1.0f);
	static const ImVec4 yellow(0.75f, 0.75f, 0.0f, 1.0f);
	static const ImVec4 grey(0.5f, 0.5f, 0.5f, 1.0f);

	ImVec4 result;
	switch (icon) {
		case 11:
			result = yellow;
			break;
		case 5: case 6: case 7: case 8: case 10: case 12:
			result = red;
			break;
		case 1: case 2: case 3: case 4: case 13:
			result = blue;
			break;
		case 9:
			result = grey;
			break;
		default:
			result = grey;
			result.w = 0.2f;
			break;
	}

	ImGui::PushStyleColor(ImGuiCol_Button, result);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, result);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, result);
}
//-----------------------------------------------------------------------------
void UserInterface::SetLedState(int led)
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
void UserInterface::SetIconState(int icon)
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
void UserInterface::SetComputerModel(TComputerModel model)
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
			modelName = "α1"; break;
		case CM_ALFA2:
			modelName = "α2"; break;
		case CM_C2717:
			modelName = "C2717"; break;
		case CM_MATO:
			modelName = "Maťo"; break;
		default:
			break;
	}

	if (modelName)
		sprintf(computerModel, "%s: ", modelName);
	else
		computerModel[0] = '\0';
}
//---------------------------------------------------------------------------
