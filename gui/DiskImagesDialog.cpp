/*	DiskImagesDialog.cpp: Part of GUI rendering class: Disk Images popup dialog
	Copyright (c) 2026 Martin Borik <martin@borik.net>

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
void UserInterface::DiskImagesMenuItems(bool inMenu)
{
	static char buf[FILENAME_MAX];
	static TSettings::SetPMD32Drive *drives[4] = {
		&Settings->PMD32->driveA,
		&Settings->PMD32->driveB,
		&Settings->PMD32->driveC,
		&Settings->PMD32->driveD
	};

	ImVec4 bbase, hover;
	for (char i = 0, letter = 'A'; i < 4; i++, letter++) {
		TSettings::SetPMD32Drive *drive = drives[i];
		const char *imagePath = ExtractFileName(drive->image);
		sprintf(buf, "Drive%c  %c: %s", letter, letter, imagePath ? imagePath : "[empty]");
		buf[6] = '\0';

		ImGui::PushID(buf);
		ImGui::SetNextItemAllowOverlap();
		if (ImGui::MenuItem(buf + 8))
			Emulator->ActionPMD32LoadDisk((int) i + 1);

		bbase = imagePath ? Color[GCCol_ButtonEject] : Color[GCCol_ButtonBase];
		hover = imagePath ? Color[GCCol_ButtonEjectHover] : Color[GCCol_ButtonBaseHover];
		ImGui::PushStyleColor(ImGuiCol_Button, bbase);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

		ImGui::SameLine();
		sprintf(buf, "Eject##%c", letter);
		if (ImGui::SmallButton(buf) && imagePath) {
			delete [] drive->image;
			drive->image = NULL;
			ProcessSettingsCallback.connect(&TEmulator::ActionPMD32Update, Emulator);
			InvokeSettingsChange |= PS_CLOSEALL;
		}

		bbase = drive->writeProtect ? Color[GCCol_ButtonWP] : Color[GCCol_ButtonBase];
		hover = drive->writeProtect ? Color[GCCol_ButtonWPHover] : Color[GCCol_ButtonBaseHover];
		ImGui::PushStyleColor(ImGuiCol_Button, bbase);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

		ImGui::SameLine();
		sprintf(buf, "\u2302##WP%c", letter);
		if (ImGui::SmallButton(buf) && imagePath) {
			drive->writeProtect = !drive->writeProtect;
			ProcessSettingsCallback.connect(&TEmulator::ActionPMD32Update, Emulator);
			InvokeSettingsChange |= PS_CLOSEALL;
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Drive %c Write Protect: %s", letter, drive->writeProtect ? "ON" : "OFF");

		ImGui::PopStyleColor(6);
		ImGui::PopID();
	};

}
//-----------------------------------------------------------------------------
void UserInterface::DrawDiskImagesDialog()
{
	if (Settings->GUI->dialogDiskImagesOpened) {
		ImGui::Begin("Disk Images", &Settings->GUI->dialogDiskImagesOpened,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
		DiskImagesMenuItems();
		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
