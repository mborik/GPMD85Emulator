/*	TapeDialog.cpp: Part of GUI rendering class: Tape Browser Dialog
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
void UserInterface::DrawTapeDialog()
{
	if (TapeBrowser->shouldUpdateEntries || tapeDialogEntries.empty())
		TapeBrowser->FillFileList(tapeDialogEntries);

	if (Settings->GUI->dialogTapeBrowserOpened) {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(320.0f, 480.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("Tape Browser", &Settings->GUI->dialogTapeBrowserOpened,
			TapeBrowser->tapeChanged ? ImGuiWindowFlags_UnsavedDocument : ImGuiWindowFlags_None);

		static char label[12];
		static ImGuiSelectionBasicStorage selection;
		bool hex = Settings->TapeBrowser->hex;
		float sz = ImGui::GetFrameHeight();

		if (ImGui::BeginTable("TapeBrowserHeader", 2, ImGuiTableFlags_NoSavedSettings)) {
			ImGui::TableSetupColumn("##hdr1", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##hdr2", ImGuiTableColumnFlags_WidthFixed, 64.0f);

			static char *ptr = NULL;
			if (Settings->TapeBrowser->fileName && !TapeBrowser->preparedForSave) {
				ptr = strrchr(Settings->TapeBrowser->fileName, '/');
				if (ptr)
					ptr++;
				else
					ptr = Settings->TapeBrowser->fileName;
			}
			else
				ptr = (char *) "[new tape]";

			ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch, sz * 2.0f);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(ptr);

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

			static const char* hexItems[] = { "DEC", "HEX" };
			static const char* autoStopItems[] = {
				"Stop when the end is reached",
				"Stop at the next header block",
				"Stop at the block with the STOP marker"
			};

			int hexdecIdx = (int) Settings->TapeBrowser->hex;
			if (ImGui::SliderInt("##hexdec", &hexdecIdx, 0, 1, hexItems[hexdecIdx], ImGuiSelectableFlags_SpanAvailWidth))
				Settings->TapeBrowser->hex = (bool) hexdecIdx;

			ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextColumn();

			float availableWidth = ImGui::GetContentRegionAvail().x;
			ImGui::Dummy(ImVec2(availableWidth, 2.0f));
			ImGui::TextUnformatted("Auto Stop:");

			int autoStopIdx = (int) Settings->TapeBrowser->autoStop;
			ImGui::SetNextItemWidth(availableWidth);
			if (ImGui::BeginCombo("##autostop", autoStopItems[autoStopIdx], ImGuiComboFlags_HeightSmall)) {
				for (int n = 0; n < IM_COUNTOF(autoStopItems); n++) {
					const bool is_selected = (autoStopIdx == n);
					if (ImGui::Selectable(autoStopItems[n], is_selected))
						Settings->TapeBrowser->autoStop = (TAutoStopType) (autoStopIdx = n);
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::TableNextColumn();
			ImGui::Checkbox("Audio", &Settings->TapeBrowser->monitoring);
			ImGui::Checkbox("Flash", &Settings->TapeBrowser->flash);
			ImGui::EndTable();
		}

		ImGui::Separator();

		if (ImGui::BeginTable("TapeBrowserPlayer", 2, ImGuiTableFlags_NoSavedSettings)) {
			ImGui::TableSetupColumn("##btn", ImGuiTableColumnFlags_WidthFixed, 25.0f);
			ImGui::TableSetupColumn("##progress", ImGuiTableColumnFlags_NoHide);

			ImVec2 buttonSize(24.0f, 24.0f);
			ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch, buttonSize.y);
			ImGui::TableNextColumn();

			if (iconState >= 9) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.2f, 0.2f, 0.9f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 0.5f));
				if (ImGui::Button("\u23F9", buttonSize))
					TapeBrowser->ActionStop();
				ImGui::PopStyleColor(2);
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImColor::HSV(0.35f, 1.0f, 0.6f, 0.9f).Value);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.35f, 1.0f, 0.6f, 0.5f).Value);
				if (ImGui::ArrowButtonEx("Play", ImGuiDir_Right, buttonSize))
					TapeBrowser->ActionPlay();
				ImGui::PopStyleColor(2);
			}

			ImGui::TableNextColumn();

			const ImVec2 progressBarSize = ImVec2(ImGui::GetContentRegionAvail().x, buttonSize.y);
			ImGui::PushID("##progress");
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.16f, 0.4f, 0.2f, 1.0f));

			if (iconState >= 9)
				sprintf(label, Settings->TapeBrowser->hex ? "#%04X" : "%5d", TapeBrowser->ProgressBar->Position);
			else
				label[0] = '\0';

			ImGui::ProgressBar(
				((float) TapeBrowser->ProgressBar->Position / (float) TapeBrowser->ProgressBar->Max),
				progressBarSize, label
			);

			ImGui::PopStyleColor(2);
			ImGui::PopID();

			ImGui::EndTable();
		}

		ImGui::Separator();

		static ImGuiTableColumnFlags columnFlags =
			ImGuiTableColumnFlags_NoReorder |
			ImGuiTableColumnFlags_NoSort |
			ImGuiTableColumnFlags_NoClip;

		static ImGuiMultiSelectFlags selectionFlags =
			ImGuiMultiSelectFlags_BoxSelect1d |
			ImGuiMultiSelectFlags_NoSelectOnRightClick;

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 0.0f));
		if (ImGui::BeginTable("TapeBrowserItems", 5,
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_ScrollY)) {

			ImGui::TableSetupColumn("Cursor", columnFlags
				| ImGuiTableColumnFlags_WidthFixed
				| ImGuiTableColumnFlags_NoHeaderLabel, sz);
			ImGui::TableSetupColumn("ID/T Header", columnFlags);
			ImGui::TableSetupColumn("Start", columnFlags
				| ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("Length", columnFlags
				| ImGuiTableColumnFlags_WidthFixed, 50.0f);
			ImGui::TableSetupColumn("CRC Error", columnFlags
				| ImGuiTableColumnFlags_WidthFixed
				| ImGuiTableColumnFlags_NoHeaderLabel, 10.0f);
			ImGui::TableSetupScrollFreeze(0, 1);

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
			ImGui::TableHeadersRow();
			ImGui::PopStyleVar();

			int count = tapeDialogEntries.size();
			ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(selectionFlags, selection.Size, count);
			selection.ApplyRequests(ms_io);

/* TODO: Implement external storage for selection...
			ImGuiSelectionExternalStorage sel_adapter;
			sel_adapter.UserData = (void*) TapeBrowser;
			sel_adapter.AdapterSetItemSelected =
				[](ImGuiSelectionExternalStorage* self, int idx, bool selected) {
					TTapeBrowser* tapeBrowser = (TTapeBrowser *) self->UserData;
					tapeBrowser->ForceSelection(idx, selected);
				};
			sel_adapter.ApplyRequests(ms_io);
*/

			for (int i = 0; i < count; ++i) {
				const TTapeBrowser::TDialogItem &item = tapeDialogEntries[i];
				sprintf(label, "##item%05d", i);

				ImGui::TableNextRow();
				ImGui::PushID(label);
				ImGui::TableNextColumn();

				bool wasRightClickedButton = false;
				ImVec2 szVec(ImGui::GetFrameHeight(), ImGui::GetFrameHeight());
				ImGuiButtonFlags buttonFlags = ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight;

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4());
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImColor::HSV(0.5f, 0.3f, 0.3f, 0.3f).Value);
				if (i == TapeBrowser->currBlockIdx)
					ImGui::ArrowButtonEx("##cursor", ImGuiDir_Right, szVec, buttonFlags);
				else if (i == TapeBrowser->stopBlockIdx)
					ImGui::ButtonEx("\u23F9##cursor", szVec, buttonFlags);
				else
					ImGui::ButtonEx("##cursor", szVec, buttonFlags);

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
					wasRightClickedButton = true;
					if (i > TapeBrowser->currBlockIdx)
						TapeBrowser->stopBlockIdx = i;
				}
				else if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
					TapeBrowser->SetCurrentBlock(i);

				ImGui::PopStyleColor(2);
				ImGui::TableNextColumn();

				ImGui::SetNextItemSelectionUserData(i);
				ImGui::Selectable(item.name,
					selection.Contains((ImGuiID) i),
					ImGuiSelectableFlags_SpanAllColumns
				);

				if (!(tapeDialogEntries.empty() || wasRightClickedButton))
					DrawTapeDialogContextMenu(selection, i, item);

				ImGui::TableNextColumn();
				if (item.start < 0)
					ImGui::TextUnformatted("");
				else
					ImGui::TextAligned(1.0f, -FLT_MIN, (hex ? "#%04X" : "%5d"), item.start);

				ImGui::TableNextColumn();
				ImGui::TextAligned(1.0f, -FLT_MIN, (hex ? "#%04X" : "%5d"), item.length);

				ImGui::TableNextColumn();
				if (item.headCrcError)
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "!");
				else
					ImGui::TextUnformatted("");

				ImGui::PopID();
			}

			ms_io = ImGui::EndMultiSelect();
			selection.ApplyRequests(ms_io);
			ImGui::EndTable();
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DrawTapeDialogContextMenu(
	ImGuiSelectionBasicStorage &selection,
	int &index, const TTapeBrowser::TDialogItem &item
) {
	static char buf[64];
	bool noSelection = selection.Size == 0;

	if (ImGui::BeginPopupContextItem("##ctxmnu", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("Select all", NULL, false, selection.Size == tapeDialogEntries.size())) {
			for (size_t i = 0; i < tapeDialogEntries.size(); i++)
				selection.SetItemSelected((ImGuiID) i, true);
		}
		if (ImGui::MenuItem("Deselect all", NULL, false, noSelection))
			selection.Clear();

		ImGui::Separator();

		if (ImGui::MenuItem("Set current tape position", NULL, index == TapeBrowser->currBlockIdx, index != TapeBrowser->currBlockIdx)) {
			TapeBrowser->SetCurrentBlock(index);
		}
		if (ImGui::MenuItem("Set STOP Marker", NULL, index == TapeBrowser->stopBlockIdx,
			index > TapeBrowser->currBlockIdx && index != TapeBrowser->stopBlockIdx
		)) {
			TapeBrowser->stopBlockIdx = index;
		}
		ImGui::Separator();

		if (selection.Size > 0) {
			sprintf(buf, "Delete %d selected block%s", selection.Size, (selection.Size > 1) ? "s" : "");
			if (ImGui::MenuItem(buf)) {
				void* it = NULL;
				ImGuiID idx;
				while (selection.GetNextSelectedItem(&it, &idx)) {
					TapeBrowser->DeleteSelected((int) idx);
				}
				selection.Clear();
			}
		}
		else {
			if (ImGui::MenuItem("Delete current block"))
				TapeBrowser->DeleteSelected(index);
		}
/*
		ImGui::MenuItem("Move Block Up", NULL, false, false);
		ImGui::MenuItem("Move Block Down", NULL, false, false);
*/
		ImGui::EndPopup();
	}
}
//-----------------------------------------------------------------------------
