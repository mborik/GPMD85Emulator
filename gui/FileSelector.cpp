/*	FileSelector.cpp: Part of GUI rendering class: File selector
	Copyright (c) 2011-2012 Martin Borik <mborik@users.sourceforge.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
//-----------------------------------------------------------------------------
#include "UserInterface.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
void UserInterface::DrawFileSelector(bool update)
{
	if (update || fileSelector->dirEntries == NULL)
		ScanDir(fileSelector->path, &fileSelector->dirEntries, &fileSelector->count, Settings->showHiddenFiles);

	if (fileSelector->count <= 0) {
		MenuClose();
		return;
	}

	cMenu_data = NULL;
	cMenu_hilite = cMenu_leftMargin = 0;
	cMenu_count = fileSelector->count;

	cMenu_rect->w = GUI_CONST_BORDER + (maxCharsOnScreen * fontWidth) + GUI_CONST_BORDER;
	cMenu_rect->h = (3 * GUI_CONST_BORDER) + (17 * GUI_CONST_ITEM_SIZE) + GUI_CONST_BORDER + GUI_CONST_SEPARATOR;
	cMenu_rect->x = 1;
	cMenu_rect->y = (frameHeight - cMenu_rect->h) / 2;

	fileSelector->itemsOnPage = (fileSelector->type == GUI_FS_SNAPSAVE) ? 30 : 32;

	GUI_SURFACE *defaultSurface = LockSurface(defaultTexture);

	DrawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y,
		cMenu_rect->w, cMenu_rect->h);
	PrintTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1,
		cMenu_rect->w, GUI_COLOR_BACKGROUND, fileSelector->title);
	DrawLineH(defaultSurface, cMenu_rect->x + (GUI_CONST_BORDER / 2),
		cMenu_rect->y + (3 * GUI_CONST_BORDER) +
		((fileSelector->itemsOnPage / 2) * GUI_CONST_ITEM_SIZE) + 4,
		cMenu_rect->w - GUI_CONST_BORDER, GUI_COLOR_SEPARATOR);

	int mx = cMenu_rect->x + cMenu_rect->w - GUI_CONST_BORDER - 1,
		my = cMenu_rect->y + cMenu_rect->h - 6 - fontHeight;

	PrintText(defaultSurface, mx - (6 * fontWidth) - GUI_CONST_HOTKEYCHAR, my,
		GUI_COLOR_FOREGROUND, "HOME \a\203\aH");

	if (fileSelector->type == GUI_FS_SNAPSAVE) {
		PrintText(defaultSurface, mx - (14 * fontWidth), my - fontLineHeight,
			GUI_COLOR_FOREGROUND, "ENTER NAME \aT\aA\aB");
	}

	// quick-search mini window
	if (strlen(fileSelector->search)) {
		int hx = (21 * fontWidth), hy = 6 + ((fileSelector->type == GUI_FS_SNAPSAVE) ? fontLineHeight + 2 : 0);
		DrawRectangle(defaultSurface, mx - hx - 2, my - hy, hx + 8, fontHeight + 6, GUI_COLOR_BACKGROUND);
		DrawOutlineRounded(defaultSurface, mx - hx - 2, my - hy, hx + 8, fontHeight + 6, GUI_COLOR_DISABLED);
		PrintText(defaultSurface, mx - hx + 2, my - hy + 3, GUI_COLOR_SMARTKEY, fileSelector->search);
	}

	mx = cMenu_rect->x + GUI_CONST_BORDER;
	if (fileSelector->type == GUI_FS_BASESAVE) {
		PrintText(defaultSurface, mx, my,
			GUI_COLOR_FOREGROUND, "\aT\aA\aB ENTER NAME");
	}
	else if (fileSelector->type == GUI_FS_SNAPLOAD) {
		PrintCheck(defaultSurface, mx, my + 1, GUI_COLOR_CHECKED,
			SCHR_CHECK, Settings->Snapshot->dontRunOnLoad);
		PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
			GUI_COLOR_FOREGROUND, "\a\203\aD DEBUG AFTER LOAD");
	}
	else if (fileSelector->type == GUI_FS_SNAPSAVE) {
		PrintCheck(defaultSurface, mx, my + 1, GUI_COLOR_CHECKED,
			SCHR_CHECK, Settings->Snapshot->saveWithMonitor);
		PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
			GUI_COLOR_FOREGROUND, "\a\203\aR SAVE WITH ROM");

		PrintCheck(defaultSurface, mx, my - fontLineHeight + 1,
			GUI_COLOR_CHECKED, SCHR_CHECK, Settings->Snapshot->saveCompressed);
		PrintText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my - fontLineHeight,
			GUI_COLOR_FOREGROUND, "\a\203\aC SAVE COMPRESSED");
	}

	char *ptr = fileSelector->path, c = *ptr;
	if (strlen(ptr) > maxCharsOnScreen) {
		while (strlen(ptr) > (DWORD) (maxCharsOnScreen - 1))
			ptr++;
		c = *(--ptr);
		*ptr = (char) SCHR_BROWSE;
	}

	PrintText(defaultSurface, cMenu_rect->x + GUI_CONST_BORDER,
		cMenu_rect->y + GUI_CONST_ITEM_SIZE + 1, GUI_COLOR_BORDER, ptr);

	*ptr = c;

	DrawFileSelectorItems(defaultSurface);

	UnlockSurface(defaultTexture, defaultSurface);
}
//-----------------------------------------------------------------------------
void UserInterface::DrawFileSelectorItems(GUI_SURFACE *s)
{
	bool needUnlock = false;
	if (s == NULL) {
		s = LockSurface(defaultTexture);
		needUnlock = true;
	}

	int i, x, y, c, halfpage = (fileSelector->itemsOnPage / 2),
		itemCharWidth = (maxCharsOnScreen / 2) - 1,
		itemPixelWidth = (itemCharWidth * fontWidth) + GUI_CONST_HOTKEYCHAR - 1;

	SDL_Rect *r = new SDL_Rect(*cMenu_rect);
	char ptr[32], *wrk;

	r->x += GUI_CONST_BORDER;
	r->y += (3 * GUI_CONST_BORDER) + 2;
	r->w -= (2 * GUI_CONST_BORDER);

	PrintChar(s, r->x + r->w, r->y + 1, (cMenu_leftMargin > 0)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_UP);

	for (i = cMenu_leftMargin; i < (cMenu_leftMargin + fileSelector->itemsOnPage); i++) {
		x = r->x + (((i - cMenu_leftMargin) >= halfpage) ? itemPixelWidth : 0);
		y = r->y + (((i - cMenu_leftMargin) % halfpage) * GUI_CONST_ITEM_SIZE);

		DrawRectangle(s, x - 2, y, itemPixelWidth, GUI_CONST_ITEM_SIZE,
			(i == cMenu_hilite) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);

		if (i >= cMenu_count)
			continue;

		strncpy(ptr, fileSelector->dirEntries[i], 31);
		ptr[31] = '\0';

		if (ptr[0] == DIR_DELIMITER) {
			c = GUI_COLOR_HOTKEY;
			PrintChar(s, x, y + 2, c, SCHR_DIRECTORY);
		}
		else if (ptr[0] == '\xA0') {
			if (fileSelector->extFilter) {
				c = GUI_COLOR_DISABLED;
				wrk = strrchr(fileSelector->dirEntries[i], '.');
				if (wrk) {
					wrk++;
					for (int j = 0; fileSelector->extFilter[j] != NULL; j++) {
						if (strcasecmp(wrk, fileSelector->extFilter[j]) == 0) {
							c = GUI_COLOR_FOREGROUND;
							break;
						}
					}
				}
			}
			else
				c = GUI_COLOR_FOREGROUND;
		}
		else {
			c = GUI_COLOR_HOTKEY;
			ptr[1] = (char) SCHR_BROWSE;
			ptr[2] = '\0';
		}

		if (strlen(ptr) > (DWORD) itemCharWidth) {
			while (strlen(ptr) > (DWORD) (itemCharWidth - 1))
				ptr[strlen(ptr) - 1] = '\0';
			ptr[strlen(ptr)] = (char) SCHR_BROWSE;
		}

		PrintText(s, x + GUI_CONST_HOTKEYCHAR, y + 2, c, ptr + 1);
	}

	PrintChar(s, r->x + r->w,
		r->y + ((halfpage - 1) * GUI_CONST_ITEM_SIZE) + 2, (i < cMenu_count)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_DW);

	if (needUnlock)
		UnlockSurface(defaultTexture, s);

	delete r;
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerFileSelector(WORD key)
{
	int i = cMenu_hilite, halfpage = fileSelector->itemsOnPage / 2,
			prevLeftMargin = 0, searchlen = strlen(fileSelector->search);
	char *ptr = fileSelector->dirEntries[cMenu_hilite], *lastItem;
	bool change = false;
	BYTE b = 0;

	switch (key) {
		case SDL_SCANCODE_F4 | KM_ALT:
			Emulator->ActionExit();
			MenuCloseAll();
			return;

		case SDL_SCANCODE_ESCAPE:
			if (searchlen > 0) {
				change = KeyhandlerFileSelectorSearchClean();
				needRelease = true;
			}
			else {
				MenuClose();
				return;
			}
			break;

		case SDL_SCANCODE_C | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPSAVE) {
				prevLeftMargin = cMenu_leftMargin;
				Settings->Snapshot->saveCompressed = !Settings->Snapshot->saveCompressed;
				DrawFileSelector(false);
				change = true;
			}
			break;

		case SDL_SCANCODE_D | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPLOAD) {
				prevLeftMargin = cMenu_leftMargin;
				Settings->Snapshot->dontRunOnLoad = !Settings->Snapshot->dontRunOnLoad;
				DrawFileSelector(false);
				change = true;
			}
			break;

		case SDL_SCANCODE_R | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPSAVE) {
				prevLeftMargin = cMenu_leftMargin;
				Settings->Snapshot->saveWithMonitor = !Settings->Snapshot->saveWithMonitor;
				DrawFileSelector(false);
				change = true;
			}
			break;

		case SDL_SCANCODE_H | KM_ALT:
			KeyhandlerFileSelectorSearchClean();
			strcpy(fileSelector->path, PathUserHome);
			DrawFileSelector();
			needRelease = true;
			break;

		case SDL_SCANCODE_PERIOD | KM_ALT:
			KeyhandlerFileSelectorSearchClean();
			Settings->showHiddenFiles = !Settings->showHiddenFiles;
			DrawFileSelector();
			needRelease = true;
			break;

		case SDL_SCANCODE_TAB:
			if (fileSelector->type == GUI_FS_BASESAVE
			 || fileSelector->type == GUI_FS_SNAPSAVE) {
				char buffer[40];
				buffer[0] = '\0';

				if (*ptr == '\xA0')
					strcpy(buffer, ptr + 1);

				if (EditBox("ENTER FILENAME:", NULL, buffer, 32, false) && strlen(buffer) > 0) {
					for (b = 0; b < strlen(buffer); b++) {
						switch (buffer[b]) {
						// multiplatform restricted characters in filenames;
						// filtering to avoid file portability problems;
							case ' ':  case '/':  case '\\': case '?':
							case '*':  case '%':  case ':':  case '|':
							case '"':  case '<':  case '>':
								buffer[b] = '_';
								break;

							default:
								break;
						}
					}

					if (fileSelector->extFilter) {
						if ((ptr = strrchr(buffer, '.'))) {
							for (b = 0; fileSelector->extFilter[b] != NULL; b++) {
								if (strcasecmp(ptr + 1, fileSelector->extFilter[b]) == 0) {
									KeyhandlerFileSelectorCallback(buffer);
									return;
								}
							}
						}

						ptr = buffer + strlen(buffer);
						*ptr = '.';
						strcpy(ptr + 1, fileSelector->extFilter[0]);
					}

					KeyhandlerFileSelectorCallback(buffer);
					return;
				}
			}
			break;

		case SDL_SCANCODE_SPACE:
			if (searchlen > 0) {
				i = KeyhandlerFileSelectorSearch(i);
				if (i < 0)
					i = KeyhandlerFileSelectorSearch();
				if (i < 1)
					i = cMenu_hilite;
				DrawFileSelector();
				change = true;
			}
			break;

		case SDL_SCANCODE_BACKSPACE:
			if (searchlen > 1) {
				fileSelector->search[searchlen - 1] = '\0';
				i = KeyhandlerFileSelectorSearch();
				if (i < 1)
					i = cMenu_hilite;
				DrawFileSelector();
				change = true;
				break;
			}
			else if (searchlen == 1) {
				change = KeyhandlerFileSelectorSearchClean();
				needRelease = true;
				break;
			}
			else
				ptr = fileSelector->dirEntries[0];
				/* no break, only select ".." and continue in next case... */

		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_KP_ENTER:
			change = KeyhandlerFileSelectorSearchClean();
			if (*ptr == '\xA0') {
				lastItem = ptr + 1;
				if ((ptr = strrchr(lastItem, '.')) && fileSelector->extFilter) {
					for (b = 0; fileSelector->extFilter[b] != NULL; b++) {
						if (strcasecmp(ptr + 1, fileSelector->extFilter[b]) == 0) {
							KeyhandlerFileSelectorCallback(lastItem);
							return;
						}
					}
				}
				else if (!fileSelector->extFilter) {
					KeyhandlerFileSelectorCallback(lastItem);
					return;
				}
			}
			else if (TestDir(fileSelector->path, ptr + 1, &lastItem)) {
				cMenu_hilite = 0;
				DrawFileSelector();
				needRelease = true;

				if (lastItem) {
					for (i = 0; i < cMenu_count; i++) {
						ptr = fileSelector->dirEntries[i];
						if (strcmp(ptr + 1, lastItem) == 0) {
							change = true;
							break;
						}
					}
				}
				else
					return;
			}
			break;

		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_PAGEUP:
			change = KeyhandlerFileSelectorSearchClean();
			if (i > 0) {
				i -= (key == SDL_SCANCODE_LEFT) ? halfpage : fileSelector->itemsOnPage;
				if (i < 0)
					i = 0;
				change = true;
			}
			break;

		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_PAGEDOWN:
			change = KeyhandlerFileSelectorSearchClean();
			if (i < (cMenu_count - 1)) {
				i += (key == SDL_SCANCODE_RIGHT) ? halfpage : fileSelector->itemsOnPage;
				if (i >= cMenu_count)
					i = (cMenu_count - 1);
				change = true;
			}
			break;

		case SDL_SCANCODE_UP:
			change = KeyhandlerFileSelectorSearchClean();
			if (i > 0) {
				i--;
				change = true;
			}
			break;

		case SDL_SCANCODE_DOWN:
			change = KeyhandlerFileSelectorSearchClean();
			if (i < (cMenu_count - 1)) {
				i++;
				change = true;
			}
			break;

		case SDL_SCANCODE_HOME:
			KeyhandlerFileSelectorSearchClean();
			i = 0;
			needRelease = true;
			change = true;
			break;

		case SDL_SCANCODE_END:
			KeyhandlerFileSelectorSearchClean();
			i = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			b = (key & 127);
			if (searchlen < 21 &&
				((b > SDL_SCANCODE_SPACE && b <= SDL_SCANCODE_Z) ||
				 (key >= SDL_SCANCODE_KP_0 && key <= SDL_SCANCODE_KP_PLUS))) {

				if (key & KM_SHIFT) {
					if (b >= 'a' && b <= 'z')
						b ^= 32;
					else switch (b) {
					// filtered only multiplatform restricted
					// characters to avoid portability problems...
						case '[': b = '{'; break;
						case ']': b = '}'; break;
						case '-': b = '_'; break;
						case '=': b = '+'; break;
						case '`': b = '~'; break;
						case '1': b = '!'; break;
						case '2': b = '@'; break;
						case '3': b = '#'; break;
						case '4': b = '$'; break;
						case '5': b = '%'; break;
						case '6': b = '^'; break;
						case '7': b = '&'; break;
						case '9': b = '('; break;
						case '0': b = ')'; break;
						default:  b = 0; break;
					}
				}
				else if (key >= SDL_SCANCODE_KP_0 && key <= SDL_SCANCODE_KP_9)
					b ^= 48;
				else if (key == SDL_SCANCODE_KP_PERIOD)
					b = '.';
				else if (key == SDL_SCANCODE_KP_PLUS)
					b = '+';
				else if (key == SDL_SCANCODE_KP_MINUS)
					b = '-';
				else if (!((b >= 'a' && b <= 'z') || (b >= '0' && b <= '9'))) {
					switch (b) {
					// filtered only multiplatform restricted
					// characters to avoid portability problems...
						case '`': case '-': case '=': case '[': case ']':
						case ';': case ',': case '.': case '\'':
							break;
						default:
							b = 0;
							break;
					}
				}

				fileSelector->search[searchlen++] = b;
				fileSelector->search[searchlen] = '\0';
				i = KeyhandlerFileSelectorSearch();
				if (i < 1) {
					fileSelector->search[--searchlen] = '\0';
					i = cMenu_hilite;
				}
				change = true;
				DrawFileSelector();
			}
			break;
	}

	if (change) {
		if (prevLeftMargin)
			cMenu_leftMargin = prevLeftMargin;

		while (i < cMenu_leftMargin) {
			cMenu_leftMargin -= halfpage;
			if (cMenu_leftMargin < 0)
				cMenu_leftMargin = 0;
		}

		if (i >= (cMenu_leftMargin + fileSelector->itemsOnPage))
			cMenu_leftMargin = i - halfpage;

		cMenu_hilite = i;
		DrawFileSelectorItems();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::KeyhandlerFileSelectorCallback(char *fileName)
{
	BYTE ret = fileSelector->tag;

	char *ptr = fileSelector->path + strlen(fileSelector->path);
	*ptr = DIR_DELIMITER;
	*(ptr + 1) = '\0';
	strcat(fileSelector->path, fileName);

	if ((fileSelector->type == GUI_FS_BASESAVE
	  || fileSelector->type == GUI_FS_SNAPSAVE)
	  && FileExists(fileSelector->path)) {

		if (QueryDialog("OVERWRITE?", false) != GUI_QUERY_YES) {
			*ptr = '\0';
			return;
		}
	}

	fileSelector->callback(fileSelector->path, &ret);
	if (ret == 0) {
		MenuCloseAll();
		uiSetChanges = PS_CLOSEALL;
	}
	else if (ret == 1)
		MenuClose();
	else
		*ptr = '\0';
}
//-----------------------------------------------------------------------------
int UserInterface::KeyhandlerFileSelectorSearch(int from)
{
	char *ptr;

	for (int i = from + 1; i < fileSelector->count; i++) {
		ptr = fileSelector->dirEntries[i];
		if (strncmp(ptr + 1, fileSelector->search, strlen(fileSelector->search)) == 0)
			return i;
	}

	return -1;
}
//-----------------------------------------------------------------------------
bool UserInterface::KeyhandlerFileSelectorSearchClean()
{
	if (fileSelector->search[0] != '\0') {
		fileSelector->search[0] = '\0';
		DrawFileSelector();
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------
