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
//-----------------------------------------------------------------------------
enum TKeyColor { KC_ENDLINE = -1, KC_BASE = 0, KC_BLUE, KC_RED, KC_SPACE, KC_DUMMY };
typedef struct _VKEY {
	TKeyColor color;
	WORD value;
	const char* label;
	const char* shifted;
	const char* ctrled;
} VKEY;
static const VKEY keymapPMD85[] = {
	{ KC_BLUE, SDL_SCANCODE_F1, "K0", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F2, "K1", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F3, "K2", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F4, "K3", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F5, "K4", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F6, "K5", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F7, "K6", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F8, "K7", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F9, "K8", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F10, "K9", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F11, "K10", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_F12, "K11", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_GRAVE, "WRK", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_TAB, "C-D", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_PAGEUP, "RCL", NULL, NULL },
	{ KC_ENDLINE },

	{ KC_BASE, SDL_SCANCODE_1, "1", "!", NULL },
	{ KC_BASE, SDL_SCANCODE_2, "2", "\"", NULL },
	{ KC_BASE, SDL_SCANCODE_3, "3", "#", NULL },
	{ KC_BASE, SDL_SCANCODE_4, "4", "$", NULL },
	{ KC_BASE, SDL_SCANCODE_5, "5", "%", NULL },
	{ KC_BASE, SDL_SCANCODE_6, "6", "&", NULL },
	{ KC_BASE, SDL_SCANCODE_7, "7", "'", NULL },
	{ KC_BASE, SDL_SCANCODE_8, "8", "(", NULL },
	{ KC_BASE, SDL_SCANCODE_9, "9", ")", NULL },
	{ KC_BASE, SDL_SCANCODE_0, "0", "-", NULL },
	{ KC_BASE, SDL_SCANCODE_MINUS, "_", "=", NULL },
	{ KC_BASE, SDL_SCANCODE_EQUALS, "}", "{", "CSL" },
	{ KC_BLUE, SDL_SCANCODE_INSERT, "INS", "PTL", NULL },
	{ KC_BLUE, SDL_SCANCODE_DELETE, "DEL", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_PAGEDOWN, "CLR", NULL, NULL },
	{ KC_ENDLINE },

	{ KC_DUMMY, 5 },
	{ KC_BASE, SDL_SCANCODE_Q, "Q", "q", NULL },
	{ KC_BASE, SDL_SCANCODE_W, "W", "w", NULL },
	{ KC_BASE, SDL_SCANCODE_E, "E", "e", NULL },
	{ KC_BASE, SDL_SCANCODE_R, "R", "r", NULL },
	{ KC_BASE, SDL_SCANCODE_T, "T", "t", NULL },
	{ KC_BASE, SDL_SCANCODE_Z, "Z", "z", NULL },
	{ KC_BASE, SDL_SCANCODE_U, "U", "u", NULL },
	{ KC_BASE, SDL_SCANCODE_I, "I", "i", NULL },
	{ KC_BASE, SDL_SCANCODE_O, "O", "o", NULL },
	{ KC_BASE, SDL_SCANCODE_P, "P", "p", NULL },
	{ KC_BASE, SDL_SCANCODE_LEFTBRACKET, "@", "`", NULL },
	{ KC_BASE, SDL_SCANCODE_RIGHTBRACKET, "\\", "^", NULL },
	{ KC_BLUE, SDL_SCANCODE_LEFT, "←", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_HOME, "↖", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_RIGHT, "→", NULL, NULL },
	{ KC_ENDLINE },

	{ KC_DUMMY, 10 },
	{ KC_BASE, SDL_SCANCODE_A, "A", "a", NULL },
	{ KC_BASE, SDL_SCANCODE_S, "S", "s", NULL },
	{ KC_BASE, SDL_SCANCODE_D, "D", "d", NULL },
	{ KC_BASE, SDL_SCANCODE_F, "F", "f", NULL },
	{ KC_BASE, SDL_SCANCODE_G, "G", "g", NULL },
	{ KC_BASE, SDL_SCANCODE_H, "H", "h", NULL },
	{ KC_BASE, SDL_SCANCODE_J, "J", "j", NULL },
	{ KC_BASE, SDL_SCANCODE_K, "K", "k", NULL },
	{ KC_BASE, SDL_SCANCODE_L, "L", "l", NULL },
	{ KC_BASE, SDL_SCANCODE_SEMICOLON, ";", "+", NULL },
	{ KC_BASE, SDL_SCANCODE_APOSTROPHE, ":", "*", NULL },
	{ KC_BASE, SDL_SCANCODE_BACKSLASH, "]", "[", NULL },
	{ KC_BLUE, SDL_SCANCODE_UP, "⎹←", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_END, "END", NULL, NULL },
	{ KC_BLUE, SDL_SCANCODE_DOWN, "→⎸", NULL, NULL },
	{ KC_ENDLINE },

	{ KC_DUMMY, 15 },
	{ KC_RED, SDL_SCANCODE_LSHIFT, "↑", NULL, NULL },
	{ KC_BASE, SDL_SCANCODE_Y, "Y", "y", NULL },
	{ KC_BASE, SDL_SCANCODE_X, "X", "x", NULL },
	{ KC_BASE, SDL_SCANCODE_C, "C", "c", NULL },
	{ KC_BASE, SDL_SCANCODE_V, "V", "v", NULL },
	{ KC_BASE, SDL_SCANCODE_B, "B", "b", NULL },
	{ KC_BASE, SDL_SCANCODE_N, "N", "n", NULL },
	{ KC_BASE, SDL_SCANCODE_M, "M", "m", NULL },
	{ KC_BASE, SDL_SCANCODE_COMMA, ",", "<", NULL },
	{ KC_BASE, SDL_SCANCODE_PERIOD, ".", ">", NULL },
	{ KC_BASE, SDL_SCANCODE_SLASH, "/", "?", NULL },
	{ KC_RED, SDL_SCANCODE_RSHIFT, "↑", NULL, NULL },
	{ KC_RED, SDL_SCANCODE_LCTRL, "STP", NULL, NULL },
	{ KC_RED, SDL_SCANCODE_KP_ENTER, "EOL", NULL, NULL },
	{ KC_RED, SDL_SCANCODE_RETURN, "EOL", NULL, NULL },
	{ KC_ENDLINE },

	{ KC_DUMMY, 70 },
	{ KC_SPACE, SDL_SCANCODE_SPACE, " ", NULL, NULL },
	{ KC_ENDLINE },
};
//-----------------------------------------------------------------------------
void UserInterface::DrawKeyboardDialog()
{
	static bool isOpening = true;
	static ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoNavInputs |
		ImGuiWindowFlags_AlwaysAutoResize;

	float button_width = GetMonoTextWidth(3.0f, 2.0f);

	if (dialogKeyboardOpened) {
		if (isOpening)
			ImGui::SetNextWindowFocus();

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1.0f, 1.0f));

		if (ImGui::Begin("Virtual Keyboard", &dialogKeyboardOpened, flags)) {
			if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
				dialogKeyboardFocused = true;

			isOpening = false;

			for (int i = 0, l = sizeof(keymapPMD85) / sizeof(VKEY); i < l; i++) {
				const VKEY *key = &keymapPMD85[i];
				if (key->color == KC_ENDLINE)
					continue;
				else if (key->color == KC_DUMMY) {
					ImGui::Dummy(ImVec2(button_width * (0.1f * (float) key->value), 0.0f));
					ImGui::SameLine();
					continue;
				}

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

				ImGui::PushStyleColor(ImGuiCol_Button, bbase);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, hover);

				ImGui::PushID(i);
				ImGui::Button(
					key->label,
					ImVec2(button_width * (key->color == KC_SPACE ? 4.0f : 1.0f), 0.0f)
				);

				BYTE keyState = Emulator->keyBuffer[key->value];
				if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && !keyState)
					Emulator->keyBuffer[key->value] = 1;
				else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && keyState)
					Emulator->keyBuffer[key->value] = 0;

				ImGui::PopStyleColor(3);
				ImGui::PopID();

				if (keymapPMD85[i + 1].color != KC_ENDLINE)
					ImGui::SameLine();
			}
		}

		ImGui::End();
		ImGui::PopStyleVar(2);
	}
	else if (dialogKeyboardFocused) {
		dialogKeyboardFocused = false;

		if (!isOpening)
			isOpening = true;
	}
}