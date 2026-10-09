/*	CommonDialog.cpp: Part of GUI rendering class: Common popup dialogs
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
void UserInterface::DrawQueryDialog()
{
	if (queryDialogShouldOpen) {
		ImGui::OpenPopup("QueryDialog");
		queryDialogShouldOpen = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("QueryDialog", NULL,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

		if (queryDialogTitle && queryDialogTitle[0] != '\0') {
			ImGui::PushStyleColor(ImGuiCol_Text, Color[GCCol_DialogTitleText]);
			ImGui::SeparatorText(queryDialogTitle);
			ImGui::PopStyleColor();
		}

		ImGui::TextWrapped("%s", queryDialogMessage);
		ImGui::Separator();
		ImGui::Spacing();

		float button_width = ImMax(100.0f, GetMonoTextWidth(10, 8.0f));

		if (queryDialogSaveType) {
			if (ImGui::Button("Save", ImVec2(button_width, 0.0f))) {
				ImGui::CloseCurrentPopup();
				QueryDialogCallback(GUI_QUERY_SAVE);
			}
			ImGui::SameLine();
			if (ImGui::Button("Don't Save", ImVec2(button_width, 0.0f))) {
				ImGui::CloseCurrentPopup();
				QueryDialogCallback(GUI_QUERY_DONTSAVE);
			}
			ImGui::SameLine();
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("Cancel", ImVec2(button_width, 0.0f))) {
				ImGui::CloseCurrentPopup();
				QueryDialogCallback(GUI_QUERY_CANCEL);
			}
		}
		else if (queryDialogTitle && queryDialogTitle[0] != '\0') {
			if (ImGui::Button("Yes", ImVec2(button_width * 1.5, 0.0f))) {
				ImGui::CloseCurrentPopup();
				QueryDialogCallback(GUI_QUERY_YES);
			}
			ImGui::SameLine();
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("No", ImVec2(button_width * 1.5, 0.0f))) {
				ImGui::CloseCurrentPopup();
				QueryDialogCallback(GUI_QUERY_NO);
			}
		}
		else {
			ImGui::SetItemDefaultFocus();
			if (ImGui::Button("OK", ImVec2(button_width * 2.0f, 0.0f))) {
				ImGui::CloseCurrentPopup();
				QueryDialogCallback(GUI_QUERY_NO);
			}
		}

		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::QueryDialog(const char *title, const char *message, bool save)
{
	queryDialogTitle = title;
	queryDialogMessage = message;
	queryDialogSaveType = save;
	queryDialogShouldOpen = true;
}
//-----------------------------------------------------------------------------
void UserInterface::MessageBox(const char *text, ...)
{
	va_list va;
	va_start(va, text);
	vsprintf(msgbuffer, text, va);
	va_end(va);

	queryDialogTitle = NULL;
	queryDialogMessage = msgbuffer;
	queryDialogSaveType = false;
	queryDialogShouldOpen = true;
}
//-----------------------------------------------------------------------------
