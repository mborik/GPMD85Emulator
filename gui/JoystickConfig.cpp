/*	JoystickConfig.cpp: Part of GUI rendering class: Game controller handling
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
void UserInterface::DrawJoystickMenuItems(bool enabled)
{
	ImGui::BeginDisabled(!enabled);

	int devCount = Emulator->ActionJoyControllers();
	auto generateString = [&](TSettings::SetJoystickGPIO *gp) -> std::string {
		std::string result(32, 0);

		if (gp->type == JT_KEYS) {
			result = "KEYS:";
			switch (gp->ctrlUp) {
				case SDL_SCANCODE_KP_8:
					result += "Numpad";
					break;
				case SDL_SCANCODE_UP:
					result += "Cursors";
					break;
				case SDL_SCANCODE_Q:
					result += "QAOP\u25A9";
					break;
				case SDL_SCANCODE_W:
					result += "WASDC";
					break;
				default:
					result += "Custom";
					break;
			}
		}
		else {
			switch (gp->type) {
				case JT_POV:
					result = "DPAD";
					break;
				case JT_AXES:
					result = "Analog";
					break;
				case JT_BUTTONS:
					result = "Buttons";
					break;
				default:
					result = "[N/A]";
					return result;
			}

			if (devCount == 0)
				result += " [No Controller]";
			else if (gp->guid) {
				const char *controllerName = Emulator->GetControllerName(gp->guid);
				if (controllerName) {
					std::string ctrl = std::string(controllerName);
					if (ctrl.size() > 20)
						ctrl = ctrl.substr(0, 20) + "\u2026";
					result += " on " + ctrl;
				}
			}
		}

		return result;
	};

	ImGui::PushID("GPIO0");
	TSettings::SetJoystickGPIO *gpio = Settings->Joystick->GPIO0;
	bool state = !Settings->PMD32->connected && gpio->connected;

	if (ImGui::MenuItem("On K3-GPIO/0",
		Settings->PMD32->connected ? "N/A (PMD 32)" : NULL,
		state, !Settings->PMD32->connected)) {

		gpio->connected = !state;
		InvokeSettingsChange |= PS_PERIPHERALS | PS_CLOSEONLEAVE;
	}
	if (ImGui::MenuItem("Configure", generateString(gpio).c_str(), false, state)) {
		joyConfigDialogForGPIO = GP_GPIO_0;
		joyConfigDialogShouldOpen = true;
	}

	ImGui::PopID();
	ImGui::Separator();

	ImGui::PushID("GPIO1");
	gpio = Settings->Joystick->GPIO1;
	state = gpio->connected;

	if (ImGui::MenuItem("On K4-GPIO/1", NULL, state)) {
		gpio->connected = !state;
		InvokeSettingsChange |= PS_PERIPHERALS | PS_CLOSEONLEAVE;
	}

	if (ImGui::MenuItem("Configure", generateString(gpio).c_str(), false, state)) {
		joyConfigDialogForGPIO = GP_GPIO_1;
		joyConfigDialogShouldOpen = true;
	}

	ImGui::PopID();
	ImGui::EndDisabled();
}
//-----------------------------------------------------------------------------
void UserInterface::DrawJoystickConfigDialog()
{
	static bool configDialogOpened = false;
	static TSettings::SetJoystickGPIO *gpio = NULL;
	static ImGuiChildFlags childFlags =
		ImGuiChildFlags_AutoResizeY |
		ImGuiChildFlags_Borders |
		ImGuiWindowFlags_NoScrollWithMouse;

	if (joyConfigDialogShouldOpen) {
		joyConfigDialogShouldOpen = false;

		gpio = joyConfigDialogForGPIO == GP_GPIO_1 ?
			Settings->Joystick->GPIO1 :
			Settings->Joystick->GPIO0;

		ImGui::OpenPopup("Joystick Configuration");
		configDialogOpened = true;
	}

	ImVec2 childSize = ImVec2(GetMonoTextWidth(35), 0);
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Joystick Configuration", &configDialogOpened,
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize)) {

		ImGui::SeparatorText(joyConfigDialogForGPIO == GP_GPIO_1 ? "K4-GPIO/1" : "K3-GPIO/0");

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::BeginChild("MapToKeys", childSize, childFlags);

		if (ImGui::RadioButton("Map to Keyboard", gpio->type == JT_KEYS))
			gpio->type = JT_KEYS;

		static const char* items[] = {
			"Numpad Arrows + Num0",
			"Cursors + LCtrl",
			"QAOP + Space",
			"WASD + C",
			"Custom"
		};
		static const WORD joyKeymaps[4][5] = {
			/* Numpad Arrows + Num0 */
			{ SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_2, SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_0 },
			/* Cursors + LCtrl */
			{ SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_LCTRL },
			/* QAOP + Space */
			{ SDL_SCANCODE_Q, SDL_SCANCODE_A, SDL_SCANCODE_O, SDL_SCANCODE_P, SDL_SCANCODE_SPACE },
			/* WASD + C */
			{ SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_C }
		};

		int item_selected_idx = 4;
		for (int i = 0; i < 4; i++) {
			if (gpio->ctrlUp == joyKeymaps[i][0] &&
				gpio->ctrlDown == joyKeymaps[i][1] &&
				gpio->ctrlLeft == joyKeymaps[i][2] &&
				gpio->ctrlRight == joyKeymaps[i][3] &&
				gpio->ctrlFire == joyKeymaps[i][4]) {
				item_selected_idx = i;
				break;
			}
		}

		if (ImGui::BeginCombo("##mapping", items[item_selected_idx])) {
			for (int i = 0; i < IM_COUNTOF(items); i++) {
				bool is_selected = (item_selected_idx == i);
				if (ImGui::Selectable(items[i], is_selected) && i < 4) {
					item_selected_idx = i;

					gpio->ctrlUp = joyKeymaps[i][0];
					gpio->ctrlDown = joyKeymaps[i][1];
					gpio->ctrlLeft = joyKeymaps[i][2];
					gpio->ctrlRight = joyKeymaps[i][3];
					gpio->ctrlFire = joyKeymaps[i][4];
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImGui::EndChild();
		ImGui::BeginChild("GameController", childSize, childFlags);

		static char selectedController[128] = "";
		const char *preview = Emulator->GetControllerName(gpio->guid);
		auto* devices = Emulator->GetControllerDeviceNames();

		bool anyDevices = false;
		if (devices && !devices->empty())
			anyDevices = true;

		if (anyDevices) {
			if (ImGui::BeginCombo("##controller", preview)) {
				for (const auto& [guid, name] : *devices) {
					anyDevices = true;
					std::snprintf(selectedController, sizeof(selectedController), "%s [%s]", name.c_str(), guid.c_str());
					bool is_selected = (gpio->guid ? CompareNoCase(gpio->guid, guid.c_str()) == 0 : false);
					if (ImGui::Selectable(selectedController, is_selected)) {
						size_t guidlen = guid.size() + 1;
						gpio->guid = (char *) realloc(gpio->guid, guidlen * sizeof(char));
						std::strncpy(gpio->guid, guid.c_str(), guidlen);
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		else {
			ImGui::TextDisabled("No game controllers connected.");
			ImGui::BeginDisabled();
		}

		if (ImGui::RadioButton("Control by DPAD + A", gpio->type == JT_POV))
			gpio->type = JT_POV;
		if (ImGui::RadioButton("Control by ABXY + Bumpers", gpio->type == JT_BUTTONS))
			gpio->type = JT_BUTTONS;
		if (ImGui::RadioButton("Control by Analog (Axis)", gpio->type == JT_AXES))
			gpio->type = JT_AXES;

		if (gpio->type == JT_AXES) {
			static const char* controllerAxis[] = { "Left", "Right" };
			ImGui::SetNextItemWidth(GetMonoTextWidth(8));
			ImGui::SliderInt("##axis", &gpio->axis, 0, 1, controllerAxis[gpio->axis]);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(GetMonoTextWidth(24));
			int sensitivity = 100 - gpio->sensitivity;
			if (ImGui::SliderInt("##sens", &sensitivity, 0, 99, "Sensitivity: %d%%", ImGuiSliderFlags_AlwaysClamp))
				gpio->sensitivity = 100 - sensitivity;
		}

		if (!anyDevices)
			ImGui::EndDisabled();

		ImGui::EndChild();
		ImGui::PopStyleVar();

		ImGui::Separator();

		float button_width = GetMonoTextWidth(8, 8.0f);
		ImGui::SetCursorPosX(
			ImGui::GetCursorPosX() +
			(ImGui::GetContentRegionAvail().x - button_width) * 0.5f
		);
		if (ImGui::Button("Apply", ImVec2(button_width, 0.0f))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
//-----------------------------------------------------------------------------
