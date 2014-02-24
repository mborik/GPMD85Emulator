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
#include "CommonUtils.h"
//-----------------------------------------------------------------------------
char CommonUtils::buffer[MAX_PATH] = "";
struct stat CommonUtils::filestat;
struct CommonUtils::TGlobalVideoInfo CommonUtils::globalVideoInfo;
//-----------------------------------------------------------------------------
char *CommonUtils::pathApplicationConfig = NULL;
char *CommonUtils::pathApplication = NULL;
char *CommonUtils::pathResources = NULL;
char *CommonUtils::pathUserHome = NULL;
//-----------------------------------------------------------------------------
const char *CommonUtils::commonAdaptFilePath(const char *filePath, char *path)
{
	if (filePath == NULL)
		return filePath;

	if (path == NULL)
		path = pathApplication;

	const char *tail = strstr(filePath, path);
	if (tail == NULL)
		return filePath;

	return tail + (strlen(path) + 1);
}
//-----------------------------------------------------------------------------
const char *CommonUtils::commonExtractFileName(const char *filePath)
{
	if (filePath == NULL)
		return filePath;

	const char *tail = strrchr(filePath, '/');
	if (tail == NULL)
		return filePath;

	return ++tail;
}
//-----------------------------------------------------------------------------
char *CommonUtils::commonComposeFilePath(const char *filePath)
{
	if (filePath == NULL)
		return NULL;

	char *buf = NULL;

	if (filePath[0] != '/') {
		if (filePath[0] == '.' && filePath[1] == '/')
			filePath += 2;

		buf = new char[strlen(PathApplication) + strlen(filePath) + 2];
		sprintf(buf, "%s%c%s", PathApplication, DIR_DELIMITER, filePath);
	}
	else {
		buf = new char[strlen(filePath) + 1];
		strcpy(buf, filePath);
	}

	return buf;
}
//-----------------------------------------------------------------------------
char *CommonUtils::commonLocateResource(const char *fileName, bool copyToHome)
{
	if (fileName == NULL)
		return NULL;

	sprintf(buffer, "%s%c%s", PathAppConfig, DIR_DELIMITER, fileName);
	if (stat(buffer, &filestat) != 0) {
		sprintf(buffer, "%s%c%s", PathResources, DIR_DELIMITER, fileName);
		if (stat(buffer, &filestat) != 0) {
			sprintf(buffer, "%s%c%s%c%s", PathApplication, DIR_DELIMITER, "res", DIR_DELIMITER, fileName);
			if (stat(buffer, &filestat) != 0)
				return NULL;
		}

		if (copyToHome) {
			FILE *in = fopen(buffer, "rb");
			if (in == NULL)
				return NULL;

			char *copybuf = new char[MAX_PATH];
			strcpy(copybuf, buffer);
			sprintf(buffer, "%s%c%s", PathAppConfig, DIR_DELIMITER, fileName);

			FILE *out = fopen(buffer, "wb");
			if (out == NULL) {
				fclose(in);
				return copybuf;
			}

			int i;
			while ((i = fread(copybuf, sizeof(char), MAX_PATH, in)))
				fwrite(copybuf, sizeof(char), i, out);

			fclose(out);
			fclose(in);
		}
	}

	return buffer;
}
//-----------------------------------------------------------------------------
char *CommonUtils::commonLocateRom(const char *fileName)
{
	if (fileName == NULL)
		return (char *) fileName;

	sprintf(buffer, "%s%c%s", PathResources, DIR_DELIMITER, fileName);
	if (stat(buffer, &filestat) != 0) {
		sprintf(buffer, "%s%c%s%c%s", PathApplication, DIR_DELIMITER, "rom", DIR_DELIMITER, fileName);
		if (stat(buffer, &filestat) != 0) {
			sprintf(buffer, "%s%c%s", PathAppConfig, DIR_DELIMITER, fileName);
			if (stat(buffer, &filestat) != 0)
				return NULL;
		}
	}

	return buffer;
}
//-----------------------------------------------------------------------------
int CommonUtils::commonReadFromFile(const char *fileName, int offset, int size, BYTE *dest)
{
	int dwR = -1;

	FILE *f = fopen(fileName, "rb");
	if (f) {
		if (fseek(f, offset, SEEK_SET) == 0)
			dwR = fread(dest, sizeof(BYTE), size, f);

		fclose(f);
	}

	return dwR;
}
//-----------------------------------------------------------------------------
int CommonUtils::commonWriteToFile(const char *fileName, int offset, int size, BYTE *src, bool createNew)
{
	int dwW = -1;

	FILE *f = fopen(fileName, createNew ? "wb" : "rb+");
	if (f) {
		if (fseek(f, offset, SEEK_SET) == 0)
			dwW = fwrite(src, sizeof(BYTE), size, f);

		fclose(f);
	}

	return dwW;
}
//---------------------------------------------------------------------------
bool CommonUtils::commonCreateMedium(const char *fileName, DWORD size, BYTE fill) {
	bool result = false;
	BYTE *buff;
	DWORD dwW, ii;

	FILE *f = fopen(fileName, "wb");
	if (f) {
		buff = new BYTE [CHUNK];
		memset(buff, fill, CHUNK);

		do {
			for (ii = 0; ii < size; ii += CHUNK) {
				dwW = fwrite(buff, sizeof(BYTE), CHUNK, f);
				if (dwW != CHUNK)
					break;
			}

			if (ii < size)
				break;

			result = true;

		} while (false);

		if (buff)
			delete buff;
		buff = NULL;

		fclose(f);
	}

	return result;
}
//-----------------------------------------------------------------------------
// Metoda sa pokusi spakovat blok o dlzke 'len' zo 'src' do 'dest'.
// 'src' a 'dest' musia byt osobitne alokovane bloky. Oba  musia mat velkost
// minimalne 'len'.
// Navratovou hodnotou je dlzka spakovaneho bloku v 'dest'.
// Ak je tato hodnota = 1, zdrojovy blok bol vyplneny jednou hodnotou, ktora je
// ulozena v '*dest'. Ak je vratena hodnota = 'len', blok sa nepodarilo spakovat
// a do 'dest' je skopirovany povodny blok zo 'src'. V ostatnych pripadoch
// (navratova hodnota < 'len') je blok spakovany jednoduchou RLE kompresiou.
// Format kompresovanych dat: Flag Data Flag Data Flag Data ...
// Flag - 0x00 az 0x7F => 1 byte ako Data pre 3 az 130 rovnakych bytov
// Flag - 0x80 az 0xFF => blok 1 az 128 nepakovatelnych bytov ako Data
//---------------------------------------------------------------------------
int CommonUtils::commonPackBlock(BYTE *dest, BYTE *src, int len)
{
	BYTE *psrc = src;
	BYTE *maxsrc = src + len;
	BYTE *pdest = dest;
	BYTE *maxdest = dest + len - 1;
	BYTE x, cnt = 0;

	// zistenie, ci je blok vyplneny iba jednou hodnotou
	x = *src;
	while (++psrc < maxsrc) {
		if (x != *psrc)
		break;
	}

	if (psrc == maxsrc) {
		*dest = x; // blok je vyplneny iba jednou hodnotou
		return 1;
	}

	// kompresia bloku
	psrc = src;
	while (psrc < maxsrc && pdest < maxdest) {
		x = *psrc++;
		if (maxsrc - psrc >= 2 && x == *psrc && x == *(psrc + 1)) {
			// nasli sa rovnake 3 byty za sebou
			if (cnt > 0) {
				// ak tomu predchadzal nepakovatelny blok, zaznamenaj to
				*(pdest - cnt) = (BYTE)((cnt - 1) | 0x80); // pocet - flag
				pdest++;
				cnt = 0;
			}

			// najdenie dalsich opakujucich sa bytov
			if (pdest < maxdest) {
				psrc += 2;

				// max 130 (= 3 + 127)
				while (psrc < maxsrc && x == *psrc && cnt < 127) {
					psrc++;
					cnt++;
				}

				*pdest++ = cnt; // pocet - flag
				*pdest++ = x; // byte
				cnt = 0;
			}
		}
		else {
			*(++pdest) = x; // nepakovatelny byte
			if (++cnt == 0x80) { // max 128
				// pocet 128 - flag ((128 - 1) or 0x80)
				*(pdest - cnt) = 0xFF;
				pdest++;
				cnt = 0;
			}
		}
	}

	if (cnt > 0 && pdest < maxdest + 1) {
		*(pdest - cnt) = (BYTE)((cnt - 1) | 0x80);
		pdest++;
	}

	// dlzka spakovaneho bloku
	int packedLen = pdest - dest;
	if (packedLen < len && psrc == maxsrc)
		return packedLen; // kompresia uspesna

	// neuspesna kompresia - do dest sa ulozi povodny blok
	memcpy(dest, src, len);
	return len;
}
//---------------------------------------------------------------------------
// Metoda sa pokusi rozpakovat blok o dlzke 'srclen' zo 'src' do 'dest'.
// 'src' a 'dest' musia byt osobitne alokovane bloky.
// 'destlen' je dlzka cieloveho 'dest' buffra. V pripade, ze 'srclen' = 1
// alebo 'srclen' = 'destlen', 'destlen' je zaroven pozadovana dlzka cielovych
// dat. Ak je 'srclen' = 1, tak sa bytom na '*src' vyplni 'dest' o dlzke
// 'destlen'. Ak je 'srclen' = 'destlen', jedna sa o nespakovany blok a prevedie
// sa iba prekopirovanie zo 'src' do 'dest'.
// Navratovou hodnotou je pocet bytov ulozenych v 'dest'.
// Ak je vratena hodnota -1, su chybne vstupne parametre.
// Ak je vratena hodnota -2, doslo ku chybe pri rozpakovani, teda 'src' data
// nezodpovedaju pakovanemu bloku, ktory sa po rozpakovani "vojde" do 'dest'.
int CommonUtils::commonUnpackBlock(BYTE *dest, int destlen, BYTE *src, int srclen)
{
	BYTE *origDest = dest;
	BYTE *maxsrc = src + srclen;
	BYTE *maxdest = dest + destlen;
	BYTE x, y;

	if (srclen == 0 || destlen == 0 || destlen < srclen)
	return -1;

	if (srclen == 1) {
		memset(dest, *src, destlen);
		return destlen;
	}

	if (srclen == destlen) {
		memcpy(dest, src, destlen);
		return destlen;
	}

	while (src < maxsrc && dest < maxdest) {
		x = *src++;

		// nepakovane data 1 az 128 => 0x80 az 0xFF
		if (x & 0x80) {
			x &= 0x7F;
			x++;
			while (x-- > 0 && dest < maxdest)
			*dest++ = *src++;
		}
		// pakovane data 3 az 130 => 0x00 az 0x7F
		else {
			x += (BYTE)3;
			y = *src++;
			while (x-- > 0 && dest < maxdest)
			*dest++ = y;
		}
	}

	if (maxsrc - src > 0)
	return -2;

	return dest - origDest;
}
//---------------------------------------------------------------------------
bool CommonUtils::commonTestDir(const char *directory, char *add, char **tail)
{
	static DWORD uid = getuid();
	static DWORD gid = getgid();

	if (tail)
		*tail = NULL;

	if (strcmp(add, "..") == 0) {
		strcpy(buffer, directory);
		char *wrk = strrchr(buffer, DIR_DELIMITER);
		if (wrk && (wrk - buffer) > 0)
			*wrk = '\0';
		else if (wrk == buffer) {
			wrk = strcpy(wrk + strlen(buffer) + 1, buffer);
			buffer[1] = '\0';
		}

		if (tail && wrk)
			*tail = (wrk + 1);
	}
	else if (strcmp(directory, "/") == 0)
		sprintf(buffer, "%s%s", directory, add);
	else
		sprintf(buffer, "%s%c%s", directory, DIR_DELIMITER, add);

	if (stat(buffer, &filestat) == 0) {
		if (S_ISDIR(filestat.st_mode) &&
		   ((filestat.st_uid == uid && (filestat.st_mode & S_IRUSR)) ||
		    (filestat.st_gid == gid && (filestat.st_mode & S_IRGRP)) ||
		    (filestat.st_mode & S_IROTH))) {

			strcpy((char *) directory, buffer);
			return true;
		}
	}

	if (!tail)
		strcpy((char *) directory, buffer);

	return false;
}
//-----------------------------------------------------------------------------
void CommonUtils::commonScanDir(const char *dir, char ***filenames, int *numfiles, bool showHiddenFiles)
{
	struct dirent *dirent;
	DIR *directory = NULL;
	char filemode;
	char **newpt;
	struct stat statdata;

	if (*filenames) {
		newpt = *filenames;
		for (int i = 0; i < *numfiles; i++)
			free(newpt[i]);

		free(newpt);
		*filenames = newpt = NULL;
	}

	*numfiles = -1;

	if (dir)
		directory = opendir(dir);
	if (!directory)
		return;

	int allocator = 8;
	newpt = (char **) malloc(allocator * sizeof(char *));
	while (true) {
		dirent = readdir(directory);
		if (dirent) {
#ifdef HAVE_STRUCT_DIRENT_D_TYPE
			if (dirent->d_type == DT_REG)
				filemode = '\xA0'; // fake for qsort: directories on top
			else if (dirent->d_type == DT_DIR)
				filemode = DIR_DELIMITER;
			else if (dirent->d_type == DT_UNKNOWN || dirent->d_type == DT_LNK) {
#endif
				// fallback if the d_type is supported neither by the system
				// nor the filesystem, or d_type is symbolic link:
				if (dir)
					sprintf(buffer, "%s%c%s", dir, DIR_DELIMITER, dirent->d_name);
				else
					sprintf(buffer, "%s", dirent->d_name);

				if (stat(buffer, &statdata) < 0)
					continue;

				if (S_ISREG(statdata.st_mode))
					filemode = '\xA0'; // fake for qsort: directories on top
				else if (S_ISDIR(statdata.st_mode))
					filemode = DIR_DELIMITER;
				else
					continue;
#ifdef HAVE_STRUCT_DIRENT_D_TYPE
			}
			else
				continue;
#endif

			// exclude "dot dir" and hidden files...
			if (dirent->d_name[0] == '.') {
				if (dirent->d_name[1] == '.')
					filemode = ' ';
				else if ((dirent->d_name[1] == '\0') || (showHiddenFiles == false))
					continue;
			}

			if (++*numfiles >= allocator) {
				allocator <<= 1;
				newpt = (char **) realloc(*filenames, allocator * sizeof(char *));
			}

			if (newpt) {
				newpt[*numfiles] = (char *) malloc(sizeof(char) * (strlen(dirent->d_name) + 2));
				if (newpt[*numfiles]) {
					sprintf(newpt[*numfiles], "%c%s", filemode, dirent->d_name);
					*filenames = newpt;
					continue;
				}
				else
					newpt = NULL;
			}

			if (newpt == NULL) {
				warning(NULL, "ScanDir: memory allocation error");

				newpt = *filenames;
				for (--*numfiles; *numfiles >= 0; --*numfiles)
					free(newpt[*numfiles]);

				free(newpt);
				*filenames = newpt = NULL;

				closedir(directory);
				return;
			}
		}
		else
			break;
	}

	closedir(directory);
	qsort(*filenames, ++*numfiles, sizeof(char *), CommonUtils::qsortstrcmp);
}
//-----------------------------------------------------------------------------
