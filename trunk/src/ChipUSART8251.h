/*	ChipUSART8251.h: Class for emulation of USART 8251 chip
	Copyright (c) 2006-2007 Roman Borik <pmd85emu@gmail.com>

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
#ifndef ChipUSART8251H
#define ChipUSART8251H
//-----------------------------------------------------------------------------
#include "globals.h"
//-----------------------------------------------------------------------------
enum TUSARTReg { UR_CTRL = 0, UR_STATUS = 0, UR_DATA = 1 };
enum TUSARTPin { UP_TXD, UP_TXR, UP_TXE, UP_RXR, UP_DTR, UP_RTS, UP_SYN, UP_BRK };
//-----------------------------------------------------------------------------
// ******** control word ********

// BaudRate factor for asynchronous mode or select synchronous mode
#define BRF_MASK      0x03
#define BRF_SYNC      0x00    // synchronous mode
#define BRF_ASYNC_1   0x01
#define BRF_ASYNC_16  0x02
#define BRF_ASYNC_64  0x03

// data size
#define CL_MASK       0x0C
#define CL_5          0x00
#define CL_6          0x04
#define CL_7          0x08
#define CL_8          0x0C
#define CL_SHIFT      2

// enable parity
#define PEN_MASK      0x10
#define PEN_DIABLED   0x00
#define PEN_ENABLED   0x10

// parity type
#define PT_MASK       0x20
#define PT_ODD        0x00
#define PT_EVEN       0x20

// size of stop bit - valid only for Tx and asynchronous mode
#define SBL_MASK      0xC0
#define SBL_INVALID   0x00
#define SBL_1         0x40
#define SBL_15        0x80
#define SBL_2         0xC0

// select synchronization method in synchronous mode - internal/external
#define ESD_MASK      0x40
#define ESD_INTERNAL  0x00    // pin SYNDET is output
#define ESD_EXTERNAL  0x40    // pin SYNDET is input

// number of synchronization marks in synchronous mode
#define SCS_MASK      0x80
#define SCS_2         0x00    // 2 synchronization marks
#define SCS_1         0x80    // 1 synchronization mark

//-----------------------------------------------------------------------------
// ******** command word ********

// enable transmitter function
#define TEN_MASK      0x01
#define TEN_DISABLED  0x00
#define TEN_ENABLED   0x01

// DTR control
#define DTR_MASK      0x02
#define DTR_OFF       0x00    // /DTR = 1
#define DTR_ON        0x02    // /DTR = 0

// enable receiver function
#define REN_MASK      0x04
#define REN_DISABLED  0x00
#define REN_ENABLED   0x04

// BREAK mark transmission
#define SBC_MASK      0x08
#define SBC_NORMAL    0x00    // standard operation
#define SBC_BREAK     0x08    // TxD = 0

// clear error flags
#define ER_MASK       0x10
#define ER_NONE       0x00
#define ER_RESET      0x10    // clear flags PE, OE, FE

// RTS control
#define RTS_MASK      0x20
#define RTS_OFF       0x00    // /RTS = 1
#define RTS_ON        0x20    // /RTS = 0

// internal reset of 8251
#define IR_MASK       0x40
#define IR_NONE       0x00
#define IR_RESET      0x40    // internal reset - 8251 expects control word

// find SYNC mark during internal synchronization in synchronous mode
#define EHM_MASK      0x80
#define EHM_DISABLED  0x00
#define EHM_ENABLED   0x80

//-----------------------------------------------------------------------------
// ******** status word ********

// transmitter buffer register is empty
#define TXRDY_MASK    0x01p
#define TXRDY_FULL    0x00
#define TXRDY_EMPTY   0x01

// receiver received data (ready to receive data - pin RxR)
#define RXRDY_MASK    0x02
#define RXRDY_NO      0x00
#define RXRDY_YES     0x02

// buffer and transmit registers are empty - pin TxE
#define TXE_MASK      0x04
#define TXE_FULL      0x00
#define TXE_EMPTY     0x04

// flag - parity error
#define PE_MASK       0x08
#define PE_NO         0x00
#define PE_YES        0x08

// flag - overflow error - CPU didn’t take the data
#define OE_MASK       0x10
#define OE_NO         0x00
#define OE_YES        0x10

// flag - framing error - no STOP bit received in asynchronous mode
#define FE_MASK       0x20
#define FE_NO         0x00
#define FE_YES        0x20

// data synchronization achieved during internal synchronization - output SYNDET
#define SYNDET_MASK   0x40
#define SYNDET_NO     0x00
#define SYNDET_YES    0x40

// BREAK mark detected in synchronous mode - output BRKDET
#define BREAK_MASK    0x40
#define BREAK_NO      0x00
#define BREAK_YES     0x40

// status of /DSR output
#define DSR_MASK      0x80
#define DSR_OFF       0x00    // /DSR = 1
#define DSR_ON        0x80    // /DSR = 0
//-----------------------------------------------------------------------------
// status of initialization
#define INIT_MODE     0
#define INIT_SYNC1    1
#define INIT_SYNC2    2
#define COMMAND_MODE  3
//-----------------------------------------------------------------------------
// status of receiver and transmitter
#define STATE_MASK    0x3F
#define SYNC_MASK     0xC0
#define SYNC_HUNT     0
#define WAIT_START    1
#define ASYNC_TX_IDLE 2
#define START_BIT     3
#define DATA_BITS     4
#define PARITY_BIT    5
#define STOP_BIT      6
#define STOP_BIT15    7
#define STOP_BIT2     8
#define SYNC_CHAR1    64
#define SYNC_CHAR2    128
//-----------------------------------------------------------------------------
class ChipUSART8251 : public sigslot::has_slots<>
{
public:
	// constructor
	ChipUSART8251();

	// CPU side
	void ChipReset(bool clearNotifyFunc);
	void CpuWrite(TUSARTReg dest, BYTE val);
	BYTE CpuRead(TUSARTReg src);

	// notification functions settings
	sigslot::signal0<> OnTxDChange;
	sigslot::signal0<> OnTxRChange;
	sigslot::signal0<> OnTxEChange;
	sigslot::signal0<> OnRxRChange;
	sigslot::signal0<> OnDtrChange;
	sigslot::signal0<> OnRtsChange;
	sigslot::signal0<> OnSynDetChange;
	sigslot::signal0<> OnBrkDetChange;

	sigslot::signal0<> OnTxDSet;
	sigslot::signal0<> OnTxRSet;
	sigslot::signal0<> OnTxESet;
	sigslot::signal0<> OnRxRSet;
	sigslot::signal0<> OnDtrSet;
	sigslot::signal0<> OnRtsSet;
	sigslot::signal0<> OnSynDetSet;
	sigslot::signal0<> OnBrkDetSet;

	int GetChipState(BYTE *buffer);
	void SetChipState(BYTE *buffer);

protected:
	// peripheral side
	void PeripheralSetRxD(bool state);
	bool PeripheralReadTxD();

	void PeripheralSetTxC(bool state);
	void PeripheralSetRxC(bool state);

	bool PeripheralReadTxR();
	bool PeripheralReadTxE();
	bool PeripheralReadRxR();

	void PeripheralSetDSR(bool state);
	bool PeripheralReadDTR();
	void PeripheralSetCTS(bool state);
	bool PeripheralReadRTS();

	void PeripheralSetSynDet(bool state);
	bool PeripheralReadSynBrk();

	// helper methods for complete byte write and read
	// and forced acceleration of reception and transmission
	void SetByteTransferMode(bool byteTransferMode);
	void PeripheralWriteByte(BYTE value);
	BYTE PeripheralReadByte();

private:
	int InitState;

	BYTE CWR;
	BYTE Command;

	int  CharLen;
	int  Factor;

	bool SyncMode;
	bool SynDetState;
	BYTE SyncChar1;
	BYTE SyncChar2;

	int  RxState;
	BYTE RxChar;
	BYTE RxShift;
	bool RxParity;
	int  RxBitCounter;
	int  RxFactorCounter;
	bool RxBreakState; // RxD held in Log.0 state
	int  RxBreakCounter;

	int  TxState;
	BYTE TxChar;
	BYTE TxShift;
	bool TxParity;
	int  TxBitCounter;
	int  TxFactorCounter;
	bool TxBreakState;

	bool StatusTxR;
	bool StatusRxR;
	bool StatusTxE;

	// error flags
	bool ParityError;  // parity error
	bool OverrunError; // overrun error, CPU didn’t take previous data
	bool FrameError;   // end of frame error in asynchronous mode

	bool TxD;
	bool TxC;
	bool RxD;
	bool RxC;

	bool _DSR;
	bool _CTS;

	bool ByteTransferMode;

	void ClearAllNotifyFunctions();
	bool CalculateParity(BYTE value);
	void PrepareAsyncTx();
	void PrepareSyncTx();
	void PrepareSyncRx(bool hunt, bool sync2);
	void SynchroDetected(bool inHunt);
	void CharReceived();
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
