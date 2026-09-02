/*	AboutDialog.cpp: Part of GUI rendering class: About popup dialog
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
#define URL_GITHUB PACKAGE_URL
#define URL_PMD85EMU "https://pmd85.borik.net"
#define URL_DEARIMGUI "https://github.com/ocornut/imgui"
//-----------------------------------------------------------------------------
void UserInterface::DrawAboutDialog()
{
	static bool showAboutDialog = false;

	if (dialogAboutOpened) {
		ImGui::OpenPopup("About");
		dialogAboutOpened = false;
		showAboutDialog = true;
	}

	float button_width = 120.0f;
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("About", &showAboutDialog, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));
		ImGui::Text("%s v%s © %s", PACKAGE_NAME, VERSION, PACKAGE_YEAR);
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
		ImGui::TextLinkOpenURL("Dear ImGui", URL_DEARIMGUI);
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

		ImGui::SameLine();
		ImGui::TextUnformatted(ImGui::GetVersion());

		ImGui::Spacing();
		ImGui::Bullet();
		ImGui::TextLinkOpenURL((PACKAGE_URL) + 8, PACKAGE_URL);
		ImGui::Bullet();

		const char *pmd85emu_url = URL_PMD85EMU;
		ImGui::TextLinkOpenURL(pmd85emu_url + 8, pmd85emu_url);

		ImGui::Spacing();
		ImGui::TextDisabled("Licensed under the MIT License.");

		ImGui::Separator();
		ImGui::SetCursorPosX(
			ImGui::GetCursorPosX() +
			(ImGui::GetContentRegionAvail().x - button_width) * 0.5f
		);

		ImGui::SetItemDefaultFocus();
		if (ImGui::Button("OK", ImVec2(button_width, 0.0f))) {
			ImGui::CloseCurrentPopup();
			showAboutDialog = false;
		}

		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
}
//-----------------------------------------------------------------------------
