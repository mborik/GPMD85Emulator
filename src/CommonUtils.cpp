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
#include "CommonUtils.h"
#include <cctype>
#include <random>
//-----------------------------------------------------------------------------
static char buffer[MAX_PATH] = "";
struct TGraphicsDeviceContext gdc;
//-----------------------------------------------------------------------------
char *PathAppConfig = NULL;
char *PathGuiConfig = NULL;
char *PathApplication = NULL;
char *PathResources = NULL;
char *PathUserHome = NULL;
//-----------------------------------------------------------------------------
/*
 * Checks whether given path (file or directory) exists.
 *
 * @param path path to be tested
 * @return true if path exists, false otherwise (or on any error)
 */
static inline bool PathExists(const char *path)
{
	std::error_code ec;
	return std::filesystem::exists(path, ec);
}
//-----------------------------------------------------------------------------
/*
 * Joins up to three path components by the platform specific separator
 * and stores the result to the internal static buffer.
 *
 * @param a first component
 * @param b second component
 * @param c third component, optional
 * @return pointer to the internal static buffer
 */
static char *JoinToBuffer(const char *a, const char *b, const char *c = NULL)
{
	std::filesystem::path path = std::filesystem::path(a) / b;
	if (c != NULL)
		path /= c;

	const std::string result = path.string();
	strncpy(buffer, result.c_str(), MAX_PATH - 1);
	buffer[MAX_PATH - 1] = '\0';
	return buffer;
}
//-----------------------------------------------------------------------------
/*
 * Strips the leading `path` (default is the application path) from `filePath`
 * to get a relative path. No allocation is made, result points into `filePath`.
 *
 * @param filePath full file path
 * @param path prefix to be stripped, NULL means PathApplication
 * @return relative path, or unchanged `filePath` if it doesn't contain the prefix
 */
const char *AdaptFilePath(const char *filePath, const char *path)
{
	if (filePath == NULL)
		return filePath;

	if (path == NULL)
		path = PathApplication;

	const char *tail = strstr(filePath, path);
	if (tail == NULL)
		return filePath;

	return tail + (strlen(path) + 1);
}
//-----------------------------------------------------------------------------
/*
 * Extracts the file name from a path (the last path component).
 * No allocation is made, result points into `filePath`.
 *
 * @param filePath file path
 * @return file name part of the path, or NULL if `filePath` is NULL
 */
const char *ExtractFileName(const char *filePath)
{
	if (filePath == NULL)
		return filePath;

	const size_t nameLen = std::filesystem::path(filePath).filename().native().length();
	return filePath + (strlen(filePath) - nameLen);
}
//-----------------------------------------------------------------------------
/*
 * Composes an absolute file path. Relative paths are prefixed with the
 * application path, absolute paths are just duplicated.
 *
 * @param filePath absolute or relative file path
 * @return newly allocated string (release it by `delete[]`), or NULL
 */
char *ComposeFilePath(const char *filePath)
{
	if (filePath == NULL)
		return NULL;

	std::filesystem::path path(filePath);
	if (!path.is_absolute()) {
		if (filePath[0] == '.' && (filePath[1] == '/' || filePath[1] == '\\'))
			filePath += 2;

		path = std::filesystem::path(PathApplication) / filePath;
	}

	const std::string result = path.string();
	char *buf = new char[result.length() + 1];
	strcpy(buf, result.c_str());
	return buf;
}
//-----------------------------------------------------------------------------
/*
 * Returns current working directory.
 *
 * @return newly allocated string (release it by `delete[]`), or NULL on failure
 */
char *GetCurrentDir()
{
	std::error_code ec;
	std::string cwd = std::filesystem::current_path(ec).string();
	if (ec)
		return NULL;

	char *result = new char[cwd.length() + 1];
	strcpy(result, cwd.c_str());
	return result;
}
//-----------------------------------------------------------------------------
/*
 * Creates the directory (including missing parents) if it doesn't exist yet.
 *
 * @param path directory path
 * @return true if the directory exists or was created, false otherwise
 */
bool CreateDirIfNotExist(const char *path)
{
	if (path == NULL || *path == '\0')
		return false;

	std::error_code ec;
	if (std::filesystem::is_directory(path, ec))
		return true;

	ec.clear();
	std::filesystem::create_directories(path, ec);
	return !ec;
}
//-----------------------------------------------------------------------------
/*
 * Composes a unique, not yet existing temporary file name ending with ".tmp".
 * If `basePath` is NULL, system temporary directory with application name
 * as prefix is used, otherwise name is `basePath` with unique suffix appended.
 *
 * @param basePath optional prefix of the temporary file name
 * @return newly allocated string (release it by `delete[]`), or NULL on failure
 */
char *CreateTempFileName(const char *basePath)
{
	std::string base;
	if (basePath != NULL)
		base = basePath;
	else {
		std::error_code ec;
		std::filesystem::path dir = std::filesystem::temp_directory_path(ec);
		if (ec)
			return NULL;

		base = (dir / PACKAGE_TARNAME).string();
	}

	std::random_device rd;
	for (int attempt = 0; attempt < 100; attempt++) {
		char suffix[32];
		snprintf(suffix, sizeof(suffix), ".%08x%08x.tmp", (unsigned) rd(), (unsigned) SDL_GetTicks());

		std::string name = base + suffix;
		std::error_code ec;
		if (!std::filesystem::exists(name, ec)) {
			char *result = new char[name.length() + 1];
			strcpy(result, name.c_str());
			return result;
		}
	}

	return NULL;
}
//-----------------------------------------------------------------------------
/*
 * Removes a file.
 *
 * @param path file path
 * @return true if the file was removed, false otherwise
 */
bool RemoveFile(const char *path)
{
	if (path == NULL)
		return false;

	std::error_code ec;
	return std::filesystem::remove(path, ec);
}
//-----------------------------------------------------------------------------
/*
 * Case-insensitive (ASCII) comparison of two strings, optionally limited to
 * `maxLen` characters. Replacement of POSIX strcasecmp() and strncasecmp().
 *
 * @param a first string
 * @param b second string
 * @param maxLen maximum number of compared characters, 0 means unlimited
 * @return 0 if strings are equal, negative or positive value otherwise
 */
int CompareNoCase(const char *a, const char *b, size_t maxLen)
{
	for (size_t i = 0; maxLen == 0 || i < maxLen; i++) {
		int ca = std::tolower((unsigned char) a[i]);
		int cb = std::tolower((unsigned char) b[i]);
		if (ca != cb)
			return ca - cb;
		if (ca == 0)
			break;
	}

	return 0;
}
//-----------------------------------------------------------------------------
/*
 * Locates a resource file in the user config directory (optional), in the
 * resources directory or in the `res` subdirectory of the application path.
 * Optionally copies the found resource to the user config directory.
 * Returned pointer refers to an internal static buffer.
 *
 * @param fileName resource file name
 * @param locateAtHome look into the user config directory first
 * @param copyToHome copy a found resource to the user config directory
 * @return full path of the resource, or NULL if not found
 */
char *LocateResource(const char *fileName, bool locateAtHome, bool copyToHome)
{
	if (fileName == NULL)
		return NULL;

	if (locateAtHome)
		JoinToBuffer(PathAppConfig, fileName);
	if (!locateAtHome || !PathExists(buffer)) {
		JoinToBuffer(PathResources, fileName);
		if (!PathExists(buffer)) {
			JoinToBuffer(PathApplication, "res", fileName);
			if (!PathExists(buffer))
				return NULL;
		}

		if (copyToHome) {
			const std::string source = buffer;
			JoinToBuffer(PathAppConfig, fileName);

			std::error_code ec;
			std::filesystem::copy_file(source, buffer,
				std::filesystem::copy_options::overwrite_existing, ec);
			if (ec)
				strncpy(buffer, source.c_str(), MAX_PATH - 1);
		}
	}

	return buffer;
}
//-----------------------------------------------------------------------------
/*
 * Locates a ROM file in the resources directory, in the `rom` subdirectory
 * of the application path or in the user config directory.
 * Returned pointer refers to an internal static buffer.
 *
 * @param fileName ROM file name
 * @return full path of the ROM file, or NULL if not found
 */
char *LocateROM(const char *fileName)
{
	if (fileName == NULL)
		return (char *) fileName;

	JoinToBuffer(PathResources, fileName);
	if (!PathExists(buffer)) {
		JoinToBuffer(PathApplication, "rom", fileName);
		if (!PathExists(buffer)) {
			JoinToBuffer(PathAppConfig, fileName);
			if (!PathExists(buffer))
				return NULL;
		}
	}

	return buffer;
}
//-----------------------------------------------------------------------------
/*
 * Gets the size of a file.
 *
 * @param fileName file path
 * @return size in bytes, or -1 on error
 */
long FileSize(const char *fileName)
{
	long size = -1;

	FILE *f = fopen(fileName, "rb");
	if (f) {
		if (fseek(f, 0, SEEK_END) == 0)
			size = ftell(f);
		fclose(f);
	}

	return size;
}
//-----------------------------------------------------------------------------
/*
 * Reads a block of data from a file.
 *
 * @param fileName file path
 * @param offset position in the file where reading starts
 * @param size number of bytes to read
 * @param dest destination buffer of at least `size` bytes
 * @return number of bytes actually read, or -1 on error
 */
int ReadFromFile(const char *fileName, int offset, int size, BYTE *dest)
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
/*
 * Writes a block of data to a file.
 *
 * @param fileName file path
 * @param offset position in the file where writing starts
 * @param size number of bytes to write
 * @param src source buffer
 * @param createNew true to create/truncate the file, false to update existing one
 * @return number of bytes actually written, or -1 on error
 */
int WriteToFile(const char *fileName, int offset, int size, BYTE *src, bool createNew)
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
//-----------------------------------------------------------------------------
/*
 * Creates a new file of given size filled with a single byte value.
 *
 * @param fileName file path
 * @param size size of the file in bytes
 * @param fill byte value used to fill the file
 * @return true on success, false otherwise
 */
bool CreateMedium(const char *fileName, DWORD size, BYTE fill)
{
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
			delete[] buff;
		buff = NULL;

		fclose(f);
	}

	return result;
}
//-----------------------------------------------------------------------------
/*
 * Method to pack a block from `src` of given length to `dest` in RLE format.
 * Return value 1 means that all bytes are just single value. If return value
 * equals to `len`, packing was unsuccessful and `src` was copied do `dest`.
 * Otherwise returns a new packed block length less than given block length.
 */
int PackBlock(BYTE *dest, BYTE *src, int len)
{
	BYTE *psrc = src;
	BYTE *maxsrc = src + len;
	BYTE *pdest = dest;
	BYTE *maxdest = dest + len - 1;
	BYTE x, cnt = 0;

	// test block filled with single value
	x = *src;
	while (++psrc < maxsrc) {
		if (x != *psrc)
			break;
	}

	if (psrc == maxsrc) {
		*dest = x;
		return 1;
	}

	// block compression
	psrc = src;
	while (psrc < maxsrc && pdest < maxdest) {
		x = *psrc++;
		if (maxsrc - psrc >= 2 && x == *psrc && x == *(psrc + 1)) {
			// 3 same bytes sequence found
			if (cnt > 0) {
				*(pdest - cnt) = (BYTE)((cnt - 1) | 0x80); // count - flag
				pdest++;
				cnt = 0;
			}

			// find another repeating sequence
			if (pdest < maxdest) {
				psrc += 2;

				// max 130 (= 3 + 127)
				while (psrc < maxsrc && x == *psrc && cnt < 127) {
					psrc++;
					cnt++;
				}

				*pdest++ = cnt; // count - flag
				*pdest++ = x; // byte
				cnt = 0;
			}
		}
		else {
			// uncompressed sequence
			*(++pdest) = x;
			if (++cnt == 0x80) { // max 128
				// count 128 - flag ((128 - 1) or 0x80)
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

	// packed block length
	int packedLen = pdest - dest;
	if (packedLen < len && psrc == maxsrc)
		return packedLen; // success

	// unsuccessful compression - copy src to dest
	memcpy(dest, src, len);
	return len;
}
//-----------------------------------------------------------------------------
/*
 * Unpack block of RLE packed data or single repeating byte or unpacked data
 * from `src` to pre-allocated `dest` of given `scrlen`.
 */
int UnpackBlock(BYTE *dest, int destlen, BYTE *src, int srclen)
{
	BYTE *origDest = dest;
	BYTE *maxsrc = src + srclen;
	BYTE *maxdest = dest + destlen;
	BYTE x, y;

	if (srclen == 0 || destlen == 0 || destlen < srclen)
		return -1;

	// single repeating byte
	if (srclen == 1) {
		memset(dest, *src, destlen);
		return destlen;
	}

	// unpacked data
	if (srclen == destlen) {
		memcpy(dest, src, destlen);
		return destlen;
	}

	while (src < maxsrc && dest < maxdest) {
		x = *src++;

		// unpacked sequence 1 to 128 => 0x80 ~ 0xFF
		if (x & 0x80) {
			x &= 0x7F;
			x++;
			while (x-- > 0 && dest < maxdest)
				*dest++ = *src++;
		}
		// packed data sequence 3 to 130 => 0x00 ~ 0x7F
		else {
			x += (BYTE) 3;
			y = *src++;
			while (x-- > 0 && dest < maxdest)
				*dest++ = y;
		}
	}

	if (maxsrc - src > 0)
		return -2;

	return dest - origDest;
}
//-----------------------------------------------------------------------------
