/*	CommonUtils.cpp: Class with common static methods and properties.
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
#ifndef COMMONUTILS_H_
#define COMMONUTILS_H_
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
extern struct TGraphicsDeviceContext {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_GLContext context;
	DWORD windowID;
	DWORD format;
	int freq;
	int w;
	int h;
} gdc;

extern struct stat filestat;

extern char *PathAppConfig;
extern char *PathGuiConfig;
extern char *PathApplication;
extern char *PathResources;
extern char *PathUserHome;

const char *AdaptFilePath(const char *filePath, char *path = NULL);
const char *ExtractFileName(const char *filePath);
char *ComposeFilePath(const char *filePath);
char *LocateResource(const char *fileName, bool locateAtHome = true, bool copyToHome = false);
char *LocateROM(const char *fileName);
long int FileSize(const char *fileName);
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
