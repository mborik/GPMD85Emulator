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

	struct UserDataContext {
		TDebugger *dbg;
		BYTE *buffer;
	};

	static UserDataContext ctx = { Debugger, memEditorBuffer };
	memEditor->UserData = (void *) &ctx;
	memEditor->ReadFn = [](const ImU8* mem, size_t off, void* user_data) -> ImU8 {
		auto* ctx = static_cast<UserDataContext*>(user_data);
		BYTE value;
		ctx->dbg->GetMemState(off, &value);
		ctx->buffer[off] = value;
		return value;
	};
	memEditor->WriteFn = [](ImU8* mem, size_t off, ImU8 val, void* user_data) {
		auto* ctx = static_cast<UserDataContext*>(user_data);
		ctx->dbg->WriteByte(off, val);
		mem[off] = val;
	};
	memEditor->BgColorFn = [](const ImU8* mem, size_t off, void* user_data) -> ImU32 {
		auto* ctx = static_cast<UserDataContext *>(const_cast<void *>(user_data));
		ImU8 changing = ctx->dbg->GetChangingBufferValue(off), changingQ = changing / 4;
		ImU32 color = IM_COL32(changingQ, changing / 2, changing, 64 + changingQ);
		BYTE value, state = ctx->dbg->GetMemState(off, &value);
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
}
//-----------------------------------------------------------------------------
void UserInterface::DrawDebugWidgetDisass(bool full)
{
/*
	int l1 = (full ? 8 : 4), l2 = (full ? 25 : 12);

	BYTE b = l1;
	char *line = NULL;
	for (int i = 0; i < l2; i++) {
		line = Debugger->FillDisass(&b);

		if (line)
			PrintText(s, mx, my, GUI_COLOR_DBG_TEXT, line);
		if (b)
			PrintChar(s, mx - GUI_CONST_HOTKEYCHAR, my,
					GUI_COLOR_HIGHLIGHT, SCHR_NAVIGATOR);
	}
*/
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
/*
	DrawDebugWidgetDisass((Settings->Debugger->listType == DL_DISASM));
	DrawDebugWidgetRegs();
	DrawDebugWidgetStack();
	DrawDebugWidgetBreaks();
*/
}
//-----------------------------------------------------------------------------
void UserInterface::DrawMemEditDialog()
{
	static MemoryEditor::Sizes s;

	if (Settings->GUI->dialogMemEditOpened) {
		ImGuiStyle& style = ImGui::GetStyle();

		memEditor->OptFooterExtraHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 3.0f;
		memEditor->CalcSizes(s, MEM_MAX, 0);

		ImGui::SetNextWindowSize(ImVec2(s.WindowWidth, s.WindowWidth * 0.60f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(ImVec2(s.WindowWidth, 0.0f), ImVec2(s.WindowWidth, FLT_MAX));

		if (ImGui::Begin("Memory Editor", &Settings->GUI->dialogMemEditOpened, ImGuiWindowFlags_NoScrollbar)) {
			memEditor->DrawContents(memEditorBuffer, MEM_MAX, 0);

			ImGui::Separator();
			ImGui::SetNextItemWidth(4 * s.GlyphWidth + style.FramePadding.x * 2.0f);

			static const char* widthItems[] = { "", "8", "16" };
			int widthIdx = (int) Settings->GUI->memEditColumns / 8;
			if (ImGui::SliderInt("##medcols", &widthIdx, 1, 2, widthItems[widthIdx])) {
				memEditor->ContentsWidthChanged = true;
				memEditor->Cols = Settings->GUI->memEditColumns = widthIdx * 8;
			}

			// ImGui::SameLine();
			// ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			if (ImGui::Checkbox("ASCII", &Settings->GUI->memEditAscii)) {
				memEditor->ContentsWidthChanged = true;
				memEditor->OptShowAscii = Settings->GUI->memEditAscii;
			}

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();
			ImGui::SetNextItemWidth((s.AddrDigitsCount + 1) * s.GlyphWidth + style.FramePadding.x * 2.0f);
			ImGui::InputText("##medaddr",
				memEditor->AddrInputBuf, 5,
				ImGuiInputTextFlags_CharsHexadecimal);
			ImGui::SameLine();
			if (ImGui::Button("MEM")) {
				size_t gotoAddr;
				if (sscanf(memEditor->AddrInputBuf, "%zX", &gotoAddr) == 1) {
					memEditor->GotoAddr = gotoAddr;
					memEditor->HighlightMin = memEditor->HighlightMax = (size_t) -1;
				}
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
