/*	KeyboardDialog.cpp - Virtual keyboard dialog for the emulator GUI.
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
#include "KeyboardLayouts.h"
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
void UserInterface::DrawKeyboardDialog()
{
	static bool keyMapHolder[2] = { false, false };
	static ImVec4 highlight = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	static ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoNavInputs |
		ImGuiWindowFlags_AlwaysAutoResize;

	float base_width = GetMonoTextWidth(3.0f, 2.0f);

	if (Settings->GUI->dialogKeyboardOpened) {
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));
		ImGui::Begin("Virtual Keyboard", &Settings->GUI->dialogKeyboardOpened, flags);

		BYTE *ShiftStopCtrl;
		BYTE *KeyColumns = Emulator->GetKeyState(ShiftStopCtrl);

		const VKEY *keymap = keymapPMD85;
		float space_height = 4.0f;
		BYTE modMaskShift = 0x20;
		BYTE modMaskCtrl = 0x40;

		if (Settings->CurrentModel->type == CM_C2717) {
			keymap = keymapC2717;
			space_height = 6.15f;
		}
		else if (
			Settings->CurrentModel->type == CM_ALFA ||
			Settings->CurrentModel->type == CM_ALFA2) {

			keymap = keymapAlfa;
			space_height = 2.0f;
		}
		else if (Settings->CurrentModel->type == CM_MATO) {
			keymap = keymapMato;
			modMaskShift = 0x20;
			modMaskCtrl = 0x40;
		}

		for (int i = 0; KeyColumns && keymap[i].color != KC_END; i++) {
			const VKEY *key = &keymap[i];
			if (key->color == KC_NEWLINE)
				continue;
			else if (key->color == KC_DUMMY) {
				ImGui::Dummy(ImVec2(base_width * (0.1f * (float) key->column), 0.0f));
				ImGui::SameLine();
				continue;
			}

			bool pressed = (
				key->column == CTL ?
					(*ShiftStopCtrl & key->rowmask) :
					(KeyColumns[key->column] & key->rowmask)
			);

			ImVec4 bbase, hover;
			if (key->color == KC_BLUE) {
				bbase = Color[GCCol_ButtonEject];
				hover = Color[GCCol_ButtonEjectHover];
			}
			else if (key->color == KC_RED) {
				bbase = Color[GCCol_ButtonWP];
				hover = Color[GCCol_ButtonWPHover];
			}
			else {
				bbase = Color[GCCol_ButtonBase];
				hover = Color[GCCol_ButtonBaseHover];
			}

			if (pressed) {
				bbase = ImLerp(bbase, highlight, 0.5f);
				hover = ImLerp(hover, highlight, 0.5f);
			}

			ImGui::PushStyleColor(ImGuiCol_Button, bbase);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

			const char *label = key->label;
			if ((*ShiftStopCtrl & modMaskCtrl) && key->ctrled)
				label = key->ctrled;
			else if ((*ShiftStopCtrl & modMaskShift) && key->shifted)
				label = key->shifted;

			float button_width = base_width *
				(key->color == KC_SPACE ? space_height : 1.0f);

			ImGui::PushID(i);
			ImGui::Button(label, ImVec2(button_width, 0.0f));

			if (ImGui::IsItemHovered()) {
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					if (key->column == CTL)
						*ShiftStopCtrl |= key->rowmask;
					else
						KeyColumns[key->column] |= key->rowmask;
				}
				else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
					if (key->column == CTL) {
						*ShiftStopCtrl &= ~key->rowmask;
						BYTE idx = (key->rowmask & modMaskShift) ? 0 : 1;
						if (keyMapHolder[idx]) {
							keyMapHolder[idx] = false;

							Emulator->keyBuffer[SDL_SCANCODE_LSHIFT] = keyMapHolder[0] ? 1 : 0;
							Emulator->keyBuffer[SDL_SCANCODE_LCTRL] = keyMapHolder[1] ? 1 : 0;
						}
					}
					else
						KeyColumns[key->column] &= ~key->rowmask;
				}
				else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && key->column == CTL) {
					BYTE idx = (key->rowmask & modMaskShift) ? 0 : 1;
					keyMapHolder[idx] = !keyMapHolder[idx];

					if (keyMapHolder[idx])
						*ShiftStopCtrl |= key->rowmask;
					else
						*ShiftStopCtrl &= ~key->rowmask;

					Emulator->keyBuffer[SDL_SCANCODE_LSHIFT] = keyMapHolder[0] ? 1 : 0;
					Emulator->keyBuffer[SDL_SCANCODE_LCTRL] = keyMapHolder[1] ? 1 : 0;
				}
			}

			ImGui::PopStyleColor(3);
			ImGui::PopID();

			if (keymap[i + 1].color > KC_NEWLINE)
				ImGui::SameLine();
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}
}
//-----------------------------------------------------------------------------
