/*	Snapshot.h: Structures and handling of emulator snapshot files.
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>
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
#ifndef SNAPSHOT_H
#define SNAPSHOT_H
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
#define SNAP_SIGNATURE        "PSN"
#define SNAP_SIGNATURE_LEN    3

#define SNAP_VERSION_MAX      2
#define SNAP_HEADER_V1_LEN    56
#define SNAP_HEADER_V2_LEN    124

#define SNAP_MIN_HEADER_LEN   SNAP_HEADER_V1_LEN
#define SNAP_MAX_HEADER_LEN   SNAP_HEADER_V2_LEN
#define SNAP_BLOCK_LEN        16384
//---------------------------------------------------------------------------
enum TSnapshotError {
	SNAP_OK = 0,
	SNAP_WRITE_ERROR = 1,
	SNAP_READ_ERROR,
	SNAP_BAD_HEADER,
	SNAP_UNKNOWN_VERSION,
	SNAP_UNKNOWN_MODEL,
	SNAP_BAD_MEM_BLOCK,
	SNAP_INTERNAL_ERROR
};
//---------------------------------------------------------------------------
const char *SNAPSHOT_ERR_MSG[] = {
	nullptr,
	"Error writing file!\nSnapshot might be corrupted.",
	"Error reading snapshot!",
	"Invalid snapshot format!",
	"Unknown snapshot version!",
	"Unknown snapshot model!",
	"Corrupted memory block!",
	"Internal error!",
};
//---------------------------------------------------------------------------
#pragma pack(push, 1)
struct Snap
{
	char signature[3];     // "PSN"
	BYTE version;
	WORD blocksOffset;
	BYTE model;
	BYTE interrupt;        // CPU
	WORD af;
	WORD bc;
	WORD de;
	WORD hl;
	WORD pc;
	WORD sp;
	WORD romLength;        // ROM
	WORD block0Length;     // 0000:3FFF
	WORD block1Length;     // 4000:7FFF
	WORD block2Length;     // 8000:BFFF
	WORD block3Length;     // C000:FFFF
	BYTE systemPioCwr;     // System PIO
	BYTE systemPioC;
	BYTE systemPioA;
	BYTE gpioCwr;          // GPIO
	BYTE gpioC;
	BYTE gpioB;
	BYTE gpioA;
	BYTE gpioInt;
	BYTE ims2Cwr;          // IMS-2
	BYTE ims2C;
	BYTE ims2B;
	BYTE ims2A;
	BYTE ims2Int;
	BYTE timer0Cwr;        // Timer 0
	BYTE timer0L;
	BYTE timer0H;
	BYTE timer1Cwr;        // Timer 1
	BYTE timer1L;
	BYTE timer1H;
	BYTE timer2Cwr;        // Timer 2
	BYTE timer2L;
	BYTE timer2H;
	BYTE usartCwr;         // USART
	BYTE usartSync1;
	BYTE usartSync2;
	BYTE usartCmd;

	// version 2
	BYTE videoIntVGA;
	BYTE memMap;
	WORD block4Length;
	WORD block5Length;
	WORD block6Length;
	WORD block7Length;
	WORD block8Length;
	WORD block9Length;
	WORD block10Length;
	WORD block11Length;
	WORD block12Length;
	WORD block13Length;
	WORD block14Length;
	WORD block15Length;
	BYTE mif85Int;
	BYTE mif85Regs[32];
	BYTE musicaTimer0Cwr;  // Timer 0
	BYTE musicaTimer0L;
	BYTE musicaTimer0H;
	BYTE musicaTimer1Cwr;  // Timer 1
	BYTE musicaTimer1L;
	BYTE musicaTimer1H;
	BYTE musicaTimer2Cwr;  // Timer 2
	BYTE musicaTimer2L;
	BYTE musicaTimer2H;
};
#pragma pack(pop)
//---------------------------------------------------------------------------
#endif
