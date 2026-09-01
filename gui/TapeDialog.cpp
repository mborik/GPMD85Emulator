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
#include "TapeBrowser.h"
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
void UserInterface::InitTapeDialog()
{
	tapeDialogEntries.clear();
	tapeDialogSelection = new ImGuiSelectionBasicStorage();
	tapeDialogSelectionAdapter = new ImGuiSelectionExternalStorage();
	tapeDialogSelectionAdapter->UserData = (void*) TapeBrowser;
	tapeDialogSelectionAdapter->AdapterSetItemSelected =
		[](ImGuiSelectionExternalStorage* self, int idx, bool selected) {
			TTapeBrowser* tapeBrowser = reinterpret_cast<TTapeBrowser *>(self->UserData);
			tapeBrowser->DoSelection(idx, selected);
		};
}
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
		bool hex = Settings->TapeBrowser->hex;
		const ImGuiStyle& style = ImGui::GetStyle();
		float sz = ImGui::GetFrameHeight();

		if (ImGui::BeginTable("TapeBrowserHeader", 2, ImGuiTableFlags_NoSavedSettings)) {
			ImGui::TableSetupColumn("##hdr1", ImGuiTableColumnFlags_NoHide);
			ImGui::TableSetupColumn("##hdr2", ImGuiTableColumnFlags_WidthFixed, GetMonoTextWidth(9));

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
			float bigButtonSize = 24.0f * style.FontScaleMain;

			ImGui::TableSetupColumn("##btn", ImGuiTableColumnFlags_WidthFixed, bigButtonSize);
			ImGui::TableSetupColumn("##progress", ImGuiTableColumnFlags_NoHide);

			ImVec2 buttonSize(bigButtonSize, bigButtonSize);
			ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch, bigButtonSize);
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

		float fixedColumnWidth = GetMonoTextWidth(6, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 0.0f));
		if (ImGui::BeginTable("TapeBrowserItems", 5,
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_ScrollY)) {

			ImGui::TableSetupColumn("Cursor", columnFlags
				| ImGuiTableColumnFlags_WidthFixed
				| ImGuiTableColumnFlags_NoHeaderLabel, sz);
			ImGui::TableSetupColumn("ID/T Header", columnFlags);
			ImGui::TableSetupColumn("Start", columnFlags
				| ImGuiTableColumnFlags_WidthFixed, fixedColumnWidth);
			ImGui::TableSetupColumn("Length", columnFlags
				| ImGuiTableColumnFlags_WidthFixed, fixedColumnWidth);
			ImGui::TableSetupColumn("CRC Error", columnFlags
				| ImGuiTableColumnFlags_WidthFixed
				| ImGuiTableColumnFlags_NoHeaderLabel, GetMonoTextWidth(1, 2.0f));
			ImGui::TableSetupScrollFreeze(0, 1);

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 8.0f));
			ImGui::TableHeadersRow();
			ImGui::PopStyleVar();

			int count = tapeDialogEntries.size();
			ImGuiMultiSelectIO* ms_io = ImGui::BeginMultiSelect(
				ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_NoSelectOnRightClick,
				tapeDialogSelection->Size,
				count
			);
			tapeDialogSelection->ApplyRequests(ms_io);
			tapeDialogSelectionAdapter->ApplyRequests(ms_io);

			for (int i = 0; i < count; ++i) {
				const TTapeBrowser::TDialogItem &item = tapeDialogEntries[i];
				sprintf(label, "##item%05d", i);

				ImGui::TableNextRow();
				ImGui::PushID(label);
				ImGui::TableNextColumn();
				ImGui::BeginDisabled(TapeBrowser->playing);

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

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !TapeBrowser->playing) {
					if (i > TapeBrowser->currBlockIdx)
						TapeBrowser->stopBlockIdx = i;
				}
				else if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !TapeBrowser->playing)
					TapeBrowser->SetCurrentBlock(i);

				ImGui::PopStyleColor(2);
				ImGui::TableNextColumn();

				ImGui::SetNextItemSelectionUserData(i);
				ImGui::Selectable(item.name,
					tapeDialogSelection->Contains((ImGuiID) i),
					ImGuiSelectableFlags_SpanAllColumns
				);

				if (!tapeDialogEntries.empty() && !TapeBrowser->playing)
					DrawTapeDialogContextMenu(i);

				ImGui::EndDisabled();
				ImGui::TableNextColumn();
				if (item.start < 0)
					ImGui::TextUnformatted("");
				else
					ImGui::TextAligned(1.0f, -FLT_MIN, (hex ? "#%04X" : "%5d"), item.start);

				ImGui::TableNextColumn();
				ImGui::TextAligned(1.0f, -FLT_MIN, (hex ? "#%04X" : "%5d"), item.length);

				ImGui::TableNextColumn();
				if (item.headCrcError) {
					ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "!");
					if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
						ImGui::SetTooltip("CRC Error");
				}
				else
					ImGui::TextUnformatted("");

				ImGui::PopID();
			}

			ms_io = ImGui::EndMultiSelect();
			tapeDialogSelection->ApplyRequests(ms_io);
			tapeDialogSelectionAdapter->ApplyRequests(ms_io);

			ImGui::EndTable();
		}

		ImGui::PopStyleVar();
		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DrawTapeDialogContextMenu(int &index) {
	static char buf[64];
	bool noSelection = tapeDialogSelection->Size == 0;

	TTapeBrowser::TAPE_BLOCK *blk = TapeBrowser->GetBlock(index);

	if (ImGui::BeginPopupContextItem("##ctxmnu", ImGuiPopupFlags_MouseButtonRight)) {
		if (ImGui::MenuItem("Select All", NULL, false, tapeDialogSelection->Size != tapeDialogEntries.size())) {
			for (ImGuiID id = 0; id < tapeDialogEntries.size(); id++) {
				tapeDialogSelection->SetItemSelected((ImGuiID) id, true);
				TapeBrowser->DoSelection((int) id, false);
			}
		}
		if (ImGui::MenuItem("Deselect All", NULL, false, !noSelection)) {
			ImGuiID id;
			void* it = NULL;
			while (tapeDialogSelection->GetNextSelectedItem(&it, &id))
				TapeBrowser->DoSelection((int) id, false);
			tapeDialogSelection->Clear();
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Set Current Tape Position", NULL,
			index == TapeBrowser->currBlockIdx,
			index != TapeBrowser->currBlockIdx)) {

			TapeBrowser->SetCurrentBlock(index);
		}
		if (ImGui::MenuItem("Set STOP Marker", NULL,
			index == TapeBrowser->stopBlockIdx,
			index > TapeBrowser->currBlockIdx && index != TapeBrowser->stopBlockIdx)) {

			TapeBrowser->stopBlockIdx = index;
		}

		ImGui::Separator();

		if (tapeDialogSelection->Size > 0) {
			sprintf(buf, "Delete %d Selected Block%s", tapeDialogSelection->Size, (tapeDialogSelection->Size > 1) ? "s" : "");
			if (ImGui::MenuItem(buf)) {
				TapeBrowser->DeleteSelected();
				tapeDialogSelection->Clear();
			}
		}
		else {
			if (ImGui::MenuItem("Delete Block")) {
				TapeBrowser->DeleteSelected(index);
				tapeDialogSelection->Clear();
			}
		}

		ImGui::Separator();

		int directionPointer = sprintf(buf, "Move %sBlock%s Up",
			(tapeDialogSelection->Size > 0) ? "Selected " : "",
			(tapeDialogSelection->Size > 1) ? "s" : "");

		bool state = (tapeDialogSelection->Size > 0) ?
			(TapeBrowser->Selection->continuity && TapeBrowser->Selection->first > 0) :
			(index > 0);
		if (ImGui::MenuItem(buf, NULL, false, state)) {
			TapeBrowser->MoveSelected(true, &index);

			tapeDialogSelection->SetItemSelected((ImGuiID) TapeBrowser->Selection->last + 1, false);
			tapeDialogSelection->SetItemSelected((ImGuiID) TapeBrowser->Selection->first, true);
		}

		strcpy(buf + directionPointer - 2, "Down");

		state = (tapeDialogSelection->Size > 0) ?
			(TapeBrowser->Selection->continuity &&
				TapeBrowser->Selection->last < (TapeBrowser->totalBlocks - 1)) :
			(index < (TapeBrowser->totalBlocks - 1));
		if (ImGui::MenuItem(buf, NULL, false, state)) {
			TapeBrowser->MoveSelected(false, &index);

			tapeDialogSelection->SetItemSelected((ImGuiID) TapeBrowser->Selection->first - 1, false);
			tapeDialogSelection->SetItemSelected((ImGuiID) TapeBrowser->Selection->last, true);
		}

		if ((tapeDialogSelection->Size == 1 && blk->selected) || !tapeDialogSelection->Size) {
			ImGui::Separator();
			ImGui::BeginDisabled(true);

			if (ImGui::MenuItem("Change to Headerless Block", NULL, false, blk->cType)) { }
			if (ImGui::MenuItem("Change to Header Block", NULL, false, !blk->cType)) { }

			if (blk->cType) {
				ImGui::SeparatorText("Block Header");

				float offset = GetMonoTextWidth(8, 8.0f);
				BYTE blockNumber = (int) blk->bNumber;
				WORD blockStart = (int) blk->wStart;
				WORD blockLength = (int) blk->wLength;
				char blockType[2], blockName[9];
				blockType[0] = blk->cType;
				blockType[1] = '\0';
				memcpy(blockName, blk->cName, 8);
				blockName[8] = '\0';

				ImGui::TextUnformatted("ID/Type:");
				ImGui::SameLine(offset);
				ImGui::SetNextItemWidth(GetMonoTextWidth(8, 4.0f));
				ImGui::InputScalar("##blknum", ImGuiDataType_U8, &blockNumber, NULL, NULL, "%02X");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(GetMonoTextWidth(3, 2.0f));
				ImGui::InputText("##blktyp", blockType, 2,
					ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_CharsUppercase);

				ImGui::TextUnformatted("Name:");
				ImGui::SameLine(offset);
				ImGui::SetNextItemWidth(GetMonoTextWidth(12, 4.0f));
				ImGui::InputText("##blkname", blockName, 9,
					ImGuiInputTextFlags_AlwaysOverwrite | ImGuiInputTextFlags_CharsUppercase);

				ImGui::TextUnformatted("Start:");
				ImGui::SameLine(offset);
				ImGui::SetNextItemWidth(GetMonoTextWidth(12, 4.0f));
				ImGui::InputScalar("##blkstart", ImGuiDataType_U16, &blockStart, NULL, NULL, "%04X");

				ImGui::TextUnformatted("Length:");
				ImGui::SameLine(offset);
				ImGui::SetNextItemWidth(GetMonoTextWidth(12, 4.0f));
				ImGui::InputScalar("##blklen", ImGuiDataType_U16, &blockLength, NULL, NULL, "%04X");
			}

			ImGui::EndDisabled();
		}

		ImGui::EndPopup();
	}
}
//-----------------------------------------------------------------------------
