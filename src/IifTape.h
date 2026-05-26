/*	IifTape.h: Core of emulation of tape interface
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

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
#ifndef IifTapeH
#define IifTapeH
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
// Cassette recorder interface and IRPS uses port addresses 1Eh and 1Fh
// Because of incomplete address decoder these mirror at 1Ch and 1Dh
#define IIF_TAPE_MASK         0xFC
#define IIF_TAPE_ADR          0x1C

#define IIF_TAPE_REG_MASK     0xFD
#define IIF_TAPE_REG_DATA     0x1C
#define IIF_TAPE_REG_CWR      0x1D
#define IIF_TAPE_REG_STAT     0x1D

// Didaktik Alfa uses addresses 0F0h and 0F1h
#define IIF_TAPE_MASK_A       0xFC
#define IIF_TAPE_ADR_A        0xF0

#define IIF_TAPE_REG_MASK_A   0xFD
#define IIF_TAPE_REG_DATA_A   0xF0
#define IIF_TAPE_REG_CWR_A    0xF1
#define IIF_TAPE_REG_STAT_A   0xF1

#define MAX_TAPE_BLOCK_SIZE	  65535
#define TAPE_FREQ             1200
#define TAPE_HALF_CLOCK       ((CPU_FREQ / TAPE_FREQ) / 2)

#define TC_HEAD_LEADER    (int)(2.8 / (1.0 / TAPE_FREQ)) * 2  // 2.8 sec
#define TC_BODY_LEADER    (int)(0.5 / (1.0 / TAPE_FREQ)) * 2  // 0.5 sec
#define TC_STOP_TAIL      (int)(0.2 / (1.0 / TAPE_FREQ)) * 2  // 0.2 sec
#define TC_GAP_SIZE       (int)(1.5 / (1.0 / TAPE_FREQ)) * 2  // 1.5 sec
#define TC_NO_BYTE        (int)(1.0 / (1.0 / TAPE_FREQ)) * 2  // 1.0 sec
#define TC_BYTE_CLOCK     ((CPU_FREQ / TAPE_FREQ) * 11)
#define TC_START          2
#define TC_STOP           4
#define TC_DATA           16
#define TC_EB_GAP         (int)(0.2 / (1.0 / TAPE_FREQ)) * 2  // 0.2 sec
#define TC_EB_MAX         (int)(2.0 / (1.0 / TAPE_FREQ)) * 2  // 2.0 sec

#define TC_PULSE_SHORT    619       // 302,246 us
#define TC_PULSE_LONG     1211      // 591,309 us

#define TP_RX_IDLE        0
#define TP_RX_LEADER      1
#define TP_RX_START       2
#define TP_RX_DATA        3
#define TP_RX_STOP        4
#define TP_RX_TAIL        5
#define TP_RX_GAP         6
#define TP_RX_PULSE_0     7
#define TP_RX_PULSE_1     8
#define TP_RX_LOG_0       9
#define TP_RX_LOG_1       10
#define TP_RX_BYTE        11
#define TP_RX_BLOCK       12
#define TP_RX_BLOCK_END   13

#define TP_TX_FF          0
#define TP_TX_00          1
#define TP_TX_55          2
#define TP_TX_HEAD        3
#define TP_TX_BODY        4
#define TP_TX_WAIT_EB     5
#define TP_TX_EXT_BODY    6
#define TP_TX_WAIT_HEAD   7
#define TP_TX_WAIT_BODY   8

#define CMD_STOP          1
#define CMD_NEXT          2
#define CMD_PROGRESS      3
#define CMD_MONITORING    4
#define CMD_PRE_SAVE      5
#define CMD_SAVE          6
//---------------------------------------------------------------------------
class IifTape {
	public:
		IifTape(TComputerModel model, TTapeIfType ifType);

		sigslot::signal2<int, bool *> TapeCommand;
		sigslot::signal3<int, bool, int> PrepareSample;

		int GetTapeIcon();

		TComputerModel inline GetModel() { return model; }

		inline bool IsFlashLoadOn() { return flashLoad; }
		bool GetTapeByte(BYTE *byte);
		bool GetTapeBlock(BYTE **point, WORD *len);
		void AcceptTapeBlock(WORD len);

		void PrepareBlock(BYTE *data, WORD length, bool head, bool flash, bool onPlay);
		int GetSavedBlock(BYTE **pbuf);

		inline WORD GetRestDatalen() { return dataLen; }
		inline int GetRxState() { return tapeRxState; }
		inline int GetTxState() { return tapeTxState; }
		inline void SetTxState(int txState) { tapeTxState = txState; }

	protected:
		TComputerModel model;
		TTapeIfType ifType;

		int tapeTicks;
		bool tapeClkState;

		BYTE *data;
		WORD dataLen;
		bool head;
		bool flashLoad;

		int tapeRxState;
		int rxTickCounter;
		BYTE byte;
		bool bit;

		int tapeTxState;
		int txByteCounter;
		int txBodyEnd;
		int txTickCounter;
		BYTE buff[TAPE_BLOCK_SIZE + 2];
		BYTE crc;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
