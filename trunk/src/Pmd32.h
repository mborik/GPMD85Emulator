/*	Pmd32.h: Class for emulation of disk drive PMD 32
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
#ifndef Pmd32H
#define Pmd32H
//---------------------------------------------------------------------------
#include "CommonUtils.h"
#include "ChipMemory.h"
#include "IifGPIO.h"
//---------------------------------------------------------------------------
#define OPER_FREQ               20000
#define CLOCK_PERIOD            (CPU_FREQ / OPER_FREQ)
#define PRESENT_TICKS           100     // 5 ms
#define SEND_BYTE_TICKS         2       // 100 us
#define FORMAT_RES_TICKS        8000    // 400 ms
#define RDWR_RES_TICKS          20      // 1 ms

#define PRESENTATION            0xAA
#define ACK                     0x33
#define NAK                     0x99

#define RESULT_OK               0       // OK
#define RESULT_WP               1       // write-protected disk
#define RESULT_FE               2       // format error
#define RESULT_RE               3       // read error
#define RESULT_WE               4       // write error
#define RESULT_BD               5       // bad disk number
#define RESULT_NF               6       // file/dir not found
#define RESULT_TL               7       // file/dir name too long
#define RESULT_CE               8       // disk image creation error
#define RESULT_CM               9       // configuration file missed
#define RESULT_CR               10      // configuration file read error
#define RESULT_CC               11      // configuration file corrupted
#define RESULT_UI               12      // unknown disk image file

#define IDLE_STATE              0
#define WAIT_PRESENT            1
#define WAIT_COMMAND            2
#define WAIT_CRC                3
#define WAIT_SECTOR             4
#define WAIT_TRACK              5
#define WAIT_DRIVE              6
#define WAIT_DATA               7
#define SEND_DATA               8
#define SEND_CRC                9
#define SEND_ACK                10
#define SEND_RESULT             11
#define SEND_NAK                12
#define WAIT_ADDR_H             13
#define WAIT_ADDR_L             14
#define WAIT_LEN_H              15
#define WAIT_LEN_L              16
#define WAIT_DATA_MEM           17
#define SEND_DATA_MEM           18
#define WAIT_FIND_TYPE          19
#define WAIT_LEN_NAME           20
#define WAIT_DATA_NAME          21
#define WAIT_WP                 22

#define SECTOR_SIZE             128
#define SECTORS_PER_PHYSICS     4
#define PHYSICS_SECTOR_SIZE     (SECTORS_PER_PHYSICS * SECTOR_SIZE)

#define MAX_SECTORS_PER_TRACKS  64
#define MAX_TRACKS              256

#define NUM_DRIVES              4
#define DRIVE_A                 0
#define DRIVE_B                 1
#define DRIVE_C                 2
#define DRIVE_D                 3

#define SDROOT_IMAGES_DEF_FILE  "images.cfg"
//---------------------------------------------------------------------------
class Pmd32 : public sigslot::has_slots<>
{
	public:
		Pmd32(IifGPIO *pio, char *romFileName);
		virtual ~Pmd32();

		void SetExtraCommands(bool extraCommands, char *sdPath);
		void BeforeReset();
		void AfterReset();
		void OnSetMode2(BYTE CWR);
		void OnHandshake();

		void Disk32Service(int ticks, int dur);
		int  InsertDisk(int drive, char *file, bool WP);
		void RemoveDisk(int drive);
		inline void SetWpDisk(int drv, bool wp) { drives[drv].wp = wp; }

		int diskIcon;

	private:
		typedef struct {
			char *filePath;
			FILE *handle;
			bool  wp;
			BYTE  tracks;     // total tracks count
			BYTE  sectors;    // logical sectors per track
			BYTE  sectorSize; // physical sector size (0=128B, 1=256B, 2=512B, 3=1KB)
		} DRIVES;

		IifGPIO *pio;
		ChipMemory *memory;
		DRIVES drives[NUM_DRIVES];

		BYTE  drvnum;
		BYTE  track;
		BYTE  sector;
		int   address;
		int   length;
		BYTE  findType;
		BYTE  wp;

		BYTE  buffer[MAX_SECTORS_PER_TRACKS * SECTOR_SIZE];
		BYTE *point;
		DWORD dwRW;

		BYTE  command;
		BYTE  CRC;
		int   byteCounter;
		bool  result;
		bool  inHandshake;

		BYTE  toSend;
		bool  noSend;

		int   diskState;
		int   diskTicks;
		int   tickCounter;

		bool  extraCommands;
		char *sdRoot;
		char *sdPath;
		char *sdFile;
		DIR  *hFind;
		char *hFindDir;

		void  Disk32ServiceStateCheck();
		void  Disk32ServiceSendResultCommand();

		bool  FindFile();
		bool  PrepareSector();
		bool  WriteSector();
		int   CheckImageType(char *fn, BYTE *trk, BYTE *sec, BYTE *phys);
		bool  LocateFileInPathCaseInsensitive(char *filePath);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
