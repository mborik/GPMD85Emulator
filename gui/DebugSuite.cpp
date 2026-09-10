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
#include "imgui-mods/imgui_memory_editor.h"
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
void UserInterface::InitDebugSuite()
{
	memEditor = new MemoryEditor();
	memEditorBuffer = new BYTE[MEM_MAX]; // 64KB working buffer for memEditor
	memset(memEditorBuffer, 0, MEM_MAX);

	memEditor->Open = Settings->GUI->dialogMemEditOpened;
	memEditor->Cols = Settings->GUI->memEditColumns;
	memEditor->OptShowAscii = Settings->GUI->memEditAscii;
	memEditor->OptAddrDigitsCount = 4;
	memEditor->OptShowOptions = false;
	memEditor->GotoAddr = 0;

	memEditorDataContext = new MemEditorDataContext { Debugger, memEditorBuffer, NULL };
	memEditor->UserData = (void *) memEditorDataContext;
	memEditor->ReadFn = [](const ImU8* mem, size_t off, void* user_data) -> ImU8 {
		auto* ctx = static_cast<MemEditorDataContext*>(user_data);
		BYTE value;
		ctx->dbg->GetMemState(off, &value);
		ctx->buffer[off] = value;
		return value;
	};
	memEditor->WriteFn = [](ImU8* mem, size_t off, ImU8 val, void* user_data) {
		auto* ctx = static_cast<MemEditorDataContext*>(user_data);
		ctx->dbg->WriteByte(off, val);
		mem[off] = val;
	};
	memEditor->BgColorFn = [](const ImU8* mem, size_t off, void* user_data) -> ImU32 {
		auto* ctx = static_cast<MemEditorDataContext *>(const_cast<void *>(user_data));
		ImU8 changing = ctx->changingBuffer[off], changingQ = changing / 4;
		ImU32 color = IM_COL32(changingQ, changing / 2, changing, 64 + changingQ);
		BYTE state = ctx->dbg->GetMemState(off);
		if ((state & MA_RW) == 0)
			color |= IM_COL32(128, 0, 0, 0); // unaccessible
		if ((state & MA_RW) != MA_RW)
			color |= IM_COL32(64, 64, 0, 0); // partially blocked (RO/WO)
		else if (state & MA_VRAM_B)
			color |= IM_COL32(0, 32, 16, 0); // VRAM aside buffer
		else if (state & MA_VRAM)
			color |= IM_COL32(0, 64, 0, 0); // VRAM
		return color;
	};
}
//-----------------------------------------------------------------------------
void UserInterface::DestroyDebugSuite()
{
	if (memEditor) {
		delete memEditor;
		memEditor = NULL;
	}
	if (memEditorBuffer) {
		delete[] memEditorBuffer;
		memEditorBuffer = NULL;
	}
	if (memEditorDataContext) {
		delete memEditorDataContext;
		memEditorDataContext = NULL;
	}
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetDisass(int numberOfItems)
{
	static std::vector<TDisassLine> disassLines;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
	ImGui::BeginChild("DebugDisass", ImVec2(-FLT_MIN, 0.0f),
		ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

	float widthWidth = ImGui::GetContentRegionAvail().x;

	ImGui::PushItemFlag(
		ImGuiItemFlags_NoNav |
		ImGuiItemFlags_NoTabStop |
		ImGuiItemFlags_NoNavDefaultFocus, true);

	Debugger->FillDisass(disassLines, numberOfItems);
	for (int i = 0; i < disassLines.size(); i++) {
		TDisassLine line = disassLines[i];

		ImGui::PushID(i);
		ImGui::SetNextItemAllowOverlap();

		ImGuiSelectableFlags selectableFlags =
			ImGuiSelectableFlags_AllowDoubleClick |
			ImGuiSelectableFlags_NoHoldingActiveID |
			ImGuiSelectableFlags_NoSetKeyOwner |
			ImGuiSelectableFlags_NoAutoClosePopups;

		if (line.color == COL_CURSOR)
			selectableFlags |= ImGuiSelectableFlags_Highlight;
		if (line.color == COL_CURRENT) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.9f, 0.9f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		if (line.color == COL_BREAKPT) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.8f, 0.4f, 0.2f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		}

		ImGui::Selectable(line.text.c_str(), false, selectableFlags);

		if (line.color >= COL_CURRENT)
			ImGui::PopStyleColor(3);

		if (line.isBranch) {
			static ImVec4 branchColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f);
			const char *direction = line.isBranchFwdDir ? "\u2193" : "\u2191";

			if (line.isBranchTarget) {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(6, 0.0f));
				ImGui::TextColored(branchColor,
					Settings->Debugger->hex ? "#%02X%s" : "%03d%s",
					line.branchTarget, direction);
			}
			else if (line.isBranchSource) {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(1, 0.0f));
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 0.6f), "<");
			}
			else {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(1, 0.0f));
				ImGui::TextColored(branchColor, "%s", direction);
			}
		}
		if (line.isBreakPoint) {
			ImGui::SameLine(1.0f);
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "\u2022");
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
/*
	Debugger->FillRegs()
	Debugger->FillFlags()
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetStack()
{
/*
	Debugger->FillStack()
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetBreaks()
{
/*
	BYTE b = -1;
	char *line = NULL;
	for (int i = 0; i < 6; i++) {
		line = Debugger->FillBreakpoints(&b);

		if (line)
			PrintText(s, mx, my, GUI_COLOR_DBG_TEXT, line);

		PrintCheck(s, mx - GUI_CONST_HOTKEYCHAR + 1, my + 1,
				GUI_COLOR_CHECKED, SCHR_CHECK, (bool) b);
	}
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWindow()
{
	static bool firstTime = true;

	if (Settings->GUI->dialogDebugOpened) {
		ImGuiStyle& style = ImGui::GetStyle();

		ImVec2 framePadding = style.FramePadding * 2.0f;
		float lineHeight = ImGui::GetTextLineHeightWithSpacing();
		float widthWidth = GetMonoTextWidth(50, framePadding.x);
		float minHeight = widthWidth * 0.60f;

		ImGui::SetNextWindowSize(ImVec2(widthWidth, minHeight), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(widthWidth, minHeight), ImVec2(widthWidth, FLT_MAX));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 10.0f));

		if (ImGui::Begin("Debugger", &Settings->GUI->dialogDebugOpened, ImGuiWindowFlags_NoScrollbar)) {
			Debugger->RefreshRequest(firstTime);
			// firstTime = false;

			if (ImGui::BeginTable("DebuggerLayout", 2, ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_BordersInnerH)) {
				ImGui::TableSetupColumn("##dbghdr1", ImGuiTableColumnFlags_NoHide);
				ImGui::TableSetupColumn("##dbghdr2", ImGuiTableColumnFlags_WidthFixed, GetMonoTextWidth(12, framePadding.x));

				ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextColumn();

				if (ImGui::Button(Emulator->isRunning ? " \u23F9 " : " \u2023 "))
					Emulator->ActionPlayPause(!Emulator->isRunning, false);

				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();

				if (Emulator->isRunning)
					ImGui::BeginDisabled();

				float spacing = style.ItemInnerSpacing.x;
				if (ImGui::Button("Step")) { }
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button("Over")) { }
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button("Leave")) { }
				ImGui::SameLine(0.0f, spacing);
				if (ImGui::Button("Until Next")) { }

				if (Emulator->isRunning)
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
				int hexdecIdx = (int) Settings->Debugger->hex;
				if (ImGui::SliderInt("##hexdec", &hexdecIdx, 0, 1, hexItems[hexdecIdx]))
					Settings->Debugger->hex = (bool) hexdecIdx;

				ImGui::TableNextRow(ImGuiTableColumnFlags_WidthStretch);

				float cursorPos = ImGui::GetCursorPosY();
				float childHeight = ImGui::GetCurrentWindow()->Size.y - cursorPos - framePadding.y;
				int numberOfItems = static_cast<int>(ceil(childHeight / (lineHeight + 1.0f)));

				ImGui::TableNextColumn();
				DrawDebugWidgetDisass(numberOfItems);

				ImGui::TableNextColumn();
				DrawDebugWidgetRegs();
				DrawDebugWidgetStack();

				ImGui::EndTable();
			}
		}

		ImGui::End();
	}

	ImGui::PopStyleVar(2);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawMemEditDialog()
{
	static MemoryEditor::Sizes s;

	if (Settings->GUI->dialogMemEditOpened) {
		ImGuiStyle& style = ImGui::GetStyle();

		memEditorDataContext->changingBuffer = Debugger->GetChangingMemState();
		memEditor->OptFooterExtraHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 3.0f;
		memEditor->CalcSizes(s, MEM_MAX, 0);

		float minHeight = s.WindowWidth * 0.60f;
		ImGui::SetNextWindowSize(ImVec2(s.WindowWidth, minHeight), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(s.WindowWidth, minHeight), ImVec2(s.WindowWidth, FLT_MAX));

		if (ImGui::Begin("Memory Editor", &Settings->GUI->dialogMemEditOpened, ImGuiWindowFlags_NoScrollbar)) {
			memEditor->DrawContents(memEditorBuffer, MEM_MAX, 0);

			ImGui::Separator();
			ImGui::SetNextItemWidth(4 * s.GlyphWidth + style.FramePadding.x * 2.0f);

			static const char* widthItems[] = { NULL, "8", "16" };
			int widthIdx = (int) Settings->GUI->memEditColumns / 8;
			if (ImGui::SliderInt("##medcols", &widthIdx, 1, 2, widthItems[widthIdx])) {
				memEditor->ContentsWidthChanged = true;
				memEditor->Cols = Settings->GUI->memEditColumns = widthIdx * 8;
			}

			ImGui::SameLine();
			if (ImGui::Checkbox("ASCII", &Settings->GUI->memEditAscii)) {
				memEditor->ContentsWidthChanged = true;
				memEditor->OptShowAscii = Settings->GUI->memEditAscii;
			}

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			auto ProcessGotoAddr = [&](const char *c) {
				size_t gotoAddr;
				if (sscanf(c, "%zX", &gotoAddr) == 1) {
					memEditor->GotoAddr = gotoAddr;
					memEditor->HighlightMin = memEditor->HighlightMax = (size_t) -1;
				}
			};

			float addrInputWidth = (s.AddrDigitsCount + 1) * s.GlyphWidth + style.FramePadding.x * 2.0f;
			ImGui::SetNextItemWidth(addrInputWidth);
			ImGui::InputText("##medaddr",
				memEditor->AddrInputBuf, 5,
				ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_AlwaysOverwrite);
			ImGui::SameLine();
			if (ImGui::Button("MEM"))
				ProcessGotoAddr(memEditor->AddrInputBuf);

			static std::vector<std::string> regsLine(6, "");
			static short regUpdateCounter = 0;

			if (--regUpdateCounter <= 0) {
				regUpdateCounter = 10;
				Debugger->FillRegs(regsLine, true);
			}

			// SP, PC, HL, DE, BC
			for (int i = 5; i > 0; i--) {
				ImGui::SameLine();
				if (ImGui::Button(regsLine[i].c_str()))
					ProcessGotoAddr(regsLine[i].c_str() + 3);
			}

			if (memEditor->ContentsWidthChanged) {
				memEditor->CalcSizes(s, MEM_MAX, 0);
				ImGui::SetWindowSize(ImVec2(s.WindowWidth, ImGui::GetWindowSize().y));
			}
		}
		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
