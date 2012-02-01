/*	CommonUtils.cpp: Class with common static methods and properties.
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
#ifndef COMMONUTILS_H_
#define COMMONUTILS_H_
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
class CommonUtils {
public:
	static char buffer[MAX_PATH];
	static struct stat filestat;

	static struct TGlobalVideoInfo {
		unsigned hw:1;
		unsigned wm:1;
		unsigned depth:6;
		unsigned w:16;
		unsigned h:16;
	} globalVideoInfo;

	static char *pathApplicationConfig;
	static char *pathApplication;
	static char *pathResources;
	static char *pathUserHome;

	static const char *commonAdaptFilePath(const char *filePath, char *path = NULL);
	static const char *commonExtractFileName(const char *filePath);
	static char *commonComposeFilePath(const char *filePath);
	static char *commonLocateResource(const char *fileName, bool copyToHome);
	static char *commonLocateRom(const char *fileName);
	static int commonReadFromFile(const char *fileName, int offset, int size, BYTE *dest);
	static int commonWriteToFile(const char *fileName, int offset, int size, BYTE *src, bool createNew);
	static bool commonCreateMedium(const char *fileName, DWORD size, BYTE fill);
	static int commonPackBlock(BYTE *dest, BYTE *src, int len);
	static int commonUnpackBlock(BYTE *dest, int destlen, BYTE *src, int srclen);
	static bool commonTestDir(const char *directory, char *add, char **tail);
	static void commonScanDir(const char *directory, char ***filenames, int *numfiles, bool showHiddenFiles = false);

	static inline bool commonFileExists(const char *path)
		{ return (stat(path, &filestat) == 0 && S_ISREG(filestat.st_mode)); }
	static inline bool commonDirExists(const char *path)
		{ return (stat(path, &filestat) == 0 && S_ISDIR(filestat.st_mode)); }

	static inline int qsortstrcmp(const void *p1, const void *p2)
		{ return strcmp(*(const char **) p1, *(const char **) p2); }
};
//-----------------------------------------------------------------------------
#define PathAppConfig CommonUtils::pathApplicationConfig
#define PathApplication CommonUtils::pathApplication
#define PathResources CommonUtils::pathResources
#define PathUserHome CommonUtils::pathUserHome

#define AdaptFilePath CommonUtils::commonAdaptFilePath
#define ExtractFileName CommonUtils::commonExtractFileName
#define ComposeFilePath CommonUtils::commonComposeFilePath
#define LocateResource CommonUtils::commonLocateResource
#define LocateROM CommonUtils::commonLocateRom
#define ReadFromFile CommonUtils::commonReadFromFile
#define WriteToFile CommonUtils::commonWriteToFile
#define CreateMedium CommonUtils::commonCreateMedium
#define FileExists CommonUtils::commonFileExists
#define DirExists CommonUtils::commonFileExists
#define PackBlock CommonUtils::commonPackBlock
#define UnpackBlock CommonUtils::commonUnpackBlock
#define TestDir CommonUtils::commonTestDir
#define ScanDir CommonUtils::commonScanDir

#define gvi CommonUtils::globalVideoInfo
//-----------------------------------------------------------------------------
#endif
