/*	CommonUtils.cpp: Class with common static methods and properties.
	Copyright (c) 2011-2019 Martin Borik <mborik@users.sourceforge.net>

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
#ifndef COMMONUTILS_H_
#define COMMONUTILS_H_
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
extern struct TGraphicsDeviceContext {
	SDL_Window *window;
	SDL_Renderer *renderer;
	DWORD windowID;
	DWORD format;
	int freq;
	int w;
	int h;
} gdc;

extern struct stat filestat;

extern char *PathAppConfig;
extern char *PathApplication;
extern char *PathResources;
extern char *PathUserHome;

const char *AdaptFilePath(const char *filePath, char *path = NULL);
const char *ExtractFileName(const char *filePath);
char *ComposeFilePath(const char *filePath);
char *LocateResource(const char *fileName, bool copyToHome);
char *LocateROM(const char *fileName);
int ReadFromFile(const char *fileName, int offset, int size, BYTE *dest);
int WriteToFile(const char *fileName, int offset, int size, BYTE *src, bool createNew);
bool CreateMedium(const char *fileName, DWORD size, BYTE fill);
int PackBlock(BYTE *dest, BYTE *src, int len);
int UnpackBlock(BYTE *dest, int destlen, BYTE *src, int srclen);
bool TestDir(const char *directory, char *add, char **tail);
void ScanDir(const char *directory, char ***filenames, int *numfiles, bool showHiddenFiles = false);

inline bool FileExists(const char *path)
	{ return (stat(path, &filestat) == 0 && S_ISREG(filestat.st_mode)); }
inline bool DirExists(const char *path)
	{ return (stat(path, &filestat) == 0 && S_ISDIR(filestat.st_mode)); }
//-----------------------------------------------------------------------------
#endif
