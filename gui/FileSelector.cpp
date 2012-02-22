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
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
void UserInterface::drawFileSelector(bool update)
{
	if (update || fileSelector->dirEntries == NULL)
		ScanDir(fileSelector->path, &fileSelector->dirEntries, &fileSelector->count, Settings->showHiddenFiles);

	if (fileSelector->count <= 0) {
		menuClose();
		return;
	}

	cMenu_data = NULL;
	cMenu_hilite = cMenu_leftMargin = 0;
	cMenu_count = fileSelector->count;

	cMenu_rect->w = GUI_CONST_BORDER + (maxCharsOnScreen * fontWidth) + GUI_CONST_BORDER;
	cMenu_rect->h = (3 * GUI_CONST_BORDER) + (17 * GUI_CONST_ITEM_SIZE) + GUI_CONST_BORDER + GUI_CONST_SEPARATOR;
	cMenu_rect->x = 1;
	cMenu_rect->y = (defaultSurface->h - cMenu_rect->h) / 2;

	fileSelector->itemsOnPage = (fileSelector->type == GUI_FS_SNAPSAVE) ? 30 : 32;

	drawDialogWithBorder(defaultSurface, cMenu_rect->x, cMenu_rect->y,
		cMenu_rect->w, cMenu_rect->h);
	printTitle(defaultSurface, cMenu_rect->x, cMenu_rect->y + 1,
		cMenu_rect->w, GUI_COLOR_BACKGROUND, fileSelector->title);
	drawLineH(defaultSurface, cMenu_rect->x + (GUI_CONST_BORDER / 2),
		cMenu_rect->y + (3 * GUI_CONST_BORDER) +
		((fileSelector->itemsOnPage / 2) * GUI_CONST_ITEM_SIZE) + 4,
		cMenu_rect->w - GUI_CONST_BORDER, GUI_COLOR_SEPARATOR);

	int mx = cMenu_rect->x + cMenu_rect->w - GUI_CONST_BORDER - 1,
		my = cMenu_rect->y + cMenu_rect->h - 6 - fontHeight;

	printText(defaultSurface, mx - (6 * fontWidth) - GUI_CONST_HOTKEYCHAR, my,
		GUI_COLOR_FOREGROUND, "HOME \a\203\aH");

	if (fileSelector->type == GUI_FS_SNAPSAVE) {
		printText(defaultSurface, mx - (14 * fontWidth), my - fontLineHeight,
			GUI_COLOR_FOREGROUND, "ENTER NAME \aT\aA\aB");
	}

	// quick-search mini window
	if (strlen(fileSelector->search)) {
		int hx = (21 * fontWidth), hy = 6 + ((fileSelector->type == GUI_FS_SNAPSAVE) ? fontLineHeight + 2 : 0);
		drawRectangle(defaultSurface, mx - hx - 2, my - hy, hx + 8, fontHeight + 6, GUI_COLOR_BACKGROUND);
		drawOutlineRounded(defaultSurface, mx - hx - 2, my - hy, hx + 8, fontHeight + 6, GUI_COLOR_DISABLED);
		printText(defaultSurface, mx - hx + 2, my - hy + 3, GUI_COLOR_SMARTKEY, fileSelector->search);
	}

	mx = cMenu_rect->x + GUI_CONST_BORDER;
	if (fileSelector->type == GUI_FS_BASESAVE) {
		printText(defaultSurface, mx, my,
			GUI_COLOR_FOREGROUND, "\aT\aA\aB ENTER NAME");
	}
	else if (fileSelector->type == GUI_FS_SNAPLOAD) {
		printCheck(defaultSurface, mx, my + 1, GUI_COLOR_CHECKED,
			SCHR_CHECK, Settings->Snapshot->dontRunOnLoad);
		printText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
			GUI_COLOR_FOREGROUND, "\a\203\aD DEBUG AFTER LOAD");
	}
	else if (fileSelector->type == GUI_FS_SNAPSAVE) {
		printCheck(defaultSurface, mx, my + 1, GUI_COLOR_CHECKED,
			SCHR_CHECK, Settings->Snapshot->saveWithMonitor);
		printText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my,
			GUI_COLOR_FOREGROUND, "\a\203\aR SAVE WITH ROM");

		printCheck(defaultSurface, mx, my - fontLineHeight + 1,
			GUI_COLOR_CHECKED, SCHR_CHECK, Settings->Snapshot->saveCompressed);
		printText(defaultSurface, mx + GUI_CONST_CHK_MARGIN, my - fontLineHeight,
			GUI_COLOR_FOREGROUND, "\a\203\aC SAVE COMPRESSED");
	}

	char *ptr = fileSelector->path, c = *ptr;
	if (strlen(ptr) > maxCharsOnScreen) {
		while (strlen(ptr) > (DWORD) (maxCharsOnScreen - 1))
			ptr++;
		c = *(--ptr);
		*ptr = SCHR_BROWSE;
	}

	printText(defaultSurface, cMenu_rect->x + GUI_CONST_BORDER,
		cMenu_rect->y + GUI_CONST_ITEM_SIZE + 1, GUI_COLOR_BORDER, ptr);

	*ptr = c;

	drawFileSelectorItems();
}
//-----------------------------------------------------------------------------
void UserInterface::drawFileSelectorItems()
{
	int i, x, y, c, halfpage = (fileSelector->itemsOnPage / 2),
		itemCharWidth = (maxCharsOnScreen / 2) - 1,
		itemPixelWidth = (itemCharWidth * fontWidth) + GUI_CONST_HOTKEYCHAR - 1;

	SDL_Rect *r = new SDL_Rect(*cMenu_rect);
	char ptr[32], *wrk;

	r->x += GUI_CONST_BORDER;
	r->y += (3 * GUI_CONST_BORDER) + 2;
	r->w -= (2 * GUI_CONST_BORDER);

	printChar(defaultSurface, r->x + r->w, r->y + 1, (cMenu_leftMargin > 0)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_UP);

	for (i = cMenu_leftMargin; i < (cMenu_leftMargin + fileSelector->itemsOnPage); i++) {
		x = r->x + (((i - cMenu_leftMargin) >= halfpage) ? itemPixelWidth : 0);
		y = r->y + (((i - cMenu_leftMargin) % halfpage) * GUI_CONST_ITEM_SIZE);

		drawRectangle(defaultSurface, x - 2, y, itemPixelWidth, GUI_CONST_ITEM_SIZE,
			(i == cMenu_hilite) ? GUI_COLOR_HIGHLIGHT : GUI_COLOR_BACKGROUND);

		if (i >= cMenu_count)
			continue;

		strncpy(ptr, fileSelector->dirEntries[i], 31);
		ptr[31] = '\0';

		if (ptr[0] == DIR_DELIMITER) {
			c = GUI_COLOR_HOTKEY;
			printChar(defaultSurface, x, y + 2, c, SCHR_DIRECTORY);
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
			ptr[1] = SCHR_BROWSE;
			ptr[2] = '\0';
		}

		if (strlen(ptr) > (DWORD) itemCharWidth) {
			while (strlen(ptr) > (DWORD) (itemCharWidth - 1))
				ptr[strlen(ptr) - 1] = '\0';
			ptr[strlen(ptr)] = SCHR_BROWSE;
		}

		printText(defaultSurface, x + GUI_CONST_HOTKEYCHAR, y + 2, c, ptr + 1);
	}

	printChar(defaultSurface, r->x + r->w,
		r->y + ((halfpage - 1) * GUI_CONST_ITEM_SIZE) + 2, (i < cMenu_count)
			? GUI_COLOR_BORDER : GUI_COLOR_BACKGROUND, SCHR_SCROLL_DW);

	delete r;
	needRedraw = true;
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerFileSelector(WORD key)
{
	int i = cMenu_hilite, halfpage = fileSelector->itemsOnPage / 2,
			prevLeftMargin = 0, searchlen = strlen(fileSelector->search);
	char *ptr = fileSelector->dirEntries[cMenu_hilite], *lastItem;
	bool change = false;
	BYTE b = 0;

	switch (key) {
		case SDLK_F4 | KM_ALT:
			Emulator->ActionExit();
			menuCloseAll();
			needRelease = true;
			return;

		case SDLK_ESCAPE:
			if (searchlen > 0) {
				change = keyhandlerFileSelectorSearchClean();
				needRelease = true;
			}
			else {
				menuClose();
				needRelease = true;
				return;
			}
			break;

		case SDLK_c | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPSAVE) {
				prevLeftMargin = cMenu_leftMargin;
				Settings->Snapshot->saveCompressed = !Settings->Snapshot->saveCompressed;
				drawFileSelector(false);
				change = true;
			}
			break;

		case SDLK_d | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPLOAD) {
				prevLeftMargin = cMenu_leftMargin;
				Settings->Snapshot->dontRunOnLoad = !Settings->Snapshot->dontRunOnLoad;
				drawFileSelector(false);
				change = true;
			}
			break;

		case SDLK_r | KM_ALT:
			if (fileSelector->type == GUI_FS_SNAPSAVE) {
				prevLeftMargin = cMenu_leftMargin;
				Settings->Snapshot->saveWithMonitor = !Settings->Snapshot->saveWithMonitor;
				drawFileSelector(false);
				change = true;
			}
			break;

		case SDLK_h | KM_ALT:
			keyhandlerFileSelectorSearchClean();
			strcpy(fileSelector->path, PathUserHome);
			drawFileSelector();
			needRelease = true;
			break;

		case SDLK_PERIOD | KM_ALT:
			keyhandlerFileSelectorSearchClean();
			Settings->showHiddenFiles = !Settings->showHiddenFiles;
			drawFileSelector();
			needRelease = true;
			break;

		case SDLK_TAB:
			if (fileSelector->type == GUI_FS_BASESAVE
			 || fileSelector->type == GUI_FS_SNAPSAVE) {
				char buffer[40];
				buffer[0] = '\0';

				if (*ptr == '\xA0')
					strcpy(buffer, ptr + 1);

				if (editBox("ENTER FILENAME:", buffer, 32, false) && strlen(buffer) > 0) {
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
									keyhandlerFileSelectorCallback(buffer);
									return;
								}
							}
						}

						ptr = buffer + strlen(buffer);
						*ptr = '.';
						strcpy(ptr + 1, fileSelector->extFilter[0]);
					}

					keyhandlerFileSelectorCallback(buffer);
					return;
				}
			}
			break;

		case SDLK_SPACE:
			if (searchlen > 0) {
				i = keyhandlerFileSelectorSearch(i);
				if (i < 0)
					i = keyhandlerFileSelectorSearch();
				if (i < 1)
					i = cMenu_hilite;
				drawFileSelector();
				change = true;
			}
			break;

		case SDLK_BACKSPACE:
			if (searchlen > 1) {
				fileSelector->search[searchlen - 1] = '\0';
				i = keyhandlerFileSelectorSearch();
				if (i < 1)
					i = cMenu_hilite;
				drawFileSelector();
				change = true;
				break;
			}
			else if (searchlen == 1) {
				change = keyhandlerFileSelectorSearchClean();
				needRelease = true;
				break;
			}
			else
				// select ".." and continue in next case...
				ptr = fileSelector->dirEntries[0];

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			change = keyhandlerFileSelectorSearchClean();
			if (*ptr == '\xA0') {
				lastItem = ptr + 1;
				if ((ptr = strrchr(lastItem, '.'))) {
					for (b = 0; fileSelector->extFilter[b] != NULL; b++) {
						if (strcasecmp(ptr + 1, fileSelector->extFilter[b]) == 0) {
							keyhandlerFileSelectorCallback(lastItem);
							return;
						}
					}
				}
			}
			else if (TestDir(fileSelector->path, ptr + 1, &lastItem)) {
				cMenu_hilite = 0;
				drawFileSelector();
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

		case SDLK_LEFT:
		case SDLK_PAGEUP:
			change = keyhandlerFileSelectorSearchClean();
			if (i > 0) {
				i -= (key == SDLK_LEFT) ? halfpage : fileSelector->itemsOnPage;
				if (i < 0)
					i = 0;
				change = true;
			}
			break;

		case SDLK_RIGHT:
		case SDLK_PAGEDOWN:
			change = keyhandlerFileSelectorSearchClean();
			if (i < (cMenu_count - 1)) {
				i += (key == SDLK_RIGHT) ? halfpage : fileSelector->itemsOnPage;
				if (i >= cMenu_count)
					i = (cMenu_count - 1);
				change = true;
			}
			break;

		case SDLK_UP:
			change = keyhandlerFileSelectorSearchClean();
			if (i > 0) {
				i--;
				change = true;
			}
			break;

		case SDLK_DOWN:
			change = keyhandlerFileSelectorSearchClean();
			if (i < (cMenu_count - 1)) {
				i++;
				change = true;
			}
			break;

		case SDLK_HOME:
			keyhandlerFileSelectorSearchClean();
			i = 0;
			needRelease = true;
			change = true;
			break;

		case SDLK_END:
			keyhandlerFileSelectorSearchClean();
			i = (cMenu_count - 1);
			needRelease = true;
			change = true;
			break;

		default:
			b = (key & 127);
			if (searchlen < 21 &&
				((b > SDLK_SPACE && b <= SDLK_z) ||
				 (key >= SDLK_KP0 && key <= SDLK_KP_PLUS))) {

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
				else if (key >= SDLK_KP0 && key <= SDLK_KP9)
					b ^= 48;
				else if (key == SDLK_KP_PERIOD)
					b = '.';
				else if (key == SDLK_KP_PLUS)
					b = '+';
				else if (key == SDLK_KP_MINUS)
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
				i = keyhandlerFileSelectorSearch();
				if (i < 1) {
					fileSelector->search[--searchlen] = '\0';
					i = cMenu_hilite;
				}
				change = true;
				drawFileSelector();
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
		drawFileSelectorItems();
	}
}
//-----------------------------------------------------------------------------
void UserInterface::keyhandlerFileSelectorCallback(char *fileName)
{
	BYTE ret = fileSelector->tag;

	char *ptr = fileSelector->path + strlen(fileSelector->path);
	*ptr = DIR_DELIMITER;
	*(ptr + 1) = '\0';
	strcat(fileSelector->path, fileName);

	if ((fileSelector->type == GUI_FS_BASESAVE
	  || fileSelector->type == GUI_FS_SNAPSAVE)
	  && FileExists(fileSelector->path)) {

		if (queryDialog("OVERWRITE?", false) != GUI_QUERY_YES) {
			*ptr = '\0';
			return;
		}
	}

	fileSelector->callback(fileSelector->path, &ret);
	if (ret == 0) {
		menuCloseAll();
		needRelease = true;
		uiSetChanges = PS_CLOSEALL;
	}
	else if (ret == 1) {
		menuClose();
		needRelease = true;
	}
	else
		*ptr = '\0';
}
//-----------------------------------------------------------------------------
int UserInterface::keyhandlerFileSelectorSearch(int from)
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
bool UserInterface::keyhandlerFileSelectorSearchClean()
{
	if (fileSelector->search[0] != '\0') {
		fileSelector->search[0] = '\0';
		drawFileSelector();
		return true;
	}

	return false;
}
//-----------------------------------------------------------------------------
