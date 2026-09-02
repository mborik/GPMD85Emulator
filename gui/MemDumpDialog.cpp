/*	MemDumpDialog.cpp: Part of GUI rendering class: Memory read/write dialogs
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
void UserInterface::DrawMemDumpDialog()
{
	static bool readDialogOpened = false, writeDialogOpened = false;

	if (memDumpDialogShouldOpen) {
		ImGui::OpenPopup(memDumpDialogSaveType ? "Dump Memory Content to File" : "Load File to Memory");
		memDumpDialogShouldOpen = false;

		if (memDumpDialogSaveType)
			writeDialogOpened = true;
		else
			readDialogOpened = true;

		mdBlockStart = (int) Settings->MemoryBlock->start;
		mdBlockLength = (int) Settings->MemoryBlock->length;
		mdAutorunAddr = (int) Settings->MemoryBlock->autorunAddr;
		mdEx256pg = (int) Settings->MemoryBlock->ex256pg;
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Load File to Memory", &readDialogOpened,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

		MemDumpDialogContent(false);
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Dump Memory Content to File", &writeDialogOpened,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

		MemDumpDialogContent(true);
		ImGui::EndPopup();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::MemDumpDialogContent(bool saveType)
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 8.0f));

	static char buf[FILENAME_MAX];
	bool fileExists = Settings->MemoryBlock->fileName && FileExists(Settings->MemoryBlock->fileName);
	const char *imagePath = ExtractFileName(Settings->MemoryBlock->fileName);

	if (!saveType && !fileExists)
		imagePath = NULL;

	ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoSavedSettings;
	sprintf(buf, "%s##memdump", imagePath ? imagePath : "[choose file]");

	float baseWidth = ImMax(100.0f, GetMonoTextWidth(10, 8.0f));
	float tableRowHeight = ImGui::GetFrameHeight() * 2.5f;

	ImGui::SetNextItemAllowOverlap();
	ImGui::TextUnformatted("Selected file:");

	if (!saveType) {
		ImVec4 bbase = fileExists ? ImColor::HSV(0.35f, 1.0f, 0.6f, 0.9f) : ImColor::HSV(0.5f, 0.2f, 0.2f);
		ImVec4 hover = fileExists ? ImColor::HSV(0.35f, 1.0f, 0.6f, 0.5f) : ImColor::HSV(0.5f, 0.2f, 0.3f);
		ImGui::PushStyleColor(ImGuiCol_Button, bbase);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

		ImGui::SameLine((baseWidth * 3.0f) + 1.0f);
		if (ImGui::SmallButton("@##mdrefresh") && fileExists) {
			Settings->MemoryBlock->length = std::filesystem::file_size(Settings->MemoryBlock->fileName);
			mdBlockLength = (int) Settings->MemoryBlock->length;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
			ImGui::SetTooltip("Update block length");

		ImGui::PopStyleColor(3);
	}

	ImGui::SetNextItemWidth((baseWidth * 3.0f) + ImGui::GetStyle().ItemSpacing.x * 2.0f);
	if (ImGui::BeginCombo("##filename", buf, ImGuiComboFlags_NoArrowButton)) {
		Emulator->ActionRawFile(saveType);
		ImGui::EndCombo();
	}

	ImGui::Separator();

	ImGui::BeginDisabled(!saveType && !fileExists);
	if (ImGui::BeginTable("##mdtable", 3, tableFlags)) {
		static WORD stepperWord1 = 1, stepperWord2 = 256;
		static BYTE stepperByte1 = 1, stepperByte2 = 16;
		static const char *ramromItems[] = { "RAM", "ROM" };
		static const char *remapItems[] = { "NO", "YES" };
		static const char *c2717tooltip = "Buffer next to VideoRAM\nis mapped as a contiguous\narea from #C000 to #CFFF";

		const char *scalarWordFmt = Settings->MemoryBlock->hex ? "%04X" : "%hu";
		const char *scalarByteFmt = Settings->MemoryBlock->hex ? "%02X" : "%hu";

		ImGui::TableNextRow(ImGuiTableRowFlags_None, tableRowHeight);

		ImGui::TableSetColumnIndex(0);
		ImGui::BeginGroup();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::TextUnformatted("Start:");
		ImGui::SetNextItemWidth(baseWidth);
		ImGui::InputScalar("##blkstart", ImGuiDataType_U16, &mdBlockStart, &stepperWord1, &stepperWord2, scalarWordFmt);
		ImGui::EndGroup();

		ImGui::TableSetColumnIndex(1);
		ImGui::BeginGroup();
		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::TextUnformatted("Length:");
		ImGui::SetNextItemWidth(baseWidth);
		ImGui::InputScalar("##blklength", ImGuiDataType_U16, &mdBlockLength, &stepperWord1, &stepperWord2, scalarWordFmt);
		ImGui::EndGroup();

		if (!saveType) {
			ImGui::TableSetColumnIndex(2);
			ImGui::BeginGroup();
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::Checkbox("Autorun", &Settings->MemoryBlock->autorun);
			ImGui::BeginDisabled(!Settings->MemoryBlock->autorun);
			ImGui::SetNextItemWidth(baseWidth);
			ImGui::InputScalar("##autorun-addr", ImGuiDataType_U16, &mdAutorunAddr, &stepperWord1, &stepperWord2, scalarWordFmt);
			ImGui::EndDisabled();
			ImGui::EndGroup();

			ImGui::TableNextRow(ImGuiTableRowFlags_None, tableRowHeight);

			ImGui::TableSetColumnIndex(0);
			ImGui::BeginGroup();
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::TextUnformatted("Source:");

			bool hasAllRAM = Emulator->IsAllRAMMemory();
			int ramromIdx = (int) (hasAllRAM ? Settings->MemoryBlock->rom : 0);
			ImGui::BeginDisabled(!hasAllRAM);
			if (ImGui::SliderInt("##ramrom", &ramromIdx, 0, 1, ramromItems[ramromIdx], ImGuiSelectableFlags_SpanAvailWidth))
				Settings->MemoryBlock->rom = (bool) ramromIdx;
			ImGui::EndDisabled();
			ImGui::EndGroup();

			if (Settings->CurrentModel->type == CM_C2717) {
				ImGui::TableSetColumnIndex(1);
				ImGui::BeginGroup();
				ImGui::Dummy(ImVec2(0.0f, 2.0f));
				ImGui::TextUnformatted("Remapping:");
				int remapIdx = (int) Settings->MemoryBlock->remapping;
				if (ImGui::SliderInt("##remap", &remapIdx, 0, 1, remapItems[remapIdx], ImGuiSelectableFlags_SpanAvailWidth))
					Settings->MemoryBlock->remapping = (bool) remapIdx;
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
					ImGui::SetTooltip("%s", c2717tooltip);
				ImGui::EndGroup();
			}
			if (Settings->CurrentModel->ramExpansion256k) {
				ImGui::TableSetColumnIndex(2);
				ImGui::BeginGroup();
				ImGui::Dummy(ImVec2(0.0f, 4.0f));
				ImGui::TextUnformatted("256k Page:");
				ImGui::SetNextItemWidth(baseWidth);
				ImGui::InputScalar("##ex256og", ImGuiDataType_U8, &mdEx256pg, &stepperByte1, &stepperByte2, scalarByteFmt);
				ImGui::EndGroup();
			}
		}
		else if (Settings->CurrentModel->type == CM_C2717) {
			ImGui::TableSetColumnIndex(2);
			ImGui::BeginGroup();
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::TextUnformatted("Remapping:");
			int remapIdx = (int) Settings->MemoryBlock->remapping;
			if (ImGui::SliderInt("##remap", &remapIdx, 0, 1, remapItems[remapIdx], ImGuiSelectableFlags_SpanAvailWidth))
				Settings->MemoryBlock->remapping = (bool) remapIdx;
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
				ImGui::SetTooltip("%s", c2717tooltip);
			ImGui::EndGroup();
		}

		ImGui::EndTable();
	}

	ImGui::Separator();
	if (ImGui::BeginTable("##mdbuttons", 3, tableFlags)) {
		static const char* hexItems[] = { "DEC", "HEX" };

		ImGui::TableNextColumn();
		int hexdecIdx = (int) Settings->MemoryBlock->hex;
		if (ImGui::SliderInt("##hexdec", &hexdecIdx, 0, 1, hexItems[hexdecIdx], ImGuiSelectableFlags_SpanAvailWidth))
			Settings->MemoryBlock->hex = (bool) hexdecIdx;

		ImGui::TableNextColumn();
		if (saveType && ImGui::Button("Dump", ImVec2(baseWidth, 0.0f))) {
			ImGui::CloseCurrentPopup();
			if (fileExists) {
				QueryDialogCallback.connect([&](TMenuQueryType result) {
					if (result == GUI_QUERY_YES)
						Emulator->ProcessRawFile(true);

					GUI->QueryDialogCallback.disconnect_all();
				});

				QueryDialog("File Exists", "File already exists. Overwrite?", false);
			}
			else
				Emulator->ProcessRawFile(true);
		}
		else if (!saveType && ImGui::Button("Load", ImVec2(baseWidth, 0.0f))) {
			ImGui::CloseCurrentPopup();
			Emulator->ProcessRawFile(false);
		}

		ImGui::EndDisabled();
		ImGui::TableNextColumn();
		if (ImGui::Button("Cancel", ImVec2(baseWidth, 0.0f)))
			ImGui::CloseCurrentPopup();

		ImGui::EndTable();
	}

	ImGui::PopStyleVar();
}
//-----------------------------------------------------------------------------
