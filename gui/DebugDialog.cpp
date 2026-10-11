/*	DebugSuite.cpp: Part of GUI rendering class: Debugger Window
	Copyright (c) 2012-2026 Martin Borik <martin@borik.net>

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
#define radix Settings->Debugger->hex
//-----------------------------------------------------------------------------
static ImGuiTabBarFlags tab_bar_flags =
	ImGuiTabBarFlags_FittingPolicyMixed |
	ImGuiTabBarFlags_NoCloseWithMiddleMouseButton |
	ImGuiTabBarFlags_NoTabListScrollingButtons |
	ImGuiTabBarFlags_DrawSelectedOverline;

static ImGuiSelectableFlags selectable_flags =
	ImGuiSelectableFlags_AllowDoubleClick |
	ImGuiSelectableFlags_NoHoldingActiveID |
	ImGuiSelectableFlags_NoSetKeyOwner |
	ImGuiSelectableFlags_NoAutoClosePopups;

static ImGuiItemFlags item_flags =
	ImGuiItemFlags_NoNav |
	ImGuiItemFlags_NoTabStop |
	ImGuiItemFlags_NoNavDefaultFocus;

static ImGuiPopupFlags popup_flags =
	ImGuiWindowFlags_AlwaysAutoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoDecoration |
	ImGuiWindowFlags_NoSavedSettings;
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugDialog()
{
	static bool isOpening = true;
	static char gotoMemEditor[8];
	static ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoNavInputs;

	if (dialogDebugOpened) {
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiIO& io = ImGui::GetIO();

		ImVec2 framePadding = style.FramePadding * 2.0f;
		float widthWidth = GetMonoTextWidth(52, framePadding.x);
		float minHeight = GetTextLineHeight(27);

		if (isOpening) {
			Debugger->Reset();
			ImGui::SetNextWindowFocus();
		}

		bool isRunning = Emulator->isRunning;

		ImGui::SetNextWindowSize(ImVec2(widthWidth, minHeight), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(widthWidth, minHeight), ImVec2(widthWidth, FLT_MAX));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(5.0f, 10.0f));

		if (ImGui::Begin("Debugger", &dialogDebugOpened, flags)) {
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
				dialogDebugFocused = true;

			if (dialogDebugFocused && !ImGui::IsAnyItemActive()) {
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true)) {
					Debugger->HandleKeyboardInput(K_UP);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true)) {
					Debugger->HandleKeyboardInput(K_DOWN);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_PageUp, true)) {
					Debugger->HandleKeyboardInput(K_PAGEUP);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_PageDown, true)) {
					Debugger->HandleKeyboardInput(K_PAGEDOWN);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
					Debugger->HandleKeyboardInput(K_HOME);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Enter) && Debugger->lineAtCursor) {
					Debugger->DoGotoAddress(Debugger->lineAtCursor->lookupAddress);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Space) && Debugger->lineAtCursor) {
					std::string addr = Debugger->lineAtCursor->text.substr(1, 5);
					if (radix)
						addr = addr.substr(1);
					Debugger->ToggleBreakPoint(addr.c_str(), -1);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
					Debugger->NestPop();
				}
				if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
					Debugger->DoTrace(!isRunning);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_F7)) {
					Debugger->DoStepInto();
				}
				if (ImGui::IsKeyPressed(ImGuiKey_F8)) {
					Debugger->DoStepOver();
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Z) && Debugger->lineAtCursor) {
					Debugger->SetPC(Debugger->lineAtCursor->addr);
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F8)) {
					Debugger->DoStepOut();
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_F7)) {
					Debugger->DoStepToNext();
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_M)) {
					Settings->Debugger->listSource = LS_MEM;
					Settings->Debugger->listOffset = 0;
					if (Debugger->lineAtCursor)
						Settings->Debugger->listMemoryAddress = Debugger->lineAtCursor->lookupAddress;
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_H)) {
					Settings->Debugger->listSource = LS_HL;
					Settings->Debugger->listOffset = 0;
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_D)) {
					Settings->Debugger->listSource = LS_DE;
					Settings->Debugger->listOffset = 0;
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_B)) {
					Settings->Debugger->listSource = LS_BC;
					Settings->Debugger->listOffset = 0;
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_P)) {
					Settings->Debugger->listSource = LS_PC;
					Settings->Debugger->listOffset = 0;
				}
				if (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_S)) {
					Settings->Debugger->listSource = LS_SP;
					Settings->Debugger->listOffset = 0;
				}
			}

			Debugger->RefreshRequest(isOpening);
			isOpening = false;

			if (ImGui::BeginTable("DebuggerLayout", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInnerH)) {
				ImGui::TableSetupColumn("##dbghdr1", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("##dbghdr2", ImGuiTableColumnFlags_WidthFixed, GetMonoTextWidth(12, framePadding.x));

				ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextColumn();

				if (ImGui::Button(isRunning ? " \u23F9 " : " \u2023 "))
					isRunning = Debugger->DoTrace(!isRunning);
				ImGui::SetItemTooltip("Trace (F5)");

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();

				if (isRunning)
					ImGui::BeginDisabled();

				float spacing = style.ItemInnerSpacing.x;
				if (ImGui::Button("Step")) {
					Debugger->DoStepInto();
				}
				ImGui::SetItemTooltip("Step into (F7)");
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button("Over")) {
					Debugger->DoStepOver();
				}
				ImGui::SetItemTooltip("Step over (F8)");
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button("Leave")) {
					Debugger->DoStepOut();
				}
				ImGui::SetItemTooltip("Leave routine\n(Shift+F8)");
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button("Next")) {
					Debugger->DoStepToNext();
				}
				ImGui::SetItemTooltip("Run until next\ninstruction (Shift+F7)");

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();

				if (ImGui::Button("Mem") || ImGui::IsKeyPressed(ImGuiKey_M)) {
					std::snprintf(gotoMemEditor, sizeof(gotoMemEditor),
						radix ? "%04X" : "%05d", Debugger->GetPC());
					ImGui::OpenPopup("DebugGotoMem");
				}
				ImGui::SetItemTooltip("Move cursor to\nmemory address (M)");

				ImGui::SetNextWindowPos(ImGui::GetItemRectMin() - ImVec2(style.FramePadding.x * 2, 0.0f));
				if (ImGui::BeginPopup("DebugGotoMem", popup_flags)) {
					unsigned width = radix ? 5 : 6;
					ImGui::SetNextItemWidth(GetMonoTextWidth(width, style.FramePadding.x));

					bool enterPressed = ImGui::InputText(
						"##gotoMemEditor",
						gotoMemEditor, width,
						ImGuiInputTextFlags_AlwaysOverwrite |
						ImGuiInputTextFlags_EnterReturnsTrue |
						(radix ?
							ImGuiInputTextFlags_CharsHexadecimal :
							ImGuiInputTextFlags_CharsDecimal)
					);

					ImGui::SameLine(0, 0.05f);
					if (ImGui::Button("\u2713") || enterPressed) {
						Debugger->DoGotoAddress(gotoMemEditor);
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button(" \u02c4 ")) {
					Debugger->HandleKeyboardInput(K_HOME);
				}
				ImGui::SetItemTooltip("Back to PC (Home)");

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);

				if (isRunning)
					ImGui::EndDisabled();

				ImGui::TableNextColumn();
				ImGui::SameLine();

				static const char* cpuItems[] = { "8080", "Z80" };
				static const char* hexItems[] = { "DEC", "HEX" };
				int cputypeIdx = (int) Settings->Debugger->z80;
				ImGui::SetNextItemWidth(GetMonoTextWidth(4, framePadding.x));
				if (ImGui::SliderInt("##cputype", &cputypeIdx, 0, 1, cpuItems[cputypeIdx]))
					Settings->Debugger->z80 = (bool) cputypeIdx;

				ImGui::SameLine();
				ImGui::SetNextItemWidth(GetMonoTextWidth(4, framePadding.x));
				int hexdecIdx = (int) radix;
				if (ImGui::SliderInt("##hexdec", &hexdecIdx, 0, 1, hexItems[hexdecIdx]))
					radix = (bool) hexdecIdx;

				ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch);

				float cursorPos = ImGui::GetCursorPosY();
				float childHeight = ImGui::GetCurrentWindow()->Size.y - cursorPos - framePadding.y;
				int numberOfItems = static_cast<int>(ceil(childHeight / GetTextLineHeight(1)));

				if (isRunning)
					ImGui::BeginDisabled();

				ImGui::TableNextColumn();
				DrawDebugWidgetDisass(numberOfItems);

				ImGui::TableNextColumn();
				DrawDebugWidgetRegs();
				DrawDebugWidgetStackBreakNest();
				DrawDebugWidgetWatchers(numberOfItems - 18);

				if (isRunning)
					ImGui::EndDisabled();

				ImGui::EndTable();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}
	else if (dialogDebugFocused) {
		dialogDebugFocused = false;
		Debugger->flowControl = DBGCTL_RUNNING;
		Emulator->ActionPlayPause(true);

		if (!isOpening)
			isOpening = true;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetDisass(int numberOfItems)
{
	static std::vector<TDisassLine> disassLines;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
	ImGui::BeginChild("DebugDisass", ImVec2(-FLT_MIN, 0.0f),
		ImGuiChildFlags_Borders |
		ImGuiChildFlags_AutoResizeY);

	if (ImGui::IsWindowHovered() || ImGui::IsWindowFocused()) {
		float wheel = ImGui::GetIO().MouseWheel;
		if (wheel > 0.0f)
			Debugger->HandleKeyboardInput(K_UP);
		if (wheel < 0.0f)
			Debugger->HandleKeyboardInput(K_DOWN);
	}

	float widthWidth = ImGui::GetContentRegionAvail().x;
	float breakpointOffset = GetMonoTextWidth(14.5, 0.0f);

	ImGui::PushItemFlag(item_flags, true);

	Debugger->FillDisass(disassLines, numberOfItems);
	for (int i = 0; i < disassLines.size(); i++) {
		TDisassLine line = disassLines[i];
		std::string addr = line.text.substr(1, 5);
		if (radix)
			addr = addr.substr(1);

		ImGuiSelectableFlags selectableFlags = selectable_flags;
		if (line.color == COL_CURSOR) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, Color[GCCol_ItemCursor]);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, DimColorAlpha(Color[GCCol_ItemCursor]));
		}
		else if (line.color == COL_CURRENT) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, Color[GCCol_ItemPC]);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, DimColorAlpha(Color[GCCol_ItemPC]));
			ImGui::PushStyleColor(ImGuiCol_Text, Color[GCCol_TextBlack]);
		}
		else if (line.color == COL_BREAKPT) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, Color[GCCol_ItemBreakpoint]);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, DimColorAlpha(Color[GCCol_ItemBreakpoint]));
			ImGui::PushStyleColor(ImGuiCol_Text, Color[GCCol_TextBlack]);
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, Color[GCCol_Transparent]);
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Color[GCCol_Transparent]);
		}

		ImGui::PushID(i);
		ImGui::SetNextItemAllowOverlap();
		if (ImGui::Selectable(line.text.c_str(), false, selectableFlags) ||
			ImGui::IsItemClicked(ImGuiMouseButton_Right)) {

			Debugger->SetTraceCursor(i);
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				Debugger->ToggleBreakPoint(addr.c_str(), -1);
			}
		}

		ImVec4 branchColor = Color[GCCol_SignBranch];
		if (line.color >= COL_CURRENT) {
			ImGui::PopStyleColor(3);
			branchColor = Color[GCCol_SignBranchOpaque];
		}
		else
			ImGui::PopStyleColor(2);

		if (ImGui::BeginPopupContextItem()) {
			static char lookupMenuItem[32];
			if (ImGui::MenuItem("Toggle Breakpoint", "Space")) {
				Debugger->ToggleBreakPoint(addr.c_str(), -1);
			}
			if (line.hasLookupAddress) {
				std::snprintf(lookupMenuItem, sizeof(lookupMenuItem),
					radix ? "Inspect #%04X" : "Inspect %05d", line.lookupAddress);
				if (ImGui::MenuItem(lookupMenuItem, "Enter"))
					Debugger->DoGotoAddress(line.lookupAddress);
				std::snprintf(lookupMenuItem, sizeof(lookupMenuItem),
					radix ? "Watch #%04X" : "Watch %05d", line.lookupAddress);
				if (ImGui::MenuItem(lookupMenuItem, "\u02C4+M")) {
					Settings->Debugger->listSource = LS_MEM;
					Settings->Debugger->listMemoryAddress = line.lookupAddress;
					Settings->Debugger->listOffset = 0;
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Set PC to Address", "Z")) {
				Debugger->SetPC(line.addr);
			}
			ImGui::EndPopup();
		}

		if (line.isBranch) {
			const char *direction = line.isBranchFwdDir ? "\u2193" : "\u2191";

			if (line.isBranchTarget) {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(6, 0.0f));
				ImGui::TextColored(branchColor,
					radix ? "#%04X%s" : "%5d%s",
					line.branchTarget, direction);
			}
			else if (line.isBranchSource) {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(1, 0.0f));
				ImGui::TextDisabled("<");
			}
			else {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(1, 0.0f));
				ImGui::TextColored(branchColor, "%s", direction);
			}
		}
		if (line.isBreakPoint) {
			ImGui::SameLine(breakpointOffset);
			ImGui::TextColored(Color[GCCol_SignBreakpoint], "\u2022");
		}

		ImGui::PopID();
	}

	ImGui::PopItemFlag();
	ImGui::EndChild();
	ImGui::PopStyleVar();
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetRegs()
{
	static char regEditorValue[8], regEditorKey[3], tStatesText[16];
	static std::vector<std::string> regs, flags;
	const ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

	ImGui::BeginChild("DebugRegs", ImVec2(0, GetTextLineHeight(8)), ImGuiChildFlags_Borders);

	ImGui::PushItemFlag(item_flags, true);
	if (ImGui::BeginTable("RegsLayout", 2, ImGuiTableFlags_NoSavedSettings)) {
		ImGui::TableSetupColumn("##reghdr1", ImGuiTableColumnFlags_NoHide);
		ImGui::TableSetupColumn("##reghdr2", ImGuiTableColumnFlags_WidthFixed, GetMonoTextWidth(2));

		ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextColumn();

		Debugger->FillRegs(regs);

		for (int i = 0; i < 6; i++) {
			ImGui::PushID(regs[i].substr(0, 2).c_str());
			ImGui::Selectable(
				regs[i].c_str(), false, selectable_flags,
				ImVec2(GetMonoTextWidth(8), 0)
			);

			ImVec2 button_pos = ImGui::GetItemRectMin();

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				std::string reg = regs[i].substr(3, 5);
				if (radix)
					reg = reg.substr(1);
				strcpy(regEditorKey, regs[i].substr(0, 2).c_str());
				strcpy(regEditorValue, reg.c_str());

				ImGui::OpenPopup("DebugRegsEdit");
			}

			ImGui::SetNextWindowPos(ImVec2(
				button_pos.x + GetMonoTextWidth(radix ? 3.5f : 2.5f),
				button_pos.y - (style.ItemInnerSpacing.y + style.FramePadding.y - 1.0f)
			));
			if (ImGui::BeginPopup("DebugRegsEdit", popup_flags)) {
				unsigned width = radix ? 5 : 6;
				ImGui::SetNextItemWidth(GetMonoTextWidth(width, style.FramePadding.x));

				bool enterPressed = ImGui::InputText(
					"##regAddressEditor",
					regEditorValue, width,
					ImGuiInputTextFlags_AlwaysOverwrite |
					ImGuiInputTextFlags_EnterReturnsTrue |
					(radix ?
						ImGuiInputTextFlags_CharsHexadecimal :
						ImGuiInputTextFlags_CharsDecimal)
				);

				ImGui::SameLine(0, 0.05f);
				if (ImGui::Button("\u2713") || enterPressed) {
					Debugger->ModifyRegister(regEditorKey, regEditorValue);
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::PopID();
		}

		ImGui::TableNextColumn();
		Debugger->FillFlags(flags);
		for (int i = 0; i < flags.size(); i++) {
			ImGui::PushID(i);
			ImGui::Selectable(flags[i].c_str(), false, selectable_flags);

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				Debugger->ModifyFlag(i);

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	std::snprintf(tStatesText, sizeof(tStatesText), "T:%010d", Debugger->GetTCycles());
	ImGui::PushID("##tcycles");
	ImGui::Selectable(tStatesText, false, selectable_flags);
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		Debugger->ResetTCycles();
	ImGui::PopID();

	ImGui::TextUnformatted(Debugger->GetMemoryState());

	ImGui::PopItemFlag();
	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetStackBreakNest()
{
	static char stackEditorValue[8];
	static int stackEditorOffset;
	static char breakpointId[16];
	static char breakpointEditorValue[8];
	static int breakpointEditorIndex = -1;
	static std::vector<std::string> stack, nests;
	static std::vector<std::pair<std::string, bool>> breaks;
	const ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

	if (ImGui::BeginTabBar("DebugStackNests", tab_bar_flags)) {
		if (ImGui::BeginTabItem("SP")) {
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild("DebugStack", ImVec2(0, GetTextLineHeight(6)), ImGuiChildFlags_Borders);

			ImGui::PushItemFlag(item_flags, true);

			Debugger->FillStack(stack);
			for (int i = 0; i < stack.size(); i++) {
				ImGui::PushID(i);
				ImGui::Selectable(stack[i].c_str(), false, selectable_flags);

				ImVec2 button_pos = ImGui::GetItemRectMin();

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					std::string value = stack[i].substr(7, 5);
					if (radix)
						value = value.substr(1);
					strcpy(stackEditorValue, value.c_str());
					stackEditorOffset = (i - 1) * 2;

					ImGui::OpenPopup("DebugStackEdit");
				}

				ImGui::SetNextWindowPos(ImVec2(
					button_pos.x + GetMonoTextWidth(radix ? 7.5f : 6.5f),
					button_pos.y - (style.ItemInnerSpacing.y + style.FramePadding.y - 1.0f)
				));
				if (ImGui::BeginPopup("DebugStackEdit", popup_flags)) {
					unsigned width = radix ? 5 : 6;
					ImGui::SetNextItemWidth(GetMonoTextWidth(width, style.FramePadding.x));

					bool enterPressed = ImGui::InputText(
						"##stackValueEditor",
						stackEditorValue, width,
						ImGuiInputTextFlags_AlwaysOverwrite |
						ImGuiInputTextFlags_EnterReturnsTrue |
						(radix ?
							ImGuiInputTextFlags_CharsHexadecimal :
							ImGuiInputTextFlags_CharsDecimal)
					);

					ImGui::SameLine(0, 0.05f);
					if (ImGui::Button("\u2713") || enterPressed) {
						Debugger->ModifyStack(stackEditorOffset, stackEditorValue);
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			ImGui::PopItemFlag();
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("BP")) {
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild("DebugBreakpoints", ImVec2(0, GetTextLineHeight(6)), ImGuiChildFlags_Borders);

			ImGui::PushItemFlag(item_flags, true);

			Debugger->FillBreakpoints(breaks);
			for (int i = 0; i < breaks.size(); i++) {
				std::snprintf(breakpointId, sizeof(breakpointId), "##bpCheck%d", i);

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
				if (ImGui::Checkbox(breakpointId, &breaks[i].second))
					Debugger->ToggleBreakPoint(i + 1, breaks[i].second);

				ImGui::PopStyleVar();
				ImGui::SameLine();

				std::snprintf(breakpointId, sizeof(breakpointId), "bpValue%d", i);
				ImGui::PushID(breakpointId);
				ImGui::Selectable(breaks[i].first.c_str(), false, selectable_flags);

				ImVec2 button_pos = ImGui::GetItemRectMin();

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					std::string value = breaks[i].first.substr(radix ? 5 : 4);
					strcpy(breakpointEditorValue, value.c_str());
					breakpointEditorIndex = i + 1;

					ImGui::OpenPopup("DebugBreakpointEdit");
				}

				ImGui::SetNextWindowPos(ImVec2(
					button_pos.x + GetMonoTextWidth(radix ? 4.5f : 3.5f),
					button_pos.y - (style.ItemInnerSpacing.y + style.FramePadding.y - 1.0f)
				));
				if (ImGui::BeginPopup("DebugBreakpointEdit", popup_flags)) {
					unsigned width = radix ? 5 : 6;
					ImGui::SetNextItemWidth(GetMonoTextWidth(width, style.FramePadding.x));

					bool enterPressed = ImGui::InputText(
						"##breakpointValueEditor",
						breakpointEditorValue, width,
						ImGuiInputTextFlags_AlwaysOverwrite |
						ImGuiInputTextFlags_EnterReturnsTrue |
						(radix ?
							ImGuiInputTextFlags_CharsHexadecimal :
							ImGuiInputTextFlags_CharsDecimal)
					);

					ImGui::SameLine(0, 0.05f);
					if (ImGui::Button("\u2713") || enterPressed) {
						bool active = true;
						Debugger->ToggleBreakPoint(
							breakpointEditorValue,
							breakpointEditorIndex,
							&active
						);

						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}

				ImGui::PopID();
			}

			ImGui::PopItemFlag();
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Nests")) {
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild("DebugNests", ImVec2(0, GetTextLineHeight(6)), ImGuiChildFlags_Borders);

			if (ImGui::BeginTable("NestsLayout", 2, ImGuiTableFlags_NoSavedSettings)) {
				ImGui::TableSetupColumn("##nesthdr1", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("##nesthdr2", ImGuiTableColumnFlags_NoHide);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				Debugger->FillNestings(nests);
				if (!nests.empty())
					nests.push_back("");

				int n = 0, nestCountLeft = nests.size(), nestCountRight = 0;
				if (nestCountLeft > 6) {
					nestCountRight = nestCountLeft - 6;
					nestCountLeft = 6;
				}
				for (int i = 0; i < nestCountLeft; i++, n++) {
					ImGui::PushID(n);
					if (nests[n].empty()) {
						if (ImGui::SmallButton(" Pop ")) {
							Debugger->NestPop();
						}
						ImGui::SetItemTooltip("Back to previous view (Backspace)");
					}
					else {
						ImGui::Selectable(
							nests[n].c_str(), false,
							selectable_flags | ImGuiSelectableFlags_Disabled,
							ImVec2(GetMonoTextWidth(5), 0)
						);
					}
					ImGui::PopID();
				}

				ImGui::TableNextColumn();
				for (int i = 0; i < nestCountRight; i++, n++) {
					ImGui::PushID(n);
					if (nests[n].empty()) {
						if (ImGui::SmallButton(" Pop ")) {
							Debugger->NestPop();
						}
						ImGui::SetItemTooltip("Back to previous view (Backspace)");
					}
					else {
						ImGui::Selectable(
							nests[n].c_str(), false,
							selectable_flags | ImGuiSelectableFlags_Disabled,
							ImVec2(GetMonoTextWidth(5), 0));
					}
					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::PopStyleVar();
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetWatchers(int maxLineHeight)
{
	static char watcherEditorValue[8];
	static std::vector<std::string> watcher, regs;
	const ImGuiStyle& style = ImGui::GetStyle();

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

	if (ImGui::BeginTabBar("DebugWatchers", tab_bar_flags)) {
		ImGui::PushID("DebugWatchers");

		Debugger->FillRegs(regs);
		Debugger->FillWatchMemory(watcher, maxLineHeight);

		if (ImGui::TabItemButton("\u2193", ImGuiTabItemFlags_Trailing | ImGuiTabItemFlags_NoTooltip))
			ImGui::OpenPopup("DebugWatchSrc");

		ImGui::SetNextWindowSize(ImVec2(GetMonoTextWidth(9), 0));
		if (ImGui::BeginPopup("DebugWatchSrc")) {
			unsigned width = radix ? 5 : 6;
			std::snprintf(
				watcherEditorValue, sizeof(watcherEditorValue),
				radix ? "%04X" : "%05d",
				Settings->Debugger->listMemoryAddress
			);

			ImGui::SetNextItemWidth(GetMonoTextWidth(width, style.FramePadding.x));
			bool enterPressed = ImGui::InputText(
				"##watcherValueEditor",
				watcherEditorValue, width,
				ImGuiInputTextFlags_AlwaysOverwrite |
				ImGuiInputTextFlags_EnterReturnsTrue |
				(radix ?
					ImGuiInputTextFlags_CharsHexadecimal :
					ImGuiInputTextFlags_CharsDecimal)
			);

			ImGui::SameLine(0, 0.05f);
			if (ImGui::Button("\u2713") || enterPressed) {
				Settings->Debugger->listSource = LS_MEM;
				Settings->Debugger->listMemoryAddress = strtoul(watcherEditorValue, nullptr, radix ? 16 : 10);
				Settings->Debugger->listOffset = 0;
				ImGui::CloseCurrentPopup();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(regs[3].c_str())) {
				Settings->Debugger->listSource = LS_HL;
				Settings->Debugger->listOffset = 0;
			}
			if (ImGui::MenuItem(regs[2].c_str())) {
				Settings->Debugger->listSource = LS_DE;
				Settings->Debugger->listOffset = 0;
			}
			if (ImGui::MenuItem(regs[1].c_str())) {
				Settings->Debugger->listSource = LS_BC;
				Settings->Debugger->listOffset = 0;
			}
			if (ImGui::MenuItem(regs[5].c_str())) {
				Settings->Debugger->listSource = LS_SP;
				Settings->Debugger->listOffset = 0;
			}
			if (ImGui::MenuItem(regs[4].c_str())) {
				Settings->Debugger->listSource = LS_PC;
				Settings->Debugger->listOffset = 0;
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginTabItem(watcher.front().c_str())) {
			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild("DebugWatch", ImVec2(0, GetTextLineHeight(maxLineHeight)), ImGuiChildFlags_Borders);

			if (ImGui::IsWindowHovered() || ImGui::IsWindowFocused()) {
				float wheel = ImGui::GetIO().MouseWheel;
				if (wheel > 0.0f)
					Settings->Debugger->listOffset = std::max(-128, Settings->Debugger->listOffset - 2);
				if (wheel < 0.0f)
					Settings->Debugger->listOffset = std::min(128, Settings->Debugger->listOffset + 2);
			}

			for (int ii = 1; ii < watcher.size(); ii++) {
				ImGui::TextUnformatted(watcher[ii].c_str());
				if (ii == 1 && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					Settings->Debugger->listOffset = 0;
			}

			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::EndTabItem();
		}

		ImGui::PopID();
		ImGui::EndTabBar();
	}

	ImGui::PopStyleVar();
}
//-----------------------------------------------------------------------------
