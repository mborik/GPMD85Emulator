/*	Menu.cpp: Part of GUI rendering class: Menu drawing and handling
	Copyright (c) 2011-2026 Martin Borik <mborik@users.sourceforge.net>

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
#define MOD_KEY(k) "\u00A4+" k
#define MOD_SHIFT(k) "\u00A4\u0088+" k
//-----------------------------------------------------------------------------
bool UserInterface::OnMenuLeave()
{
	static bool wasInMenu = false;

	bool isCurrentlyInMenu = InMenu();
	bool hasLeft = (wasInMenu && !isCurrentlyInMenu);

	wasInMenu = isCurrentlyInMenu;

	return hasLeft;
}
//-----------------------------------------------------------------------------
void UserInterface::DrawMenu()
{
	if (ImGui::BeginMainMenuBar()) {
		isMenuHovered = ImGui::IsWindowFocused(ImGuiHoveredFlags_ChildWindows);

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Tape")) { }
			if (ImGui::MenuItem("Open Tape\u0085", MOD_KEY("F2"))) { }
			if (ImGui::MenuItem("Save Tape\u0085", MOD_SHIFT("F2"))) { }
			if (ImGui::MenuItem("Tape Browser", MOD_KEY("T"))) { }
			ImGui::Separator();
			ImGui::MenuItem("Disk Images", MOD_KEY("F6"), &dialogDiskImagesOpened);
			ImGui::Separator();
			if (ImGui::MenuItem("Open Snapshot\u0085", MOD_KEY("F7"))) { }
			if (ImGui::MenuItem("Create Snapshot\u0085", MOD_SHIFT("F7"))) { }
			ImGui::Separator();
			if (ImGui::MenuItem("Load to Memory\u0085", MOD_KEY("F11"))) { }
			if (ImGui::MenuItem("Save Memory\u0085", MOD_SHIFT("F11"))) { }
			ImGui::Separator();
			if (ImGui::MenuItem("Save Screenshot\u0085")) { }
			ImGui::Separator();
			if (ImGui::MenuItem("About\u0085", MOD_KEY("F1"))) {
				MenuOpen(GUI_TYPE_ABOUT);
			}
			if (ImGui::MenuItem("Quit", MOD_KEY("F4"))) {
				Emulator->isActive = false;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Display")) {
			if (ImGui::BeginMenu("Screen Size")) {
				if (ImGui::MenuItem("100%", MOD_KEY("1")), Settings->Screen->size == DM_NORMAL) {
					Settings->Screen->size = DM_NORMAL;
					uiSetChanges |= PS_SCREEN_SIZE;
				}
				if (ImGui::MenuItem("200%", MOD_KEY("2")), Settings->Screen->size == DM_DOUBLESIZE) {
					Settings->Screen->size = DM_DOUBLESIZE;
					uiSetChanges |= PS_SCREEN_SIZE;
				}
				if (ImGui::MenuItem("300%", MOD_KEY("3")), Settings->Screen->size == DM_TRIPLESIZE) {
					Settings->Screen->size = DM_TRIPLESIZE;
					uiSetChanges |= PS_SCREEN_SIZE;
				}
				if (ImGui::MenuItem("400%", MOD_KEY("4")), Settings->Screen->size == DM_QUADRUPLESIZE) {
					Settings->Screen->size = DM_QUADRUPLESIZE;
					uiSetChanges |= PS_SCREEN_SIZE;
				}
				if (ImGui::MenuItem("500%", MOD_KEY("5")), Settings->Screen->size == DM_QUINTUPLESIZE) {
					Settings->Screen->size = DM_QUINTUPLESIZE;
					uiSetChanges |= PS_SCREEN_SIZE;
				}
				if (ImGui::MenuItem("Fullscreen", MOD_KEY("F"), Settings->Screen->size == DM_FULLSCREEN, false)) { }
				ImGui::Separator();
				if (ImGui::BeginMenu("Border Size")) {
					if (ImGui::SliderInt("##border", &Settings->Screen->border, 0, 9,
						"%d", ImGuiSliderFlags_AlwaysClamp)) {

						uiSetChanges |= PS_SCREEN_SIZE;
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			if (ImGui::BeginMenu("Color Mode")) {
				ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
				if (ImGui::MenuItem("Monochromatic", MOD_KEY("M"), Settings->Screen->colorProfile == CP_MONO)) {
					Settings->Screen->colorProfile = CP_MONO;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Standard", MOD_KEY("M"), Settings->Screen->colorProfile == CP_STANDARD)) {
					Settings->Screen->colorProfile = CP_STANDARD;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Color", MOD_KEY("C"), Settings->Screen->colorProfile == CP_COLOR)) {
					Settings->Screen->colorProfile = CP_COLOR;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("ColorAce", MOD_KEY("C"), Settings->Screen->colorProfile == CP_COLORACE)) {
					Settings->Screen->colorProfile = CP_COLORACE;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				ImGui::Separator();
				if (ImGui::BeginMenu("Color Palette", Settings->Screen->colorProfile == CP_COLOR)) {
					if (ImGui::MenuItem("RGBM", NULL, Settings->Screen->colorPalette == CL_RGB)) {
						Settings->Screen->colorPalette = CL_RGB;
						uiSetChanges |= PS_SCREEN_MODE;
					}
					if (ImGui::MenuItem("VideoOut", NULL, Settings->Screen->colorPalette == CL_VIDEO)) {
						Settings->Screen->colorPalette = CL_VIDEO;
						uiSetChanges |= PS_SCREEN_MODE;
					}
					if (ImGui::MenuItem("Custom Colors", NULL, Settings->Screen->colorPalette == CL_DEFINED)) {
						Settings->Screen->colorPalette = CL_DEFINED;
						uiSetChanges |= PS_SCREEN_MODE;
					}
					ImGui::Separator();
					AttributeMenuItems(Settings->Screen->colorPalette == CL_DEFINED);
					ImGui::EndMenu();
				}
				ImGui::PopItemFlag();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Scanliner")) {
				if (ImGui::MenuItem("LCD Emulation", MOD_KEY("L"), Settings->Screen->lcdMode)) {
					Settings->Screen->lcdMode = true;
					Settings->Screen->halfPass = HP_OFF;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Half-Pass 0%", MOD_KEY("6"), Settings->Screen->halfPass == HP_0)) {
					Settings->Screen->lcdMode = false;
					Settings->Screen->halfPass = HP_0;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Half-Pass 25%", MOD_KEY("7"), Settings->Screen->halfPass == HP_25)) {
					Settings->Screen->lcdMode = false;
					Settings->Screen->halfPass = HP_25;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Half-Pass 50%", MOD_KEY("8"), Settings->Screen->halfPass == HP_50)) {
					Settings->Screen->lcdMode = false;
					Settings->Screen->halfPass = HP_50;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Half-Pass 75%", MOD_KEY("9"), Settings->Screen->halfPass == HP_75)) {
					Settings->Screen->lcdMode = false;
					Settings->Screen->halfPass = HP_75;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				if (ImGui::MenuItem("Pixel Precise", MOD_KEY("0"), Settings->Screen->halfPass == HP_OFF)) {
					Settings->Screen->lcdMode = false;
					Settings->Screen->halfPass = HP_OFF;
					uiSetChanges |= PS_SCREEN_MODE;
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Emulation")) {
			if (ImGui::MenuItem("Debugger\u0085", MOD_KEY("F12"), false, false)) { }
			if (ImGui::MenuItem("Pause", MOD_KEY("F3"), Settings->isPaused)) {
				Settings->isPaused = !Settings->isPaused;
			}
			if (ImGui::BeginMenu("Speed")) {
				float speedValue = (Settings->emulationSpeed * 100.0f);
				if (ImGui::SliderFloat("##speed", &speedValue, 10.0f, 1000.0f, "%.0f%%",
					ImGuiSliderFlags_Logarithmic | ImGuiSliderFlags_AlwaysClamp)) {

					Settings->emulationSpeed = ((double) speedValue / 100.0f);
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Reset", MOD_KEY("F5"))) {
				uiCallback.connect(Emulator, &TEmulator::ActionReset);
				uiSetChanges |= PS_CLOSEALL;
			}
			if (ImGui::MenuItem("Hard Restart", MOD_SHIFT("F5"))) {
				uiCallback.connect(Emulator, &TEmulator::ActionHardReset);
				uiSetChanges |= PS_CLOSEALL;
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("Sound")) {
				if (ImGui::MenuItem("Mute", MOD_KEY("F8"), Settings->Sound->mute)) {
					Settings->Sound->mute = !Settings->Sound->mute;
					uiSetChanges |= PS_SOUND;
				}
				if (ImGui::BeginMenu("Volume", !Settings->Sound->mute)) {
					ImGui::SliderInt("##volume", &Settings->Sound->volume, 2, 127,
						"%d", ImGuiSliderFlags_AlwaysClamp);
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Keyboard")) {
				if (ImGui::MenuItem("Swap Z/Y Keys", NULL, &Settings->Keyboard->changeZY)) {
					uiSetChanges |= PS_CONTROLS;
				}
				if (ImGui::MenuItem("Use Numeric Keypad", NULL, &Settings->Keyboard->useNumpad)) {
					uiSetChanges |= PS_CONTROLS;
				}
				if (ImGui::MenuItem("Extended Control Keys", "on Mato",
					&Settings->Keyboard->useMatoCtrl, Settings->CurrentModel->type == CM_MATO)) {

					uiSetChanges |= PS_CONTROLS;
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save Settings", NULL, false, !Settings->fixedSettings)) {
				Settings->storeSettings();
			}
			if (ImGui::MenuItem("Autosave on Exit", NULL, Settings->autosaveSettings, !Settings->fixedSettings)) {
				Settings->autosaveSettings = !Settings->autosaveSettings;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Machine")) {
			bool accessibleForPMD85 = Settings->CurrentModel->type <= CM_V3;
			char *currentFile = new char[FILENAME_MAX + 2];

			ImGui::SeparatorText("Computer Model");
			MachineMenuItem("PMD 85-1", CM_V1);
			MachineMenuItem("PMD 85-2", CM_V2);
			MachineMenuItem("PMD 85-2A", CM_V2A);
			MachineMenuItem("PMD 85-3", CM_V3);
			MachineMenuItem("Didaktik Alfa", CM_ALFA);
			MachineMenuItem("Didaktik Alfa 2", CM_ALFA2);
			MachineMenuItem("Consul 2717", CM_C2717);
			MachineMenuItem("Mato", CM_MATO);

			ImGui::SeparatorText("Memory Configuration");
			ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);

			sprintf(currentFile, "[%s]", ExtractFileName(Settings->CurrentModel->romFile));
			if (ImGui::MenuItem("System ROM File\u0085", currentFile)) { }

			if ((Settings->CurrentModel->type <= CM_V2A) &&
				ImGui::MenuItem("Split 8kB ROM", "on 8000/A000", Settings->CurrentModel->romSplit8kMode)) {

				Settings->CurrentModel->romSplit8kMode = !Settings->CurrentModel->romSplit8kMode;
				uiSetChanges |= PS_MACHINE;
			}
			if ((Settings->CurrentModel->type == CM_V3) &&
				ImGui::MenuItem("Compatibility Mode", "JUMP FFF0", Settings->CurrentModel->compatibilityMode)) {

				Settings->CurrentModel->compatibilityMode = !Settings->CurrentModel->compatibilityMode;
				uiSetChanges |= PS_MACHINE;
			}
			if ((Settings->CurrentModel->type == CM_V2A || Settings->CurrentModel->type == CM_V3) &&
				ImGui::MenuItem("256kB Memory Expansion", NULL, Settings->CurrentModel->ramExpansion256k)) {

				Settings->CurrentModel->ramExpansion256k = !Settings->CurrentModel->ramExpansion256k;
				uiSetChanges |= PS_MACHINE;
			}
			if ((Settings->CurrentModel->type == CM_MATO) &&
				ImGui::MenuItem("Fix AllRAM 64kB Mode", NULL, Settings->CurrentModel->matoAllRAM64k)) {

				Settings->CurrentModel->matoAllRAM64k = !Settings->CurrentModel->matoAllRAM64k;
				uiSetChanges |= PS_MACHINE;
			}

			ImGui::Separator();

			if (ImGui::MenuItem("ROM Module Inserted", NULL,
				accessibleForPMD85 ? Settings->CurrentModel->romModuleInserted : false, accessibleForPMD85)) {

				Settings->CurrentModel->romModuleInserted = !Settings->CurrentModel->romModuleInserted;
				uiSetChanges |= PS_MACHINE | PS_PERIPHERALS;
			}
			if (ImGui::BeginMenu("ROM Module Package", Settings->CurrentModel->romModuleInserted)) {
				const char *name = Settings->CurrentModel->romModule->name;
				for (int i = 0; i < Settings->romPackagesCount; i++) {
					if (ImGui::RadioButton(Settings->RomPackages[i]->name,
							(strcmp(Settings->RomPackages[i]->name, name) == 0))) {

						Settings->CurrentModel->romModule = Settings->RomPackages[i];
						uiSetChanges |= PS_MACHINE | PS_PERIPHERALS;
					}
				}
				ImGui::EndMenu();
			}
			bool itemAccessible = accessibleForPMD85 && Settings->CurrentModel->romModuleInserted;
			if (ImGui::MenuItem("ROM MEGAmodule", NULL,
				itemAccessible ? Settings->CurrentModel->megaModuleEnabled : false, itemAccessible)) {

				Settings->CurrentModel->megaModuleEnabled = !Settings->CurrentModel->megaModuleEnabled;
				uiSetChanges |= PS_MACHINE | PS_PERIPHERALS;
			}
			char *mrmFile = Settings->CurrentModel->mrmFile ? currentFile : NULL;
			if (mrmFile)
				sprintf(currentFile, "[%s]", ExtractFileName(Settings->CurrentModel->mrmFile));
			if (ImGui::MenuItem("ROM MEGAmodule Image\u0085", mrmFile, false, itemAccessible)) { }
			ImGui::PopItemFlag();

			itemAccessible = accessibleForPMD85 || Settings->CurrentModel->type == CM_V3;
			ImGui::SeparatorText("Peripherals");
			if (ImGui::BeginMenu("Joystick 4004/482")) {
				// TODO joysticks are too complex :/
				ImGui::TextDisabled("TODO");
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Mouse 602", itemAccessible)) {
				bool state = Settings->Mouse->type == MT_M602;
				if (ImGui::MenuItem("Connected", NULL, &state)) {
					Settings->Mouse->type = state ? MT_M602 : MT_NONE;
					uiSetChanges |= PS_PERIPHERALS;
				}
				ImGui::Separator();
				ImGui::MenuItem("Hide Mouse Cursor", NULL, &Settings->Mouse->hideCursor);
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu("PMD 32 Disk Drive", itemAccessible)) {
				bool state = Settings->PMD32->connected;
				if (ImGui::MenuItem("Connected", NULL, &state)) {
					Settings->PMD32->connected = state;
					uiSetChanges |= PS_PERIPHERALS;
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Extended Commands", NULL, &Settings->PMD32->extraCommands, state)) {
					uiSetChanges |= PS_PERIPHERALS;
				}
				char *sdRoot = Settings->PMD32->sdRoot ? currentFile : NULL;
				if (sdRoot)
					sprintf(currentFile, "[%s]", ExtractFileName(Settings->PMD32->sdRoot));
				if (ImGui::MenuItem("Virtual SD-Card Directory\u0085", sdRoot, false,
					state && Settings->PMD32->extraCommands)) { }

				ImGui::Separator();
				DiskImagesMenuItems();

				ImGui::EndMenu();
			}
			ImGui::Separator();

			if (ImGui::MenuItem("MIF 85 Sound Interface", NULL,
				accessibleForPMD85 ? Settings->Sound->ifMIF85 : false, accessibleForPMD85)) {

				Settings->Sound->ifMIF85 = !Settings->Sound->ifMIF85;
				uiSetChanges |= PS_PERIPHERALS;
			}

			delete [] currentFile;
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::MachineMenuItem(const char *name, TComputerModel model)
{
	if (ImGui::RadioButton(name, Settings->CurrentModel->type == model)) {
		for (int i = 0; i < Settings->modelsCount; i++) {
			if (Settings->AllModels[i]->type == model) {
				Settings->CurrentModel = Settings->AllModels[i];
				break;
			}
		}

		uiSetChanges |= PS_MACHINE;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::AttributeMenuItems(bool enabled)
{
	static const char *colorNames[] = {
		"BLACK", "MAROON", "GREEN", "OLIVE",
		"NAVY", "PURPLE", "TEAL", "GRAY",
		"SILVER", "RED", "LIME", "YELLOW",
		"BLUE", "FUCHSIA", "AQUA", "WHITE"
	};

	int colors[4] = { 0, 0, 0, 0 };
	switch (Settings->Screen->colorPalette) {
		case CL_RGB:
			colors[0] = TColor::LIME;
			colors[1] = TColor::RED;
			colors[2] = TColor::BLUE;
			colors[3] = TColor::FUCHSIA;
			break;

		case CL_VIDEO:
			colors[0] = TColor::WHITE;
			colors[1] = TColor::LIME;
			colors[2] = TColor::RED;
			colors[3] = TColor::MAROON;
			break;

		case CL_DEFINED:
			colors[0] = Settings->Screen->attr00;
			colors[1] = Settings->Screen->attr01;
			colors[2] = Settings->Screen->attr10;
			colors[3] = Settings->Screen->attr11;
			break;
	}

	float sz = ImGui::GetTextLineHeight();
	char* attrName = new char[16];

	for (int n = 0; n < 4; n++) {
		sprintf(attrName, "Attribute %d%d", (n & 2) >> 1, n & 1);
		ImVec2 p = ImGui::GetCursorScreenPos();
		ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz),
			(ImU32) globalPalette[colors[n]]);
		ImGui::Dummy(ImVec2(sz, sz));
		ImGui::SameLine();
		if (ImGui::BeginMenu(attrName, enabled)) {
			for (int i = 0; i < 16; i++) {
				ImVec2 p = ImGui::GetCursorScreenPos();
				ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz), (ImU32) globalPalette[i]);
				ImGui::Dummy(ImVec2(sz, sz));
				ImGui::SameLine();
				ImGui::MenuItem(colorNames[i], NULL, colors[n] == i);
			}
			ImGui::EndMenu();
		}
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DiskImagesMenuItems()
{
	static char path[FILENAME_MAX];
	ImVec4 bbase, hover;

	auto DiskImagesMenuItem = [&](TSettings::SetPMD32Drive drive, char letter) {
		const char *imagePath = ExtractFileName(drive.image);
		sprintf(path, "Drive%c\t%c: %s", letter, letter, imagePath ? imagePath : "[empty]");
		path[6] = '\0';

		ImGui::PushID(path);
		ImGui::SetNextItemAllowOverlap();
		ImGui::MenuItem(path + 7);
		ImGui::SameLine();

		bbase = imagePath ? ImColor::HSV(0.6f, 0.7f, 0.8f) : ImColor::HSV(0.5f, 0.2f, 0.2f);
		hover = imagePath ? ImColor::HSV(0.6f, 0.9f, 1.0f) : ImColor::HSV(0.5f, 0.2f, 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Button, bbase);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

		if (ImGui::SmallButton("Eject")) {
			drive.image = NULL;
			uiSetChanges |= PS_PERIPHERALS;
		}

		bbase = drive.writeProtect ? ImColor::HSV(0.0f, 0.6f, 0.6f) : ImColor::HSV(0.5f, 0.2f, 0.2f);
		hover = drive.writeProtect ? ImColor::HSV(0.0f, 0.8f, 0.8f) : ImColor::HSV(0.5f, 0.2f, 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Button, bbase);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

		ImGui::SameLine();
		if (ImGui::SmallButton("WP")) {
			drive.writeProtect = !drive.writeProtect;
			uiSetChanges |= PS_PERIPHERALS;
		}

		ImGui::PopStyleColor(6);
		ImGui::PopID();
	};

	DiskImagesMenuItem(Settings->PMD32->driveA, 'A');
	DiskImagesMenuItem(Settings->PMD32->driveB, 'B');
	DiskImagesMenuItem(Settings->PMD32->driveC, 'C');
	DiskImagesMenuItem(Settings->PMD32->driveD, 'D');
}
//-----------------------------------------------------------------------------
