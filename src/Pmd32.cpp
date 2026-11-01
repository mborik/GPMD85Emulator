/*	Pmd32.cpp: Class for emulation of disk drive PMD 32
	Copyright (c) 2008-2026 Roman Borik <pmd85emu@gmail.com>
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
//---------------------------------------------------------------------------
#include "Pmd32.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
//---------------------------------------------------------------------------
Pmd32::Pmd32(IifGPIO *pio)
{
	this->pio = pio;
	pio->OnBeforeResetA.connect(&Pmd32::BeforeReset, this);
	pio->OnAfterResetA.connect(&Pmd32::AfterReset, this);

	for (int ii = 0; ii < NUM_DRIVES; ii++) {
		drives[ii].handle = NULL;
		drives[ii].wp = true;
	}

	extraCommands = false;

	drvnum = 0;
	diskIcon = 0;
	diskTicks = 0;
	diskState = IDLE_STATE;
	tickCounter = CPU_FREQ;
	inHandshake = false;

	memset(memory, 0, sizeof(memory));
}
//---------------------------------------------------------------------------
Pmd32::~Pmd32()
{
	RemoveDisk(DRIVE_A);
	RemoveDisk(DRIVE_B);
	RemoveDisk(DRIVE_C);
	RemoveDisk(DRIVE_D);
}
//---------------------------------------------------------------------------
void Pmd32::SetExtraCommands(bool extraCommands, const char *sdRoot)
{
	if (sdRoot == NULL)
		return;

	this->extraCommands = extraCommands;
	this->sdRoot = ToPath(sdRoot);
	this->sdPath.clear();
}
//---------------------------------------------------------------------------
void Pmd32::BeforeReset()
{
	pio->OnCpuWriteCWR.disconnect(this);
	pio->OnCpuWriteCH.disconnect(this);
}
//---------------------------------------------------------------------------
void Pmd32::AfterReset()
{
	pio->OnCpuWriteCWR.connect(&Pmd32::OnSetMode2, this);
	pio->OnCpuWriteCH.connect(&Pmd32::OnHandshake, this);

	diskTicks = 0;
	diskState = IDLE_STATE;
	tickCounter = CPU_FREQ;
	inHandshake = false;
	diskIcon = 0;
}
//---------------------------------------------------------------------------
void Pmd32::OnSetMode2(BYTE CWR)
{
	if ((CWR & GA_MODE) == GA_MODE2) {
		pio->ChangeBit(PP_PortC, _STBA, true);
		pio->ChangeBit(PP_PortC, _ACKA, true);
		diskState = WAIT_PRESENT;
		tickCounter = PRESENT_TICKS;
	}
	else {
		diskState = IDLE_STATE;
		tickCounter = CPU_FREQ;
	}
}
//---------------------------------------------------------------------------
void Pmd32::OnHandshake()
{
	BYTE val;

	inHandshake = true;
	if (pio->ReadBit(PP_PortC, _OBFA) == false) {
		val = pio->ReadByte(PP_PortA);
		pio->ChangeBit(PP_PortC, _ACKA, false);
		pio->ChangeBit(PP_PortC, _ACKA, true);

		switch (diskState) {
			case WAIT_PRESENT:
				if (val == PRESENTATION)
					diskState = WAIT_COMMAND;
				break;

			case WAIT_COMMAND:
				command = val;
				switch (val) {
					case 'B': // BOOT
					case '*': // FAST mode
					case '@': // SLOW mode
						diskState = WAIT_CRC;
						break;

					case 'Q': // read logical sector
					case 'R': // read logical sector
					case 'T': // write logical sector
					case 'W': // write logical sector
					case 'S': // write physical sector
					case 'F': // format track
						diskState = WAIT_SECTOR;
						break;

					case 'I': // drive select + HOME
						diskState = WAIT_DRIVE;
						break;

					case 'U': // write to PMD 32 memory
					case 'C': // read from PMD 32 memory
					case 'J': // execute code in PMD 32 memory
						diskState = WAIT_ADDR_H;
						break;

					case 'G': // get inserted disk image name
					case 'H': // insert disk image
					case 'P': // get disk parameters
						diskState = (extraCommands) ? WAIT_DRIVE : WAIT_PRESENT;
						break;

					case 'K': // get actual SD card path
						diskState = (extraCommands) ? WAIT_CRC : WAIT_PRESENT;
						break;

					case 'L': // get catalog of actual SD card path
						diskState = (extraCommands) ? WAIT_FIND_TYPE : WAIT_PRESENT;
						break;

					case 'M': // change actual SD card path
						diskState = (extraCommands) ? WAIT_LEN_NAME : WAIT_PRESENT;
						break;

					case 'N': // create new disk image in actual SD card path
						diskState = (extraCommands) ? WAIT_LEN_NAME : WAIT_PRESENT;
						break;

					case PRESENTATION:
						break;

					default:
						diskState = WAIT_PRESENT;
						break;
				}
				CRC = command;
				break;

			case WAIT_SECTOR:
				sector = val;
				drvnum = (BYTE) (sector >> 6);
				if (drvnum > 0 && drvnum < 3)
					drvnum ^= 3;
				sector &= 0x3F;
				CRC ^= val;
				diskIcon = 1 + (drvnum & (NUM_DRIVES - 1));
				if (command == 'T' || command == 'W' || command == 'S' || command == 'F')
					diskIcon += NUM_DRIVES;
				diskState = WAIT_TRACK;
				break;

			case WAIT_TRACK:
				track = val;
				CRC ^= val;
				if (command == 'T' || command == 'W' || command == 'S') {
					point = buffer;
					diskState = WAIT_DATA;
					if (command == 'T' || command == 'W')
						byteCounter = SECTOR_SIZE;
					else
						byteCounter = PHYSICAL_SECTOR_SIZE + 1;
				}
				else
					diskState = WAIT_CRC;
				break;

			case WAIT_DRIVE:
				drvnum = val;
				CRC ^= val;
				if (command == 'H')
					diskState = WAIT_WP;
				else
					diskState = WAIT_CRC;
				break;

			case WAIT_DATA:
				*point++ = val;
				CRC ^= val;
				if (--byteCounter == 0)
					diskState = WAIT_CRC;
				break;

			case WAIT_ADDR_H:
				address = val << 8;
				CRC ^= val;
				diskState = WAIT_ADDR_L;
				break;

			case WAIT_ADDR_L:
				address |= val;
				address &= 0x03FFF;
				CRC ^= val;
				if (command == 'J') {
					debug("PMD32", "%c: %04X", command, address);
					diskState = WAIT_CRC;
				}
				else
					diskState = WAIT_LEN_H;
				break;

			case WAIT_LEN_H:
				length = val << 8;
				CRC ^= val;
				diskState = WAIT_LEN_L;
				break;

			case WAIT_LEN_L:
				length |= val;
				CRC ^= val;
				if (command == 'C' || command == 'U')
					debug("PMD32", "%c: %04X,%04X", command, address, length);

				if (command == 'C')
					diskState = WAIT_CRC;
				else {
					byteCounter = length;
					diskState = WAIT_DATA_MEM;
				}
				break;

			case WAIT_DATA_MEM:
				if (address >= 0x1800 && address <= 0x1FFF)
					memory[(address - 0x1800) & 0x03FF] = val;
				address++;
				address &= 0x3FFF;
				CRC ^= val;
				if (--byteCounter == 0)
					diskState = WAIT_CRC;
				break;

			case WAIT_FIND_TYPE:
				findType = val;
				CRC ^= val;
				diskState = WAIT_CRC;
				break;

			case WAIT_LEN_NAME:
				CRC ^= val;
				if (val == 0 || val == 0xFF)
					diskState = WAIT_CRC;
				else
					diskState = WAIT_DATA_NAME;
				point = buffer;
				*point++ = val;
				byteCounter = (int) val;
				break;

			case WAIT_DATA_NAME:
				*point++ = val;
				CRC ^= val;
				if (--byteCounter == 0) {
					*point = 0;
					diskState = WAIT_CRC;
				}
				break;

			case WAIT_WP:
				wp = val;
				CRC ^= val;
				diskState = WAIT_LEN_NAME;
				break;

			case WAIT_CRC:
				if (val == CRC)
					diskState = SEND_ACK;
				else
					diskState = SEND_NAK;
				break;

		}
	}

	inHandshake = false;
}
//---------------------------------------------------------------------------
void Pmd32::Disk32Service(int ticks, int dur)
{
	diskTicks += dur;
	if (diskTicks >= CLOCK_PERIOD) {
		diskTicks -= CLOCK_PERIOD;

		if (inHandshake == false && --tickCounter <= 0) {
			tickCounter = SEND_BYTE_TICKS;
			if (pio->ReadBit(PP_PortC, IBFA) == false) {
				noSend = false;
				Disk32ServiceStateCheck();

				if (noSend == false) {
					pio->WriteByte(PP_PortA, toSend);
					pio->ChangeBit(PP_PortC, _STBA, false);
					pio->ChangeBit(PP_PortC, _STBA, true);
				}
			}

			if (diskState == WAIT_PRESENT || diskState == WAIT_COMMAND) {
				tickCounter = PRESENT_TICKS;
				diskIcon = 0;
			}
		}
	}
}
//---------------------------------------------------------------------------
void Pmd32::Disk32ServiceStateCheck()
{
	switch (diskState) {
		case WAIT_PRESENT:
			toSend = PRESENTATION;
			break;

		case SEND_DATA:
			toSend = *point++;
			CRC ^= toSend;
			if (--byteCounter == 0)
				diskState = SEND_CRC;
			break;

		case SEND_DATA_MEM:
			toSend = 0xFF;
			if (address >= 0x1800 && address <= 0x1FFF)
				toSend = memory[(address - 0x1800) & 0x03FF];
			address++;
			address &= 0x3FFF;
			CRC ^= toSend;
			if (--byteCounter == 0)
				diskState = SEND_CRC;
			break;

		case SEND_CRC:
			toSend = CRC;
			diskState = WAIT_COMMAND;
			break;

		case SEND_ACK:
			toSend = ACK;
			diskState = SEND_RESULT;
			switch (command) {
				case 'C': // read from PMD 32 memory
					byteCounter = length;
					CRC = ACK;
					diskState = SEND_DATA_MEM;
					break;

				case 'B': // BOOT
				case 'Q': // read logical sector
				case 'R': // read logical sector
				case 'T': // write logical sector
				case 'W': // write logical sector
				case 'S': // write physical sector
				case 'U': // write to PMD 32 memory
				case 'J': // execute code in PMD 32 memory
				case 'G': // get inserted disk image name
				case 'H': // insert disk image
				case 'K': // get actual SD card path
				case 'L': // get catalog of actual SD card path
				case 'M': // change actual SD card path
				case 'N': // create new image in actual SD card path
				case 'P': // get disk parameters
					tickCounter = RDWR_RES_TICKS;
					break;

				case 'F': // format track
					tickCounter = FORMAT_RES_TICKS;
					break;
			}
			break;

		case SEND_RESULT:
			Disk32ServiceSendResultCommand();
			break;

		case SEND_NAK:
			toSend = NAK;
			diskState = WAIT_COMMAND;
			break;

		default:
			noSend = true;
			break;
	}
}
//---------------------------------------------------------------------------
void Pmd32::Disk32ServiceSendResultCommand()
{
	switch (command) {
		case 'B': // BOOT
			drvnum = 0;
			track = 0;
			sector = 0;
			diskIcon = 1;
		case 'Q': // read logical sector
		case 'R': // read logical sector
			if (PrepareSector() == true) {
				point = buffer;
				byteCounter = SECTOR_SIZE;
				CRC = 0;
				diskState = SEND_DATA;
				toSend = RESULT_OK;
			}
			else {
				diskState = WAIT_COMMAND;
				toSend = RESULT_RE;
			}
			break;

		case 'T': // write logical sector
		case 'W': // write logical sector
		case 'S': // write physical sector
		case 'F': // format track
			if (drives[drvnum].wp == true)
				toSend = RESULT_WP;
			else if (WriteSector() == true)
				toSend = RESULT_OK;
			else
				toSend = RESULT_WE;
			diskState = WAIT_COMMAND;
			break;

		case 'I': // drive select
		case '*': // FAST mode
		case '@': // SLOW mode
		case 'U': // write to PMD 32 memory
		case 'J': // execute code in PMD 32 memory
			toSend = RESULT_OK;
			diskState = WAIT_COMMAND;
			break;

		case 'G': // get inserted disk image name
			if (drvnum > 3) {
				toSend = RESULT_BD;
				diskState = WAIT_COMMAND;
			}
			else {
				toSend = RESULT_OK;
				point = buffer;
				CRC = 0;
				diskState = SEND_DATA;
				if (drives[drvnum].handle == NULL) {
					buffer[0] = 0;
					buffer[1] = 0;
					byteCounter = 2;
				}
				else {
					const fs::path &image = drives[drvnum].filePath;
					std::string name = image.lexically_relative(sdRoot / sdPath).generic_string();
					if (name.empty())
						name = image.generic_string().substr(1);
					else if (name.find("..") == 0)
						name = image.generic_string();

					name = name.substr(0, MAX_SD_PATH_LEN);
					buffer[0] = (BYTE) (drives[drvnum].wp ? 1 : 0);
					buffer[1] = (BYTE) name.length();
					memcpy(buffer + 2, name.data(), name.length());
					byteCounter = 2 + name.length();
				}
			}
			break;

		case 'H': // insert disk image
			if (drvnum > 3)
				toSend = RESULT_BD;
			else {
				if (*buffer == 0xFF) {
					SetWpDisk(drvnum, (wp != 0));
					toSend = RESULT_OK;
				}
				else if (*buffer == 0) {
					RemoveDisk(drvnum);
					toSend = RESULT_OK;
				}
				else {
					const fs::path file = sdPath / ToPath((const char *) (buffer + 1));
					if (file.generic_string().length() > MAX_SD_PATH_LEN)
						toSend = RESULT_TL;
					else
						toSend = (BYTE) InsertDisk(drvnum, (sdRoot / file).string().c_str(), (wp != 0));
				}
			}
			diskState = WAIT_COMMAND;
			break;

		case 'K': { // get actual SD card path
			const std::string path = sdPath.generic_string();
			*buffer = (BYTE) path.length();
			memcpy(buffer + 1, path.c_str(), path.length() + 1);
			point = buffer;
			byteCounter = path.length() + 1;
			CRC = 0;
			diskState = SEND_DATA;
			noSend = true;
			break;
		}

		case 'L': // get catalog of actual SD card path
			byteCounter = 0;
			if (FindFile()) {
				byteCounter = sdFile.length();
				memcpy(buffer + 1, sdFile.c_str(), byteCounter + 1);
			}

			*buffer = (BYTE) byteCounter;
			point = buffer;
			byteCounter++;
			CRC = 0;
			diskState = SEND_DATA;
			noSend = true;
			break;

		case 'M': // change actual SD card path
			if (*buffer == 0)
				toSend = RESULT_NF;
			else if (strcmp((char *) (buffer + 1), ".") == 0)
				toSend = RESULT_OK;
			else if (strcmp((char *) (buffer + 1), "..") == 0) {
				if (sdPath.empty())
					toSend = RESULT_NF;
				else {
					sdPath = sdPath.parent_path();
					toSend = RESULT_OK;
				}
			}
			else {
				const fs::path dir = sdRoot / sdPath / ToPath((const char *) (buffer + 1));
				const fs::path found = FindWithNoCase(dir, true);
				if (!DirExists(found.string().c_str()))
					toSend = RESULT_NF;
				else {
					const fs::path relative = found.lexically_relative(sdRoot);
					if (relative.generic_string().length() > MAX_SD_PATH_LEN) {
						sdPath.clear();
						toSend = RESULT_TL;
					}
					else {
						sdPath = relative;
						toSend = RESULT_OK;
					}
				}
			}
			diskState = WAIT_COMMAND;
			break;

		case 'N': // create new disk image in actual SD card path
			if (*buffer == 0)
				toSend = RESULT_CE;
			else {
				std::string input((char *) (buffer + 1), (size_t) *buffer);
				std::transform(input.begin(), input.end(), input.begin(),
					[](unsigned char c) { return (char) std::tolower(c); });

				const fs::path file = sdPath / ToPath(input.c_str());
				if (file.generic_string().length() > MAX_SD_PATH_LEN)
					toSend = RESULT_TL;
				else {
					BYTE trk, sec, phys;
					const std::string path = (sdRoot / file).string();

					toSend = (BYTE) CheckImageType(file, &trk, &sec, &phys);
					if (toSend == RESULT_OK) {
						if (!CreateMedium(path.c_str(), ((trk) ? trk : 256) * sec * SECTOR_SIZE, 0xE5))
							toSend = RESULT_CE;
					}
				}
			}
			diskState = WAIT_COMMAND;
			break;

		case 'P': // get disk parameters
			if (drvnum > 3) {
				toSend = RESULT_BD;
				diskState = WAIT_COMMAND;
			}
			else {
				toSend = RESULT_OK;
				if (drives[drvnum].handle == NULL) {
					*buffer = 0;
					*(buffer + 1) = 0;
					*(buffer + 2) = 0;
				}
				else {
					*buffer = drives[drvnum].tracks;
					*(buffer + 1) = drives[drvnum].sectors;
					*(buffer + 2) = drives[drvnum].sectorSize;
				}
				point = buffer;
				byteCounter = 3;
				CRC = 0;
				diskState = SEND_DATA;
			}
			break;
	}
}
//---------------------------------------------------------------------------
bool Pmd32::FindFile()
{
	std::error_code ec;

	if (findType == 0)
		hFind.reset();

	if (!hFind) {
		hFind = std::make_unique<fs::directory_iterator>(sdRoot / sdPath, ec);
		if (ec) {
			hFind.reset();
			return false;
		}
	}

	const fs::directory_iterator end;
	while (*hFind != end) {
		const fs::directory_entry entry = **hFind;
		hFind->increment(ec);
		if (ec)
			*hFind = end;

		std::error_code sec;
		const bool dir = entry.is_directory(sec);
		if (!dir && !entry.is_regular_file(sec))
			continue;

		const std::string name = entry.path().filename().string();

		// exclude "dot dirs" and hidden files...
		if (name.empty() || name[0] == '.')
			continue;

		sdFile = dir ? "[" + name + "]" : name;
		return true;
	}

	hFind.reset();
	return false;
}
//---------------------------------------------------------------------------
bool Pmd32::PrepareSector()
{
	if (drives[drvnum].handle != NULL && sector < drives[drvnum].sectors
		&& (drives[drvnum].tracks == 0 || track < drives[drvnum].tracks)) {

		DWORD seek = track * drives[drvnum].sectors * SECTOR_SIZE + sector * SECTOR_SIZE;
		if (fseek(drives[drvnum].handle, seek, SEEK_SET) == 0)
			if (fread(buffer, sizeof(BYTE), SECTOR_SIZE, drives[drvnum].handle) == SECTOR_SIZE)
				return true;
	}

	return false;
}
//---------------------------------------------------------------------------
bool Pmd32::WriteSector()
{
	DWORD size;

	if (command == 'S') {
		sector &= 0x3C;
		size = PHYSICAL_SECTOR_SIZE;
	}
	else if (command == 'T' || command == 'W')
		size = SECTOR_SIZE;
	else { // 'F'
		size = drives[drvnum].sectors * SECTOR_SIZE;
		sector = 0;
		memset(buffer, 0xE5, sizeof(buffer));
	}

	if (drives[drvnum].handle != NULL && sector < drives[drvnum].sectors
		&& (drives[drvnum].tracks == 0 || track < drives[drvnum].tracks)) {

		DWORD seek = track * drives[drvnum].sectors * SECTOR_SIZE + sector * SECTOR_SIZE;
		if (fseek(drives[drvnum].handle, seek, SEEK_SET) == 0)
			if (fwrite(buffer, sizeof(BYTE), size, drives[drvnum].handle) == size)
				return true;
	}

	return false;
}
//---------------------------------------------------------------------------
int Pmd32::InsertDisk(int drive, const char *file, bool WP)
{
	RemoveDisk(drive);

	fs::path path = FindWithNoCase(ToPath(file));
	const std::string fileName = fs::absolute(path).string();
	if (!FileExists(fileName.c_str()))
		return RESULT_NF;

	if (extraCommands) {
		int ret = CheckImageType(path, &drives[drive].tracks, &drives[drive].sectors, &drives[drive].sectorSize);
		if (ret != RESULT_OK)
			return ret;
	}
	else {
		drives[drive].tracks = 80;
		drives[drive].sectors = 36;
		drives[drive].sectorSize = 2;
	}

	drives[drive].filePath = path;
	drives[drive].handle = fopen(fileName.c_str(), "rb+");
	drives[drive].wp = WP;

	if (drives[drive].handle == NULL)
		return RESULT_NF;

	return RESULT_OK;
}
//---------------------------------------------------------------------------
void Pmd32::RemoveDisk(int drive)
{
	if (drives[drive].handle)
		fclose(drives[drive].handle);
	drives[drive].handle = NULL;
	drives[drive].filePath.clear();
}
//---------------------------------------------------------------------------
int Pmd32::CheckImageType(const fs::path &fn, BYTE *trk, BYTE *sec, BYTE *phys)
{
	std::string ext = fn.extension().string();
	if (ext.length() < 2)
		return RESULT_TL;
	ext.erase(0, 1);

	std::ifstream config(sdRoot / SDROOT_IMAGES_DEF_FILE);
	if (!config)
		return RESULT_CM;

	// each line: <extension> <tracks> <sectors> [<physical sector size>]
	std::string line;
	while (std::getline(config, line)) {
		std::istringstream in(line);
		std::string name;

		// skip empty lines and comments
		if (!(in >> name) || name[0] == '*')
			continue;
		if (CompareNoCase(name.c_str(), ext.c_str()) != 0)
			continue;

		unsigned tracks, sectors, size = 2;
		if (!(in >> tracks >> sectors) || tracks < 1 || tracks > MAX_TRACKS
			|| sectors < 1 || sectors > MAX_SECTORS_PER_TRACKS)
			return RESULT_CC;

		if ((in >> size) && size > 3)
			return RESULT_CC;

		*trk = tracks;
		*sec = sectors;
		*phys = size;
		return RESULT_OK;
	}

	return config.eof() ? RESULT_UI : RESULT_CR;
}
//---------------------------------------------------------------------------
