/*	MemMapDialog.cpp: Part of GUI rendering class: Memory map dialog
	Copyright (c) 2026 Nikita Zimin <nzeemin@gmail.com>
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
void UserInterface::InitMemMapDialog()
{
	memMapReadBuffer = new BYTE[MEM_MAX];
	memMapPixelBuffer = new DWORD[MEM_MAX];
	memMapTexture = 0;
	memset(memMapReadBuffer, 0, MEM_MAX);
	memset(memMapPixelBuffer, 0, MEM_MAX * sizeof(DWORD));

	glGenTextures(1, &memMapTexture);
	glBindTexture(GL_TEXTURE_2D, memMapTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, MEMMAP_TEX_FORMAT, MEMMAP_TEX_SIZE, MEMMAP_TEX_SIZE, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (BYTE *)(memMapPixelBuffer));
	glBindTexture(GL_TEXTURE_2D, 0);
}
//-----------------------------------------------------------------------------
void UserInterface::DestroyMemMapDialog()
{
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
						Settings->Debugger->hex ? "#%04X:#%02X" : "%05d:%03d",
						addr, memMapReadBuffer[addr]
					);
				}
			}
		}

		ImGui::End();
	}
}
//-----------------------------------------------------------------------------
