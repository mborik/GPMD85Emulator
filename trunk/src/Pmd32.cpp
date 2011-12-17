/*	Pmd32.cpp: Class for emulation of disk drive PMD 32
	Copyright (c) 2008-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011 Martin Borik <mborik@users.sourceforge.net>

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
//---------------------------------------------------------------------------
#include "Pmd32.h"
//---------------------------------------------------------------------------
Pmd32::Pmd32(IifGPIO *pio, char *romFileName)
{
	this->pio = pio;
	pio->OnBeforeResetA.connect(this, &Pmd32::BeforeReset);
	pio->OnAfterResetA.connect(this, &Pmd32::AfterReset);

	for (int ii = 0; ii < NUM_DRIVES; ii++) {
		drives[ii].filePath = NULL;
		drives[ii].handle = NULL;
		drives[ii].wp = true;
	}

	extraCommands = false;

	sdRoot = NULL;
	sdPath = NULL;
	sdFile = NULL;

	hFind = NULL;
	hFindDir = NULL;

	drvnum = 0;
	diskIcon = 0;
	diskTicks = 0;
	diskState = IDLE_STATE;
	tickCounter = CPU_FREQ;
	inHandshake = false;

	memory = new ChipMemory(2 + 1);                 // 2 kB ROM, 1kB RAM
	memory->AddBlock(0, 2, 0L, NO_PAGED, MA_RO);    // ROM 0000h - 07FFh
	memory->AddBlock(6, 1, 2048L, NO_PAGED, MA_RW); // RAM 1800h - 1BFFh
	memory->Page = NO_PAGED;

	char *romFile = LocateROM(romFileName);
	if (romFile == NULL)
		romFile = romFileName;

	ReadFromFile(romFile, 0, 2048, buffer);
	memory->PutRom(0, NO_PAGED, buffer, 2048);
}
//---------------------------------------------------------------------------
Pmd32::~Pmd32()
{
	RemoveDisk(DRIVE_A);
	RemoveDisk(DRIVE_B);
	RemoveDisk(DRIVE_C);
	RemoveDisk(DRIVE_D);

	if (sdRoot)
		delete [] sdRoot;
	sdRoot = NULL;

	if (sdPath)
		delete [] sdPath;
	sdPath = NULL;

	if (sdFile)
		delete [] sdFile;
	sdFile = NULL;

	if (hFind != NULL)
		closedir(hFind);
	hFind = NULL;

	if (memory)
		delete memory;
	memory = NULL;
}
//---------------------------------------------------------------------------
void Pmd32::SetExtraCommands(bool extraCommands, char *sdRoot)
{
	if (sdRoot == NULL)
		return;

	this->extraCommands = extraCommands;

	int l, i;
	for (l = strlen(sdRoot), i = 0; i < l; i++)
		if (sdRoot[i] == '\\')
			sdRoot[i] = '/';

	this->sdRoot = new char[l + 2];
	strcpy(this->sdRoot, sdRoot);

	if (sdRoot[l - 1] != '/') {
		this->sdRoot[l++] = '/';
		this->sdRoot[l] = '\0';
	}
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
	pio->OnCpuWriteCWR.connect(this, &Pmd32::OnSetMode2);
	pio->OnCpuWriteCH.connect(this, &Pmd32::OnHandshake);

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
		pio->ChangeBit(PP_PortC, _ACKA, false);
		val = pio->ReadByte(PP_PortA);
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
						byteCounter = PHYSICS_SECTOR_SIZE + 1;
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
				CRC ^= val;
				if (command == 'J') {
					debug("%c: %04X", command, address);
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
					debug("%c: %04X,%04X", command, address, length);

				if (command == 'C')
					diskState = WAIT_CRC;
				else {
					byteCounter = length;
					diskState = WAIT_DATA_MEM;
				}
				break;

			case WAIT_DATA_MEM:
				memory->WriteByte(address, val);
				address++;
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
			toSend = memory->ReadByte(address);
			address++;
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
					char *path = NULL, *ptr;

					if (sdPath) {
						path = new char[strlen(sdRoot) + strlen(sdPath) + 1];
						strcpy(path, sdRoot);
						strcat(path, sdPath);
					}
					else {
						path = new char[strlen(sdRoot) + 1];
						strcpy(path, sdRoot);
					}

					ptr = strstr(drives[drvnum].filePath, path);

					if (ptr == drives[drvnum].filePath) {
						ptr = drives[drvnum].filePath + strlen(path);
						if (*ptr == '/')
							ptr++;
					}
					else
						ptr = drives[drvnum].filePath;

					buffer[0] = (BYTE) (drives[drvnum].wp ? 1 : 0);
					buffer[1] = (BYTE) strlen(ptr);
					memcpy(buffer + 2, ptr, strlen(ptr));
					byteCounter = 2 + strlen(ptr);

					delete [] path;
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
					char *file, *input = ((char *) (buffer + 1));
					if (sdPath == NULL || strlen(sdPath) == 0) {
						file = new char[strlen(input) + 1];
						strcpy(file, input);
					}
					else {
						file = new char[strlen(sdPath) + strlen(input) + 2];
						sprintf(file, "%s/%s", sdPath, input);
					}

					if (strlen(file) > 63)
						toSend = RESULT_TL;
					else {
						char *path = new char[strlen(sdRoot) + strlen(file) + 1];
						strcpy(path, sdRoot);
						strcat(path, file);
						toSend = (BYTE) InsertDisk(drvnum, path, (wp != 0));
						delete [] path;
					}

					delete [] file;
				}
			}
			diskState = WAIT_COMMAND;
			break;

		case 'K': // get actual SD card path
			byteCounter = 0;
			if (sdPath)
				byteCounter = strlen(sdPath);

			if (byteCounter > 0)
				memcpy(buffer, sdPath, byteCounter + 1);

			*buffer = (BYTE) byteCounter;
			point = buffer;
			byteCounter++;
			CRC = 0;
			diskState = SEND_DATA;
			noSend = true;
			break;

		case 'L': // get catalog of actual SD card path
			if (FindFile() == true) {
				byteCounter = strlen(sdFile);
				if (byteCounter > 0)
					memcpy(buffer + 1, sdFile, byteCounter + 1);
			}
			else
				byteCounter = 0;

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
				if (sdPath == NULL)
					toSend = RESULT_NF;
				else if (strlen(sdPath) == 0)
					toSend = RESULT_NF;
				else {
					char *ptr = strrchr(sdPath, '/');
					if (ptr)
						*ptr = '\0';
					else
						*sdPath = '\0';
					toSend = RESULT_OK;
				}
			}
			else {
				char *dir, *input = ((char *) (buffer + 1));
				if (sdPath == NULL || strlen(sdPath) == 0) {
					dir = new char[strlen(sdRoot) + strlen(input) + 1];
					strcpy(dir, sdRoot);
					strcat(dir, input);
				}
				else {
					dir = new char[strlen(sdRoot) + strlen(sdPath) + strlen(input) + 2];
					sprintf(dir, "%s%s/%s", sdRoot, sdPath, input);
				}

				if (DirExists(dir)) {
					dir += strlen(sdRoot);
					if (strlen(dir) > 63) {
						if (sdPath)
							*sdPath = '\0';

						toSend = RESULT_TL;
					}
					else {
						if (sdPath)
							delete [] sdPath;

						sdPath = new char[strlen(dir) + 1];
						strcpy(sdPath, dir);
						toSend = RESULT_OK;
					}
				}
				else
					toSend = RESULT_NF;

				delete [] dir;
			}
			diskState = WAIT_COMMAND;
			break;

		case 'N': // create new disk image in actual SD card path
			if (*buffer == 0)
				toSend = RESULT_CE;
			else {
				char *file, *input = ((char *) (buffer + 1));
				for (unsigned i = 0; i < strlen(input); i++)
					input[i] = tolower(input[i]);

				if (sdPath == NULL || strlen(sdPath) == 0) {
					file = new char[strlen(input) + 1];
					strcpy(file, input);
				}
				else {
					file = new char[strlen(sdPath) + strlen(input) + 2];
					sprintf(file, "%s/%s", sdPath, input);
				}

				if (strlen(file) > 63)
					toSend = RESULT_TL;
				else {
					BYTE trk, sec, phys;
					char *path = new char[strlen(sdRoot) + strlen(file) + 1];
					strcpy(path, sdRoot);
					strcat(path, file);

					toSend = (BYTE) CheckImageType(path, &trk, &sec, &phys);
					if (toSend == RESULT_OK) {
						if (!CreateMedium(path, ((trk) ? trk : 256) * sec * SECTOR_SIZE, 0xE5))
							toSend = RESULT_CE;
					}

					delete [] path;
				}

				delete [] file;
			}
			diskState = WAIT_COMMAND;
			break;

		case 'P': // get disk parameters
			*buffer = drives[drvnum].tracks;
			*(buffer + 1) = drives[drvnum].sectors;
			*(buffer + 2) = drives[drvnum].sectorSize;
			point = buffer;
			byteCounter = 3;
			CRC = 0;
			diskState = SEND_DATA;
			noSend = true;
			break;
	}
}
//---------------------------------------------------------------------------
bool Pmd32::FindFile()
{
	if (findType == 0 && hFind != NULL) {
		closedir(hFind);
		hFind = NULL;
	}

	if (hFind == NULL) {
		if (hFindDir)
			delete [] hFindDir;

		if (sdPath) {
			hFindDir = new char[(strlen(sdRoot) + strlen(sdPath) + 2)];
			sprintf(hFindDir, "%s%s/", sdRoot, sdPath);
		}
		else {
			hFindDir = new char[strlen(sdRoot) + 1];
			strcpy(hFindDir, sdRoot);
		}

		if ((hFind = opendir(hFindDir)) == NULL)
			return false;
	}

	bool dir;
	struct dirent *dirent;

	while ((dirent = readdir(hFind)) != NULL) {
		if (!(dirent->d_type == DT_REG
		   || dirent->d_type == DT_LNK
		   || dirent->d_type == DT_DIR))
			continue;

		dir = (dirent->d_type == DT_DIR);

		// exclude "dot dirs" and hidden files...
		if (dirent->d_name[0] == '.')
			continue;

		if (sdFile)
			delete [] sdFile;
		sdFile = new char[(strlen(dirent->d_name) + (dir ? 3 : 1))];

		if (dir)
			sprintf(sdFile, "[%s]", dirent->d_name);
		else
			strcpy(sdFile, dirent->d_name);
		return true;
	}

	closedir(hFind);
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
		size = PHYSICS_SECTOR_SIZE;
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
int Pmd32::InsertDisk(int drive, char *file, bool WP)
{
	RemoveDisk(drive);

	if (!FileExists(file)) {
		char *xName = strrchr(file, '/'), *xPath;
		if (xName) {
			int i = (xName - file);
			xPath = new char[i + 1];
			strncpy(xPath, file, i);
			xPath[i] = '\0';
			xName++;
		}
		else {
			xPath = new char[strlen(PathApplication) + 1];
			strcpy(xPath, PathApplication);
			xName = file;
		}

		struct dirent *dirent;
		DIR *dir = opendir(xPath);

		if (dir != NULL) {
			while ((dirent = readdir(dir))) {
				if (!(dirent->d_type == DT_REG
				   || dirent->d_type == DT_LNK))
					continue;

				// exclude "dot dirs" and hidden files...
				if (dirent->d_name[0] == '.')
					continue;

				if (strcasecmp(dirent->d_name, xName) == 0) {
					strcpy(xName, dirent->d_name);
					break;
				}
			}

			closedir(dir);
		}

		delete [] xPath;
	}

	if (!FileExists(file))
		return RESULT_NF;

	if (extraCommands) {
		int ret = CheckImageType(file, &drives[drive].tracks, &drives[drive].sectors, &drives[drive].sectorSize);
		if (ret != RESULT_OK)
			return ret;
	}
	else {
		drives[drive].tracks = 80;
		drives[drive].sectors = 36;
		drives[drive].sectorSize = 2;
	}

	if (strchr(file, '\\'))
		for (int l = strlen(file), i = 0; i < l; i++)
			if (file[i] == '\\')
				file[i] = '/';

	if (drives[drive].filePath)
		delete [] drives[drive].filePath;

	drives[drive].filePath = new char[strlen(file) + 1];
	strcpy(drives[drive].filePath, file);

	drives[drive].handle = fopen(file, "rb");
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
}
//---------------------------------------------------------------------------
int Pmd32::CheckImageType(char *fn, BYTE *trk, BYTE *sec, BYTE *phys)
{
	int ret;
	unsigned i, j, k;
	char c, *ptr, *config = new char[strlen(sdRoot) + 11], buf[65];

	strcpy(config, sdRoot);
	strcat(config, SDROOT_IMAGES_DEF_FILE);

	if (FileExists(config) == false)
		return RESULT_CM;

	FILE *file = fopen(config, "r");
	if (file == NULL)
		return RESULT_CM;

	fseek(file, 0, SEEK_SET);

	ptr = strrchr(fn, '.');
	if (ptr == NULL)
		return RESULT_TL;

	ptr++;

	c = getc(file);
	ret = RESULT_UI;

	while (true) {
		while (c == ' ' || c == '\n' || c == '\r')
			c = getc(file);

		if (c == '*') {
			while (c != '\n')
				c = getc(file);
			continue;
		}

		*buf = c;
		fgets(buf + 1, 63, file);

		for (i = 0; buf[i] > ' ' || ptr[i + 1] > ' '; i++) {
			if (toupper(buf[i]) != toupper(ptr[i])) {
				i = 0;
				break;
			}
		}

		if (i == 0) {
			ret = RESULT_CC;
			break;
		}

		k = (unsigned) -1;
		sscanf(buf + i, "%u %u %u", &i, &j, &k);

		if (i < 1 || i > MAX_TRACKS) {
			ret = RESULT_CC;
			break;
		}
		else
			*trk = i;

		if (j < 1 || j > MAX_SECTORS_PER_TRACKS) {
			ret = RESULT_CC;
			break;
		}
		else
			*sec = j;

		if (k == (unsigned) -1)
			*phys = 2;
		else if (k > 3) {
			ret = RESULT_CC;
			break;
		}
		else
			*phys = k;

		ret = RESULT_OK;
		break;
	}

	fclose(file);
	delete [] config;
	return ret;
}
//---------------------------------------------------------------------------
