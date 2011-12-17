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
// ******** riadiace slovo ********

// BaudRate faktor pre asynchronny rezim alebo volba synchronneho rezimu
#define BRF_MASK      0x03
#define BRF_SYNC      0x00    // synchronny rezim
#define BRF_ASYNC_1   0x01
#define BRF_ASYNC_16  0x02
#define BRF_ASYNC_64  0x03

// velkost slabiky
#define CL_MASK       0x0C
#define CL_5          0x00
#define CL_6          0x04
#define CL_7          0x08
#define CL_8          0x0C
#define CL_SHIFT      2

// povolenie parity
#define PEN_MASK      0x10
#define PEN_DIABLED   0x00
#define PEN_ENABLED   0x10

// typ parity
#define PT_MASK       0x20
#define PT_ODD        0x00
#define PT_EVEN       0x20

// dlzka stop bitu - ma zmysel iba pre Tx a asynchronny rezim
#define SBL_MASK      0xC0
#define SBL_INVALID   0x00
#define SBL_1         0x40
#define SBL_15        0x80
#define SBL_2         0xC0

// volba sposobu synchronizacie v synchronnom rezime - interna/externa
#define ESD_MASK      0x40
#define ESD_INTERNAL  0x00    // vyvod SYNDET je vystup
#define ESD_EXTERNAL  0x40    // vyvod SYNDET je vstup

// pocet synchronizacnych znakov v synchronnom rezime
#define SCS_MASK      0x80
#define SCS_2         0x00    // 2 synchronizacne znaky
#define SCS_1         0x80    // 1 synchronizacny znak

//-----------------------------------------------------------------------------
// ******** povelove slovo ********

// povolenie cinnosti vysielaca
#define TEN_MASK      0x01
#define TEN_DISABLED  0x00
#define TEN_ENABLED   0x01

// ovladanie DTR
#define DTR_MASK      0x02
#define DTR_OFF       0x00    // /DTR = 1
#define DTR_ON        0x02    // /DTR = 0

// povolenie cinnosti prijimaca
#define REN_MASK      0x04
#define REN_DISABLED  0x00
#define REN_ENABLED   0x04

// vysielanie znaku BREAK
#define SBC_MASK      0x08
#define SBC_NORMAL    0x00    // normalna cinnost
#define SBC_BREAK     0x08    // TxD = 0

// nulovanie chybovych priznakov
#define ER_MASK       0x10
#define ER_NONE       0x00
#define ER_RESET      0x10    // nulovanie priznakov PE, OE, FE

// ovladanie RTS
#define RTS_MASK      0x20
#define RTS_OFF       0x00    // /RTS = 1
#define RTS_ON        0x20    // /RTS = 0

// interne nulovanie 8251
#define IR_MASK       0x40
#define IR_NONE       0x00
#define IR_RESET      0x40    // interny reset - 8251 ocakava riadiace slovo

// vyhladavanie znaku SYNC pri internej synchronizacii v synchronnom rezime
#define EHM_MASK      0x80
#define EHM_DISABLED  0x00
#define EHM_ENABLED   0x80

//-----------------------------------------------------------------------------
// ******** stavove slovo ********

// vyrovnavaci register vysielaca je prazdny
#define TXRDY_MASK    0x01
#define TXRDY_FULL    0x00
#define TXRDY_EMPTY   0x01

// prijimac prijal slabiku (pripravenost prijimaca prijat slabiku) - vyvod RxR
#define RXRDY_MASK    0x02
#define RXRDY_NO      0x00
#define RXRDY_YES     0x02

// vyrovnavaci aj vysielaci register vysielaca je prazdny - vyvod TxE
#define TXE_MASK      0x04
#define TXE_FULL      0x00
#define TXE_EMPTY     0x04

// priznak - chyba parity
#define PE_MASK       0x08
#define PE_NO         0x00
#define PE_YES        0x08

// priznak - chyba prebehu - CPU si neprevzalo znak
#define OE_MASK       0x10
#define OE_NO         0x00
#define OE_YES        0x10

// priznak - chyba ukoncenia - v asynchronnom rezime neprisiel platny STOP bit
#define FE_MASK       0x20
#define FE_NO         0x00
#define FE_YES        0x20

// pri internej synchronizacii sa dosiahla slabikova synchr. - vystup SYNDET
#define SYNDET_MASK   0x40
#define SYNDET_NO     0x00
#define SYNDET_YES    0x40

// bol detekovany znak BREAK v asynchronnom rezime - vystup BRKDET
#define BREAK_MASK    0x40
#define BREAK_NO      0x00
#define BREAK_YES     0x40

// stav vstupu /DSR
#define DSR_MASK      0x80
#define DSR_OFF       0x00    // /DSR = 1
#define DSR_ON        0x80    // /DSR = 0
//-----------------------------------------------------------------------------
// status inicializacie
#define INIT_MODE     0
#define INIT_SYNC1    1
#define INIT_SYNC2    2
#define COMMAND_MODE  3
//-----------------------------------------------------------------------------
// status prijimaca a vysielaca
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
	// konstruktor
	ChipUSART8251();

	// strana CPU
	void ChipReset(bool clearNotifyFunc);
	void CpuWrite(TUSARTReg dest, BYTE val);
	BYTE CpuRead(TUSARTReg src);

	// nastavenie notifikacnych funkcii
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
	// strana periferie
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

	// pomocne metody pre zapis a citanie celeho bytu
	// a vynutene zrychlenie prijimania a vysielania
	void SetByteTransferMode(bool byteTransferMode);
	void PeripheralWriteByte(BYTE value);
	BYTE PeripheralReadByte();

private:
	int InitState;

	BYTE CWR;
	BYTE Command;

	int CharLen;
	int Factor;

	bool SyncMode;
	bool SynDetState;
	BYTE SyncChar1;
	BYTE SyncChar2;

	int RxState;
	BYTE RxChar;
	BYTE RxShift;
	bool RxParity;
	int RxBitCounter;
	int RxFactorCounter;
	bool RxBreakState; // RxD je drzany v stave Log.0
	int RxBreakCounter;

	int TxState;
	BYTE TxChar;
	BYTE TxShift;
	bool TxParity;
	int TxBitCounter;
	int TxFactorCounter;
	bool TxBreakState;

	bool StatusTxR;
	bool StatusRxR;
	bool StatusTxE;

	// chybove priznaky
	bool ParityError;  // chyba parity
	bool OverrunError; // chyba prebehu, CPU si neprevzalo predoslu slabiku
	bool FrameError;   // chyba ukoncenia ramca v asynchronnom rezime

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
