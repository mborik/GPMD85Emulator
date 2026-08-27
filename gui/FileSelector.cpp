/*	FileSelector.cpp: Part of GUI rendering class: File selector
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
#include "imgui/imgui_internal.h"
//-----------------------------------------------------------------------------
void UserInterface::FileSelector(
	TFileSelectType type,
	const char *title,
	const char *recentFile,
	const std::vector<std::string> &filter,
	bool fallbackToResourceDir)
{
	if (fileSelector) {
		warning("GUI", "File selector is already allocated!");
		return;
	}

	int flags =
		ImGuiFileBrowserFlags_CloseOnEsc |
		ImGuiFileBrowserFlags_SkipItemsCausingError;

	switch (type) {
		case GUI_FS_OPEN:
			flags |= ImGuiFileBrowserFlags_ConfirmOnEnter;
			break;
		case GUI_FS_SAVE:
			flags |=
				ImGuiFileBrowserFlags_EnterNewFilename |
				ImGuiFileBrowserFlags_CreateNewDir;
			break;
		case GUI_FS_DIR:
			flags |=
				ImGuiFileBrowserFlags_SelectDirectory |
				ImGuiFileBrowserFlags_HideRegularFiles;
			break;
		default:
			error("GUI", "Invalid file selector type!");
			return;
	}

	std::filesystem::path initialDir;
	if (recentFile) {
		char *file = ComposeFilePath(recentFile);
		strcpy(fileSelectorRecentPath, file);
		delete [] file;

		if (!TestDir(fileSelectorRecentPath, (char *) "..", NULL))
			strcpy(fileSelectorRecentPath, PathApplication);
		initialDir = std::filesystem::path(fileSelectorRecentPath);
	}
	if (!recentFile) {
		if (fallbackToResourceDir) {
			initialDir = std::filesystem::path(PathResources);
			if (!std::filesystem::exists(initialDir))
				initialDir = std::filesystem::path(PathAppConfig);
			if (!std::filesystem::exists(initialDir)) {
				initialDir = std::filesystem::path(fileSelectorRecentPath);
			}
		}
		else
			initialDir = std::filesystem::path(fileSelectorRecentPath);
	}

	fileSelector = new ImGui::FileBrowser(flags, initialDir);
	fileSelector->SetTitle(title);
	fileSelector->SetTypeFilters(filter);

	if (flags & ImGuiFileBrowserFlags_EnterNewFilename)
		fileSelector->SetInputName(recentFile ? recentFile : "");

	fileSelector->Open();
}
//-----------------------------------------------------------------------------
void UserInterface::DrawFileSelector()
{
	static bool shouldDestroy = false;

	if (!fileSelector)
		return;
	if (shouldDestroy) {
		delete fileSelector;
		fileSelector = NULL;
		shouldDestroy = false;
		return;
	}

	fileSelector->Display();
	if (fileSelector->HasSelected()) {
		strcpy(fileSelectorPath, fileSelector->GetSelected().c_str());
		strcpy(fileSelectorRecentPath, fileSelector->GetSelected().c_str());
		if (!TestDir(fileSelectorRecentPath, (char *) "..", NULL))
			strcpy(fileSelectorRecentPath, PathApplication);

		uiFileSelectorCallback(fileSelectorPath);

		fileSelector->ClearSelected();
		fileSelector->Close();
		shouldDestroy = true;
	}

	if (!fileSelector->IsOpened()) {
		uiFileSelectorCallback(nullptr);
		fileSelector->Close();
		shouldDestroy = true;
	}
}
//-----------------------------------------------------------------------------
