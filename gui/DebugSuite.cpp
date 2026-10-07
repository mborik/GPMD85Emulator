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
#define GL_GLEXT_PROTOTYPES
#ifdef IMGUI_IMPL_OPENGL_ES2
#  include "SDL_opengles2.h"
#  define MEMMAP_TEX_FORMAT GL_RGBA
#else
#  include "SDL_opengl.h"
#  define MEMMAP_TEX_FORMAT GL_RGBA8
#endif
//-----------------------------------------------------------------------------
#define MEMMAP_TEX_SIZE 256
#define MEMMAP_TEX_SCALE 2
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
void UserInterface::InitDebugSuite()
{
	memEditor = new MemoryEditor();
	memEditorBuffer = new BYTE[MEM_MAX]; // 64KB working buffer for memEditor
	memMapReadBuffer = new BYTE[MEM_MAX];
	memMapPixelBuffer = new DWORD[MEM_MAX];
	memMapTexture = 0;
	memset(memEditorBuffer, 0, MEM_MAX);
	memset(memMapReadBuffer, 0, MEM_MAX);
	memset(memMapPixelBuffer, 0, MEM_MAX * sizeof(DWORD));

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

	glGenTextures(1, &memMapTexture);
	glBindTexture(GL_TEXTURE_2D, memMapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, MEMMAP_TEX_FORMAT, MEMMAP_TEX_SIZE, MEMMAP_TEX_SIZE, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (BYTE *)(memMapPixelBuffer));
	glBindTexture(GL_TEXTURE_2D, 0);
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
	if (memMapReadBuffer) {
		delete[] memMapReadBuffer;
		memMapReadBuffer = NULL;
	}
	if (memMapPixelBuffer) {
		delete[] memMapPixelBuffer;
		memMapPixelBuffer = NULL;
	}
	if (memMapTexture) {
		glDeleteTextures(1, &memMapTexture);
		memMapTexture = 0;
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
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
		}
		else if (line.color == COL_CURRENT) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.9f, 0.9f, 0.9f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		else if (line.color == COL_BREAKPT) {
			selectableFlags |= ImGuiSelectableFlags_Highlight;
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.8f, 0.4f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.8f, 0.4f, 0.2f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
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

		ImVec4 branchColor = ImVec4(0.9f, 0.7f, 0.0f, 1.0f);
		if (line.color >= COL_CURRENT) {
			ImGui::PopStyleColor(3);
			branchColor = ImVec4(0.3f, 0.1f, 0.0f, 1.0f);
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
					radix ? "Look-up #%04X" : "Look-up %05d", line.lookupAddress);
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
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "<");
			}
			else {
				ImGui::SameLine(widthWidth - GetMonoTextWidth(1, 0.0f));
				ImGui::TextColored(branchColor, "%s", direction);
			}
		}
		if (line.isBreakPoint) {
			ImGui::SameLine(breakpointOffset);
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
void UserInterface::DrawDebugWindow()
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
				if (sscanf(c, "%" _pfSizeT "X", &gotoAddr) == 1) {
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
void UserInterface::DrawMemMapDialog()
{
	static ImGuiWindowFlags memmap_dialog_flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	if (Settings->GUI->dialogMemMapOpened) {
		ImGuiStyle& style = ImGui::GetStyle();
		float titleBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y;
		ImVec2 windowSize = ImVec2(
			MEMMAP_TEX_SIZE * MEMMAP_TEX_SCALE,
			MEMMAP_TEX_SIZE * MEMMAP_TEX_SCALE + titleBarHeight
		) + (style.WindowPadding * 2.0f);

		ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSizeConstraints(windowSize, windowSize);

		if (ImGui::Begin("Memory Map", &Settings->GUI->dialogMemMapOpened, memmap_dialog_flags)) {
			if (!Debugger->GetMem(memMapReadBuffer, 0, MEM_MAX))
				memset(memMapReadBuffer, 0, MEM_MAX);

			for (int i = 0; i < MEM_MAX; i++) {
				BYTE value = memMapReadBuffer[i];
				memMapPixelBuffer[i] = DWORD_COLOR_ENTRY(value, value, value);
			}

			glBindTexture(GL_TEXTURE_2D, memMapTexture);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, MEMMAP_TEX_SIZE, MEMMAP_TEX_SIZE,
				GL_RGBA, GL_UNSIGNED_BYTE, (BYTE *)(memMapPixelBuffer));
			glBindTexture(GL_TEXTURE_2D, 0);

			ImVec2 imagePos = ImGui::GetCursorScreenPos();
			ImGui::Image(
				(ImTextureID) (intptr_t) memMapTexture,
				ImVec2(
					MEMMAP_TEX_SIZE * MEMMAP_TEX_SCALE,
					MEMMAP_TEX_SIZE * MEMMAP_TEX_SCALE
				)
			);

			if (ImGui::IsItemHovered()) {
				ImVec2 mousePos = ImGui::GetMousePos();
				int px = (int) (mousePos.x - imagePos.x) / MEMMAP_TEX_SCALE;
				int py = (int) (mousePos.y - imagePos.y) / MEMMAP_TEX_SCALE;

				if (px >= 0 && px < MEMMAP_TEX_SIZE && py >= 0 && py < MEMMAP_TEX_SIZE) {
					WORD addr = (WORD) (py * MEMMAP_TEX_SIZE + px);
					ImGui::SetTooltip(
						radix ? "#%04X:#%02X" : "%05d:%03d",
						addr, memMapReadBuffer[addr]
					);
				}
			}
		}

		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
