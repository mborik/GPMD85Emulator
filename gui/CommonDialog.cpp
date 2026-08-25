/*	CommonDialog.cpp: Part of GUI rendering class: Common popup dialogs
	Copyright (c) 2011-2024 Martin Borik <mborik@users.sourceforge.net>

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
#include "UserInterface.h"
#include "Emulator.h"
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
void UserInterface::DiskImagesDialog()
{
	if (dialogDiskImagesOpened) {
		ImGui::Begin("Disk Images", &dialogDiskImagesOpened,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
		DiskImagesMenuItems();
		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::AboutDialog()
{
	static bool showAboutDialog = false;

	if (dialogAboutOpened) {
		ImGui::OpenPopup("About");
		dialogAboutOpened = false;
		showAboutDialog = true;
	}

	float button_width = 120.0f;
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("About", &showAboutDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
		ImGui::Text("%s v%s (c) %s", PACKAGE_NAME, VERSION, PACKAGE_YEAR);
		ImGui::Separator();

		ImGui::Text(
			"Open-source multi-platform\n"
			"emulator of the Tesla PMD 85,\n"
			"an 8-bit personal micro-computer\n"
			"produced in 80s of 20th century\n"
			"in former Czechoslovakia."
		);

		ImGui::Spacing();
		ImGui::TextUnformatted("Built with SDL2 +");
		ImGui::SameLine(0.0f, -8.0f);
		ImGui::TextLinkOpenURL("Dear ImGui", "https://github.com/ocornut/imgui");
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		ImGui::SameLine();
		ImGui::TextUnformatted(ImGui::GetVersion());

		ImGui::Spacing();
		ImGui::Bullet();
		ImGui::TextLinkOpenURL((PACKAGE_URL) + 8, PACKAGE_URL);
		ImGui::Bullet();
		const char *pmd85emu_url = "https://pmd85.borik.net";
		ImGui::TextLinkOpenURL(pmd85emu_url + 8, pmd85emu_url);

		ImGui::Spacing();
		ImGui::TextDisabled("Licensed under GNU/GPL version 3.");

		ImGui::Separator();
		ImGui::SetCursorPosX(
			ImGui::GetCursorPosX() +
			(ImGui::GetContentRegionAvail().x - button_width) * 0.5f
		);

		ImGui::SetItemDefaultFocus();
		if (ImGui::Button("OK", ImVec2(button_width, 0))) {
			ImGui::CloseCurrentPopup();
			showAboutDialog = false;
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
void UserInterface::DrawQueryDialog()
{
	if (queryDialogShouldOpen) {
		ImGui::OpenPopup("QueryDialog");
		queryDialogShouldOpen = false;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("QueryDialog", NULL,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

		if (queryDialogTitle && queryDialogTitle[0] != '\0') {
			ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4) ImColor::HSV(0.6f, 0.7f, 0.8f));
			ImGui::SeparatorText(queryDialogTitle);
			ImGui::PopStyleColor();
		}

		ImGui::TextWrapped("%s", queryDialogMessage);
		ImGui::Separator();
		ImGui::Spacing();

		if (queryDialogSaveType) {
			if (ImGui::Button("Save", ImVec2(100, 0))) {
				ImGui::CloseCurrentPopup();
				uiQueryCallback(GUI_QUERY_SAVE);
			}
			ImGui::SameLine();
			if (ImGui::Button("Don't Save", ImVec2(100, 0))) {
				ImGui::CloseCurrentPopup();
				uiQueryCallback(GUI_QUERY_DONTSAVE);
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(100, 0))) {
				ImGui::CloseCurrentPopup();
				uiQueryCallback(GUI_QUERY_CANCEL);
			}
		}
		else {
			if (ImGui::Button("Yes", ImVec2(150, 0))) {
				ImGui::CloseCurrentPopup();
				uiQueryCallback(GUI_QUERY_YES);
			}
			ImGui::SameLine();
			if (ImGui::Button("No", ImVec2(150, 0))) {
				ImGui::CloseCurrentPopup();
				uiQueryCallback(GUI_QUERY_NO);
			}
		}

		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
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
