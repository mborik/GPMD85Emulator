/*	Pmd32.h: Class for emulation of disk drive PMD 32
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
#define PHYSICAL_SECTOR_SIZE    (4 * SECTOR_SIZE)
#define INTERNAL_RAM_SIZE       (8 * SECTOR_SIZE)

#define MAX_SECTORS_PER_TRACKS  64
#define MAX_TRACKS              256

#define NUM_DRIVES              4
#define DRIVE_A                 0
#define DRIVE_B                 1
#define DRIVE_C                 2
#define DRIVE_D                 3

#define SDROOT_IMAGES_DEF_FILE  "images.cfg"
//---------------------------------------------------------------------------
class Pmd32
{
	public:
		Pmd32(IifGPIO *pio);
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
		DRIVES drives[NUM_DRIVES];

		BYTE  drvnum;
		BYTE  track;
		BYTE  sector;
		int   address;
		int   length;
		BYTE  findType;
		BYTE  wp;

		BYTE  memory[INTERNAL_RAM_SIZE];
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
