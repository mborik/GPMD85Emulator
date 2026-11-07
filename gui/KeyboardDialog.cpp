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
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
#define CTL 16
//-----------------------------------------------------------------------------
enum TKeyColor {
	KC_END = -1, KC_NEWLINE = 0,
	KC_GRAY = 1,
	KC_BLUE = 2,
	KC_RED  = 3,
	KC_SPACE, // wide space key
	KC_DUMMY  // spacer
};
typedef struct _VKEY {
	TKeyColor color;
	BYTE column;
	BYTE rowmask;
	const char* label;
	const char* shifted;
	const char* ctrled;
} VKEY;
static const VKEY keymapPMD85[] = {
	{ KC_BLUE,  0,  1, "K0", NULL, NULL },
	{ KC_BLUE,  1,  1, "K1", NULL, NULL },
	{ KC_BLUE,  2,  1, "K2", NULL, NULL },
	{ KC_BLUE,  3,  1, "K3", NULL, NULL },
	{ KC_BLUE,  4,  1, "K4", NULL, NULL },
	{ KC_BLUE,  5,  1, "K5", NULL, NULL },
	{ KC_BLUE,  6,  1, "K6", NULL, NULL },
	{ KC_BLUE,  7,  1, "K7", NULL, NULL },
	{ KC_BLUE,  8,  1, "K8", NULL, NULL },
	{ KC_BLUE,  9,  1, "K9", NULL, NULL },
	{ KC_BLUE, 10,  1, "K10", NULL, NULL },
	{ KC_BLUE, 11,  1, "K11", NULL, NULL },
	{ KC_BLUE, 12,  1, "WRK", NULL, NULL },
	{ KC_BLUE, 13,  1, "C-D", NULL, NULL },
	{ KC_BLUE, 14,  1, "RCL", NULL, NULL },
	{ KC_NEWLINE },

	{ KC_GRAY,  0,  2, "1", "!", NULL },
	{ KC_GRAY,  1,  2, "2", "\"", NULL },
	{ KC_GRAY,  2,  2, "3", "#", NULL },
	{ KC_GRAY,  3,  2, "4", "$", NULL },
	{ KC_GRAY,  4,  2, "5", "%", NULL },
	{ KC_GRAY,  5,  2, "6", "&", NULL },
	{ KC_GRAY,  6,  2, "7", "'", NULL },
	{ KC_GRAY,  7,  2, "8", "(", NULL },
	{ KC_GRAY,  8,  2, "9", ")", NULL },
	{ KC_GRAY,  9,  2, "0", "-", NULL },
	{ KC_GRAY, 10,  2, "_", "=", NULL },
	{ KC_GRAY, 11,  2, "}", "{", "CSL" },
	{ KC_BLUE, 12,  2, "INS", "PTL", NULL },
	{ KC_BLUE, 13,  2, "DEL", NULL, NULL },
	{ KC_BLUE, 14,  2, "CLR", NULL, NULL },
	{ KC_NEWLINE },

	{ KC_DUMMY, 5 },
	{ KC_GRAY,  0,  4, "Q", "q", NULL },
	{ KC_GRAY,  1,  4, "W", "w", NULL },
	{ KC_GRAY,  2,  4, "E", "e", NULL },
	{ KC_GRAY,  3,  4, "R", "r", NULL },
	{ KC_GRAY,  4,  4, "T", "t", NULL },
	{ KC_GRAY,  5,  4, "Z", "z", NULL },
	{ KC_GRAY,  6,  4, "U", "u", NULL },
	{ KC_GRAY,  7,  4, "I", "i", NULL },
	{ KC_GRAY,  8,  4, "O", "o", NULL },
	{ KC_GRAY,  9,  4, "P", "p", NULL },
	{ KC_GRAY, 10,  4, "@", "`", NULL },
	{ KC_GRAY, 11,  4, "\\", "^", NULL },
	{ KC_BLUE, 12,  4, "←", NULL, NULL },
	{ KC_BLUE, 13,  4, "↖", NULL, NULL },
	{ KC_BLUE, 14,  4, "→", NULL, NULL },
	{ KC_NEWLINE },

	{ KC_DUMMY, 10 },
	{ KC_GRAY,  0,  8, "A", "a", NULL },
	{ KC_GRAY,  1,  8, "S", "s", NULL },
	{ KC_GRAY,  2,  8, "D", "d", NULL },
	{ KC_GRAY,  3,  8, "F", "f", NULL },
	{ KC_GRAY,  4,  8, "G", "g", NULL },
	{ KC_GRAY,  5,  8, "H", "h", NULL },
	{ KC_GRAY,  6,  8, "J", "j", NULL },
	{ KC_GRAY,  7,  8, "K", "k", NULL },
	{ KC_GRAY,  8,  8, "L", "l", NULL },
	{ KC_GRAY,  9,  8, ";", "+", NULL },
	{ KC_GRAY, 10,  8, ":", "*", NULL },
	{ KC_GRAY, 11,  8, "]", "[", NULL },
	{ KC_BLUE, 12,  8, "⎹←", NULL, NULL },
	{ KC_BLUE, 13,  8, "END", NULL, NULL },
	{ KC_BLUE, 14,  8, "→⎸", NULL, NULL },
	{ KC_NEWLINE },

	{ KC_DUMMY, 15 },
	{ KC_RED, CTL, 32, "↑", NULL, NULL },
	{ KC_GRAY,  1, 16, "Y", "y", NULL },
	{ KC_GRAY,  2, 16, "X", "x", NULL },
	{ KC_GRAY,  3, 16, "C", "c", NULL },
	{ KC_GRAY,  4, 16, "V", "v", NULL },
	{ KC_GRAY,  5, 16, "B", "b", NULL },
	{ KC_GRAY,  6, 16, "N", "n", NULL },
	{ KC_GRAY,  7, 16, "M", "m", NULL },
	{ KC_GRAY,  8, 16, ",", "<", NULL },
	{ KC_GRAY,  9, 16, ".", ">", NULL },
	{ KC_GRAY, 10, 16, "/", "?", NULL },
	{ KC_RED, CTL, 32, "↑", NULL, NULL },
	{ KC_RED, CTL, 64, "STP", NULL, NULL },
	{ KC_RED,  13, 16, "EOL", NULL, NULL },
	{ KC_RED,  14, 16, "EOL", NULL, NULL },
	{ KC_NEWLINE },

	{ KC_DUMMY, 70 },
	{ KC_SPACE, 0, 16, " ", NULL, NULL },
	{ KC_END },
};
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
		BYTE modMaskShift = 0x20;
		BYTE modMaskCtrl = 0x40;

		// TODO Emulator->Settings->CurrentModel->type == CM_MATO | CM_C2717

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

			float button_width = base_width * (key->color == KC_SPACE ? 4.0f : 1.0f);

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