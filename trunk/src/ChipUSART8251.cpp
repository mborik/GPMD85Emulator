/*	ChipUSART8251.cpp: Class for emulation of USART 8251 chip
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
//---------------------------------------------------------------------------
#include "ChipUSART8251.h"
//---------------------------------------------------------------------------
/**
 * Konstruktor pre vytvorenie objektu cipu USART 8251. Zodpoveda stavu Power-up,
 * teda pripojeniu napajania spojeneho s resetom obvodu.
 * Zaroven sa nastavia adresy notifykacnych funkcii na NULL.
 */
ChipUSART8251::ChipUSART8251()
{
	ChipReset(true);
	ByteTransferMode = false;
	CWR = 0;
	SyncChar1 = 0;
	SyncChar2 = 0;
	Command = 0;
}
//---------------------------------------------------------------------------
/**
 * Metoda GetChipState je pouzivana pri vytvarani Snapshotu a ulozi niektore
 * vnutorne registre chipu do buffra. Ak je buffer null, vrati potrebnu velkost
 * buffra v bytoch.
 * Data sa do buffra ulozia v poradi: CWR, SyncChar1, SyncChar1, Command
 *
 * @param buffer buffer kam sa ulozi stav chipu
 * @return pocet bytov ulozenych do buffra
 */
int ChipUSART8251::GetChipState(BYTE *buffer)
{
	if (buffer != NULL) {
		*buffer = CWR;
		*(buffer + 1) = SyncChar1;
		*(buffer + 2) = SyncChar2;
		*(buffer + 3) = Command;
	}

	return 4;
}
//---------------------------------------------------------------------------
/**
 * Metoda SetChipState je pouzivana po otvoreni Snapshotu pre prednastavenie
 * niektorych vnutornych registrov chipu.
 * Data v buffri musia byt v poradi: CWR, SyncChar1, SyncChar1, Command
 *
 * @param buffer buffer kam sa ulozi stav chipu
 * @return pocet bytov ulozenych do buffra
 */
void ChipUSART8251::SetChipState(BYTE *buffer)
{
	if (buffer != NULL && *buffer != 0) {
		ChipReset(false);
		CpuWrite(UR_CTRL, *buffer);
		if ((*buffer & 3) == 0)
			CpuWrite(UR_CTRL, *(buffer + 1));
		if ((*buffer & 131) == 0)
			CpuWrite(UR_CTRL, *(buffer + 2));
		CpuWrite(UR_CTRL, *(buffer + 3));
	}
}
//---------------------------------------------------------------------------
/**
 * Metoda ChipReset prevedie reset cipu. Zodpoveda privedeniu urovne H na vstup
 * RESET (21).
 * Volitelne je mozne zrusit vsetky notifikacne funkcie.
 *
 * @param clearNotifyFunc ak je true, zrusi vsetky notifykacne funkcie
 */
void ChipUSART8251::ChipReset(bool clearNotifyFunc)
{
	InitState = INIT_MODE;

	if (clearNotifyFunc == true)
		ClearAllNotifyFunctions();
}
//---------------------------------------------------------------------------
/**
 * Privatna metoda pre nastavenie vsetkych notifykacnych funkcii na NULL.
 */
void ChipUSART8251::ClearAllNotifyFunctions()
{
	OnTxDChange.disconnect_all();
	OnTxRChange.disconnect_all();
	OnTxEChange.disconnect_all();
	OnRxRChange.disconnect_all();
	OnDtrChange.disconnect_all();
	OnRtsChange.disconnect_all();
	OnSynDetChange.disconnect_all();
	OnBrkDetChange.disconnect_all();

	OnTxDSet.disconnect_all();
	OnTxRSet.disconnect_all();
	OnTxESet.disconnect_all();
	OnRxRSet.disconnect_all();
	OnDtrSet.disconnect_all();
	OnRtsSet.disconnect_all();
	OnSynDetSet.disconnect_all();
	OnBrkDetSet.disconnect_all();
}
//---------------------------------------------------------------------------
/**
 * Metodu CpuWrite vola mikroprocesor pri vykonavani instrukcie OUT - zapis na
 * port (CPU -> PIO). Zodpoveda teda privedeniu urovne L na vstupy /CS (11) a
 * /WR (10).
 * Podla typu cieloveho registra a stavu obvodu sa prevedie zapis do riadiace,
 * poveloveho alebo datoveho registra.
 *
 * @param dest oznacuje register (TUSARTReg), do ktoreho sa zapise hodnota 'val'
 * @param val zapisovana hodnota
 */
void ChipUSART8251::CpuWrite(TUSARTReg dest, BYTE val)
{
	switch (dest) {
		case UR_CTRL :
			switch (InitState) {
				case INIT_MODE :  // riadiace slovo
					CWR = val;

					CharLen = ((CWR & CL_MASK) >> CL_SHIFT) + 5;
					RxBitCounter = CharLen;
					TxBitCounter = CharLen;

					switch (val & BRF_MASK) {
						case BRF_SYNC :
							Factor = 0;
							break;

						case BRF_ASYNC_1 :
							Factor = 1;
							break;

						case BRF_ASYNC_16 :
							Factor = 16;
							break;

						case BRF_ASYNC_64 :
							Factor = 64;
							break;
					}

					SyncMode = ((val & BRF_MASK) == BRF_SYNC);
					SynDetState = false;
					OnSynDetSet();

					StatusTxE = true;
					OnTxESet();
					StatusTxR = true;
					OnTxRSet();
					StatusRxR = false;
					OnRxRSet();
					ParityError = false;
					OverrunError = false;
					FrameError = false;
					RxState = 0;
					RxBreakState = false;
					RxBreakCounter = 0;
					TxState = 0;
					TxBreakState = false;

					RxD = true;
					RxC = true;
					TxD = true;
					OnTxDSet();
					TxC = false;
					_DSR = true;
					_CTS = true;
//					debug("_CTS=%d", _CTS);

					if (SyncMode == true)
						InitState = INIT_SYNC1;
					else {
						InitState = COMMAND_MODE;
						PrepareAsyncTx();
						RxState = WAIT_START;
					}

					break;

				case INIT_SYNC1 : // 1. synchronizacny znak
					SyncChar1 = val;
					if ((CWR & SCS_MASK) == SCS_2)
						InitState = INIT_SYNC2;
					else {
						InitState = COMMAND_MODE;
						PrepareSyncTx();
						PrepareSyncRx(true, false);
					}
					break;

				case INIT_SYNC2 : // 2. synchronizacny znak
					SyncChar2 = val;
					InitState = COMMAND_MODE;
					PrepareSyncTx();
					PrepareSyncRx(true, false);
					break;

				case COMMAND_MODE : // povelove slovo
					BYTE oldCommand = Command;
					Command = val;

					if ((val & IR_MASK) == IR_RESET)  // interny reset
						InitState = INIT_MODE;
					else {
						// nulovanie chybovych priznakov
						if ((val & ER_MASK) == ER_RESET) {
							ParityError = false;
							OverrunError = false;
							FrameError = false;
						}

						// vysielanie BREAK
						if ((val & SBC_MASK) == SBC_BREAK) {
							TxBreakState = true;
							if (TxD == true) {
								TxD = false;
								OnTxDSet();
								OnTxDChange();
							}
						}
						else {
							// zrusenie vysielania BREAK
							if (TxBreakState == true) {
								TxBreakState = false;
								TxD = true;
								OnTxDSet();
								OnTxDChange();
								if (SyncMode == true)
									PrepareSyncTx();
								else
									PrepareAsyncTx();
							}
						}

						// nastavenie DTR
						OnDtrSet();
						if ((oldCommand & DTR_MASK) ^ (Command & DTR_MASK))
							OnDtrChange();

						// nastavenie RTS
//						debug("!(Command & RTS_MASK)=%d", !(Command & RTS_MASK));
						OnRtsSet();
						if ((oldCommand & RTS_MASK) ^ (Command & RTS_MASK))
							OnRtsChange();

						// spustenie vyhladavania synchronizacie
						if (SyncMode == true  && (val & EHM_MASK) == EHM_ENABLED) {
							if ((CWR & ESD_MASK) == ESD_INTERNAL)
								RxState = SYNC_HUNT | SYNC_CHAR1;
							else
								RxState = SYNC_HUNT;
						}
					}
					break;
			}
			break;

		case UR_DATA :
			if (InitState != COMMAND_MODE)
				return;

			TxChar = val;

			bool oldStatusTxE = StatusTxE;
			bool oldStatusTxR = StatusTxR;
			StatusTxE = false;
			StatusTxR = false;
			OnTxESet();
			if (oldStatusTxE)
				OnTxEChange();
			OnTxRSet();
			if (oldStatusTxR)
				OnTxRChange();
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Metodu CpuRead vola mikroprocesor pri vykonavani instrukcie IN - citanie z
 * portu (CPU <- PIO). Zodpoveda teda privedeniu urovne L na vstupy /CS (11) a
 * /RD (13).
 * Podla typu zdrojoveho registra a stavu obvodu sa vrati stavove slovo obsah
 * datoveho registra.
 *
 * @param src oznacuje register (TUSARTReg), z ktoreho sa ma udaj precitat
 * @return hodnota precitana z registra
 */
BYTE ChipUSART8251::CpuRead(TUSARTReg src)
{
	BYTE val = 0xFF;

	switch (src) {
		case UR_STATUS :
			val = (BYTE)(((StatusTxR == true) ? TXRDY_EMPTY : TXRDY_FULL)
					| ((StatusRxR == true) ? RXRDY_YES : RXRDY_NO)
					| ((StatusTxE == true) ? TXE_EMPTY : TXE_FULL)
					| ((ParityError == true) ? PE_YES : PE_NO)
					| ((OverrunError == true) ? OE_YES : OE_NO)
					| ((FrameError == true) ? FE_YES : FE_NO)
					| ((SynDetState == true) ? SYNDET_YES : SYNDET_NO)
					| ((_DSR == true) ? DSR_OFF : DSR_ON));

			if (SyncMode == true && (CWR & ESD_MASK) == ESD_INTERNAL && SynDetState == true) {
				SynDetState = false;
				OnSynDetSet();
				OnSynDetChange();
			}
			break;

		case UR_DATA :
			if (StatusRxR == true) {
				val = RxChar;
				StatusRxR = false;
				OnRxRSet();
				OnRxRChange();
			}
			break;
	}

	return val;
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii nastavit pozadovanu log. uroven na vstupe RxD (3).
 * V asynchronnom rezime vyhodnocuje prichod start bitu a ukoncenie stavu Break.
 *
 * @param state log. uroven privedena na vstup RxD
 */
void ChipUSART8251::PeripheralSetRxD(bool state)
{
	if (SyncMode == false && RxState == WAIT_START && RxD == true && state == false) {
		RxState = START_BIT;
		RxFactorCounter = (Factor > 1) ? Factor / 2 : Factor;
	}
	if (SyncMode == false && state == true) {
		bool oldRxBreakState = RxBreakState;
		RxBreakState = false;
		OnBrkDetSet();
		if (oldRxBreakState)
			OnBrkDetChange();
		RxBreakCounter = 0;
	}

	RxD = state;
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven na vystupe TxD (19).
 * Tento vystup riadi vysielac.
 *
 * @return log. uroven na vystupe TxD
 */
bool ChipUSART8251::PeripheralReadTxD()
{
	return TxD;
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii riadit hodinovy signal vysielaca, teda nastavit pozadovanu
 * log. uroven na vstupe _TxC (9). Pri zostupnej hrane tohto signalu sa meni
 * stav vystupu TxD.
 *
 * @param state log. uroven na vstupe _TxC
 */
void ChipUSART8251::PeripheralSetTxC(bool state)
{
	// zmena bitu je pri zostupnej hrane TxC --__
	bool oldTxC = TxC;
	TxC = state;
	if (InitState != COMMAND_MODE || TxBreakState == true || oldTxC == TxC || TxC == true)
		return;

	bool oldTxD = TxD;
	if (SyncMode == true) { // Synchronny rezim
		switch (TxState & STATE_MASK) {
			case DATA_BITS :
				if ((_CTS == true || (Command & TEN_MASK) == TEN_DISABLED) && TxBitCounter == CharLen)
					return; // vysielanie nie je povolene

				TxD = (TxShift & 1);
				TxShift >>= 1;
				if (--TxBitCounter == 0) {
					if ((CWR & PEN_MASK) == PEN_ENABLED)
						TxState = (TxState & SYNC_MASK) | PARITY_BIT;
					else
						PrepareSyncTx();
				}
				break;

			case PARITY_BIT :
				TxD = TxParity;
				PrepareSyncTx();
				break;
		}
	}
	else {  // Asynchronny rezim
		if (--TxFactorCounter > 0)
			return;
		TxFactorCounter = Factor;

		switch (TxState) {
			case START_BIT :
				if (_CTS == true || (Command & TEN_MASK) == TEN_DISABLED)
					return; // vysielanie nie je povolene

				TxD = false;
				TxState = DATA_BITS;
				break;

			case DATA_BITS :
				TxD = (TxShift & 1);
				TxShift >>= 1;
				if (--TxBitCounter == 0)
					TxState = ((CWR & PEN_MASK) == PEN_ENABLED) ? PARITY_BIT : STOP_BIT;
				break;

			case PARITY_BIT :
				TxD = TxParity;
				TxState = STOP_BIT;
				break;

			case STOP_BIT : // 1. stop bit
				TxD = true;
				switch (CWR & SBL_MASK) {
					case SBL_15 :
						TxState = STOP_BIT15;
						break;

					case SBL_2 :
						TxState = STOP_BIT2;
						break;

					default :
						TxState = ASYNC_TX_IDLE;
						break;
				}
				break;

			case STOP_BIT15 : // 1/2 stop bit
				if (TxFactorCounter > 1)
					TxFactorCounter /= 2;
			case STOP_BIT2 :  // 2. stop bit
				TxD = true;
				TxState = ASYNC_TX_IDLE;
				break;

			case ASYNC_TX_IDLE :
				PrepareAsyncTx();
				if (TxState == START_BIT) {
					TxD = false;
					TxState = DATA_BITS;
				}
				break;
		}
	}

	OnTxDSet();
	if (oldTxD != TxD)
		OnTxDChange();
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii riadit hodinovy signal prijimaca, teda nastavit pozadovanu
 * log. uroven na vstupe _RxC (25). Pri nabeznej hrane tohto signalu sa vzorkuje
 * stav vstupu RxD.
 *
 * @param state log. uroven na vstupe _RxC
 */
void ChipUSART8251::PeripheralSetRxC(bool state)
{
	// zmena bitu je pri nabeznej hrane RxC __--
	bool oldRxC = RxC;
	RxC = state;
	if (InitState != COMMAND_MODE || oldRxC == RxC || RxC == false)
		return;

	if (SyncMode == false) { // Asynchronny rezim
		if (--RxFactorCounter > 0)
		 return;
		RxFactorCounter = Factor;
	}

	switch (RxState & STATE_MASK) {
		case SYNC_HUNT :  // Synchronny rezim
			if ((CWR & ESD_MASK) == ESD_INTERNAL) {
				if (--RxBitCounter > 0) {
					if ((bool)(RxShift & 1) == RxD)
						RxShift >>= 1;
					else
						PrepareSyncRx(true, false);
				}
				else {
					if (RxParity == RxD)
						SynchroDetected(true);
					else
						PrepareSyncRx(true, false);
				}
			}
			break;

		case WAIT_START : // Asynchronny rezim
			if (RxD == false && RxBreakCounter > 0) {
				RxState = START_BIT;
				RxFactorCounter = (Factor > 1) ? Factor / 2 : Factor;
			}
			break;

		case START_BIT :  // Asynchronny rezim
			if (RxD == true)
				RxState = WAIT_START; // Nebol to Start bit, ale len falosny zakmit
			else {
				RxState = DATA_BITS;  // bol to platny Start bit
				RxShift = 0;          // nasleduju datove bity
				RxBitCounter = CharLen;
			}
			break;

		case DATA_BITS :  // Oba rezimy
			RxShift >>= 1;
			if (RxD == true)
				RxShift |= 0x80;
			if (--RxBitCounter == 0) {
				if (CharLen < 8)
					RxShift >>= (8 - CharLen);
				if ((CWR & PEN_MASK) == PEN_ENABLED) {
					RxState = (RxState & SYNC_MASK) | PARITY_BIT;
					RxParity = CalculateParity(RxShift);
				}
				else {
					if (SyncMode == true) {
						if (((RxState & SYNC_MASK) == SYNC_CHAR1 && RxShift == SyncChar1)
						|| ((RxState & SYNC_MASK) == SYNC_CHAR2 && RxShift == SyncChar2))
							SynchroDetected(false);
						CharReceived();
					}
					else
						RxState = STOP_BIT;
				}
			}
			break;

		case PARITY_BIT :
			if (RxD != RxParity)
				ParityError = true;
			if (SyncMode == true) {
				if (((RxState & SYNC_MASK) == SYNC_CHAR1 && RxShift == SyncChar1)
				|| ((RxState & SYNC_MASK) == SYNC_CHAR2 && RxShift == SyncChar2))
					SynchroDetected(false);
				CharReceived();
			}
			else
				RxState = STOP_BIT;
			break;

		case STOP_BIT :
			if (RxD == false) {   // neplatny Stop bit
				FrameError = true;  // chyba ukoncenia ramca
				RxBreakCounter++;
				if (RxBreakCounter > 1) { // dve po sebe iduce chybne Stop bity
					RxBreakCounter = 2;     // vratane Start, Data a Parity bitov
					RxBreakState = true;    // znamena prijatie BREAK "slabiky"
					OnBrkDetSet();
					OnBrkDetChange();
				}
			}

			CharReceived();
			RxState = WAIT_START;
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven na vystupe TxRDY (15).
 * Tento vystup je v log.1 iba v pripade, ze je vyrovnavaci register vysielaca
 * prazdny, je povolene vysielanie a vstupny signal _CTS je v log.0.
 *
 * @return log. uroven na vystupe TxRDY
 */
bool ChipUSART8251::PeripheralReadTxR()
{
	return (StatusTxR == true && (Command & TEN_MASK) == TEN_ENABLED && _CTS == false);
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven na vystupe TxEMPTY (18).
 * Tento vystup je v log.1, ak je vyrovnavaci aj posuvny register vysielaca
 * prazdny.
 *
 * @return log. uroven na vystupe TxEMPTY
 */
bool ChipUSART8251::PeripheralReadTxE()
{
	return StatusTxE;
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven na vystupe RxRDY (14).
 * Tento vystup je v log.1, ak bola prijata seriova slabika do vyrovnavacieho
 * registra prijimaca.
 *
 * @return log. uroven na vystupe TxRDY
 */
bool ChipUSART8251::PeripheralReadRxR()
{
	return StatusRxR;
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii nastavit pozadovanu log. uroven na vstupe _DSR (22).
 *
 * @param state log. uroven privedena na vstup _DSR
 */
void ChipUSART8251::PeripheralSetDSR(bool state)
{
	_DSR = state;
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven na vystupe _DTR (24).
 * Tento vystup sa nastavuje povelovym slovom a ma negovanu uroven.
 *
 * @return log. uroven na vystupe _DTR
 */
bool ChipUSART8251::PeripheralReadDTR()
{
	return !(Command & DTR_MASK);
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii nastavit pozadovanu log. uroven na vstupe _CTS (17).
 *
 * @param state log. uroven privedena na vstup _CTS
 */
void ChipUSART8251::PeripheralSetCTS(bool state)
{
	_CTS = state;
//	debug("_CTS=%d", _CTS);
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven na vystupe _RTS (23).
 * Tento vystup sa nastavuje povelovym slovom a ma negovanu uroven.
 *
 * @return log. uroven na vystupe _RTS
 */
bool ChipUSART8251::PeripheralReadRTS()
{
	return !(Command & RTS_MASK);
}
//---------------------------------------------------------------------------
/**
 * Umoznuje perifernemu zariadeniu zmenu log. urovne na vstupe SYNDET (16).
 * Zmena hodnoty sa uplatni, iba ak je nastavena externa synchronizacia
 * v synchronnom rezime. Nabezna hrana impulzu __-- urcuje zaciatok slabiky.
 *
 * @param state log. uroven privedena na vstup SYNDET
 */
void ChipUSART8251::PeripheralSetSynDet(bool state)
{
	if (SyncMode == true && (CWR & ESD_MASK) == ESD_EXTERNAL) {
		if (SynDetState == state)
			return;
		SynDetState = state;
		if (state == true)
			PrepareSyncRx(false, false);
	}
}
//---------------------------------------------------------------------------
/**
 * Umoznuje periferii zistit log. uroven vystupu SYNDET/BRKDET.
 * V synchronnom rezime zodpoveda stavu internej synchronizacie. true znamena,
 * ze boli prijate jedna alebo obidve synchronizacne slabiky.
 * V asynchronnom rezime zodpoveda stavu Break, kedy dva po sebe nasledujuce
 * stop bity (vratane start, data a parity bitov) maju log.0.
 *
 * @return uroven vystupu SYNDET/BRKDET
 */
bool ChipUSART8251::PeripheralReadSynBrk()
{
	if (SyncMode == true && (CWR & ESD_MASK) == ESD_INTERNAL)
		return SynDetState;
	else if (SyncMode == false)
		return RxBreakState;

	return false;
}
//---------------------------------------------------------------------------
void ChipUSART8251::SetByteTransferMode(bool byteTransferMode)
{
	ByteTransferMode = byteTransferMode;
}
//---------------------------------------------------------------------------
void ChipUSART8251::PeripheralWriteByte(BYTE value)
{
	RxShift = value;
	CharReceived();
}
//---------------------------------------------------------------------------
BYTE ChipUSART8251::PeripheralReadByte()
{
	if (StatusTxR == false) {
		if (ByteTransferMode == true) {
			StatusTxR = true;
			OnTxRSet();
			OnTxRChange();
			if (SyncMode)
				PrepareSyncTx();
			else
				PrepareAsyncTx();
		}

		return TxChar;
	}

	return 0;
}
//---------------------------------------------------------------------------
/**
 * Vypocita paritu slabiky 'value'. Parita sa pocita z 'CharLen' bitov tejto
 * hodnoty a vratena hodnota zodpoveda pozadovanej parite. Ma teda hodnotu,
 * ktora sa ma vyslat alebo je ocakavana pri prijme.
 *
 * @param value slabika, pre ktoru ma byt vypocitana parita
 * @return vypocitana parita
 */
bool ChipUSART8251::CalculateParity(BYTE value)
{
	BYTE p = 0;

	for (int ii = 0; ii < CharLen; ii++) {
		p ^= (BYTE)(value & 1);
		value >>= 1;
	}

	if ((CWR & PT_MASK) == PT_EVEN)
		return (p & 1);

	return !(p & 1);
}
//---------------------------------------------------------------------------
/**
 * Pripravi vnutorne premenne vysielaca na vysielanie dalsej slabiky, ak ju CPU
 * zapisalo do vyrovnavacieho registra vysielaca.
 * Inak nastavi vysielac do Idle stavu.
 */
void ChipUSART8251::PrepareAsyncTx()
{
	if (StatusTxR == false) {
		TxState = START_BIT;  // je pripravena slabika na vysielanie
		TxShift = TxChar;
		TxParity = CalculateParity(TxShift);
		TxBitCounter = CharLen;
		StatusTxR = true;
		OnTxRSet();
		OnTxRChange();
	}
	else {  // nie je co vysielat
		StatusTxE = true;
		OnTxESet();
		OnTxEChange();

		TxState = ASYNC_TX_IDLE;
	}

	TxFactorCounter = Factor;
}
//---------------------------------------------------------------------------
/**
 * Na zaklade stavu premennych TxState, CWR a StatusTxR pripravi v synchronnom
 * rezime vysielanie dalsej slabiky. To moze byt slabika pripravena v TxChar
 * alebo niektora zo synchronizacnych slabik. StatusTxE sa nastavi na true, ak
 * uz nie je co vysielat (iba synchro slabiky). Zaroven sa vypocita parita
 * (TxParity) pripravenej slabiky v posuvnom registri a nastavi sa TxBitCounter.
 */
void ChipUSART8251::PrepareSyncTx()
{
	if ((TxState & SYNC_MASK) == SYNC_CHAR1 && (CWR & SCS_MASK) == SCS_2) {
		TxState = SYNC_CHAR2 | DATA_BITS; // vyslanie 2. synchro slabiky
		TxShift = SyncChar2;              // presun do posuvneho registra
	}
	else if (StatusTxR == false) {
		TxState = DATA_BITS;  // je pripravena slabika na vysielanie
		TxShift = TxChar;
		StatusTxR = true;
		OnTxRSet();
		OnTxRChange();
	}
	else {  // nie je co vysielat, budu sa vysielat synchro slabiky
		TxState = SYNC_CHAR1 | DATA_BITS;
		TxShift = SyncChar1;
		StatusTxE = true;
		OnTxESet();
		OnTxEChange();
	}

	TxParity = CalculateParity(TxShift);
	TxBitCounter = CharLen;
}
//---------------------------------------------------------------------------
void ChipUSART8251::PrepareSyncRx(bool hunt, bool sync2)
{
	if (hunt == true) {
		if ((CWR & ESD_MASK) == ESD_INTERNAL) {
			if (sync2 == true) {
				RxState = SYNC_HUNT | SYNC_CHAR2;
				RxShift = SyncChar2;
			}
			else {
				RxState = SYNC_HUNT | SYNC_CHAR1;
				RxShift = SyncChar1;
			}
			RxParity = CalculateParity(RxShift);
			RxBitCounter = CharLen + 1; // vratane parity
		}
		else
			RxState = SYNC_HUNT;

		if (StatusRxR == true) {
			StatusRxR = false;
			OnRxRSet();
			OnRxRChange();
		}
	}
	else {
		if (sync2 == true)
			RxState = DATA_BITS | SYNC_CHAR2;
		else
			RxState = DATA_BITS | SYNC_CHAR1;
		RxBitCounter = CharLen;
		RxShift = 0;
	}
}
//---------------------------------------------------------------------------
void ChipUSART8251::SynchroDetected(bool inHunt)
{
	if ((RxState & SYNC_CHAR1) && (CWR & SCS_MASK) == SCS_2)
		PrepareSyncRx(inHunt, true);
	else {
		PrepareSyncRx(false, false);
		bool oldSynDetState = SynDetState;
		SynDetState = true;
		OnSynDetSet();
		if (oldSynDetState == false)
			OnSynDetChange();
	}
}
//---------------------------------------------------------------------------
void ChipUSART8251::CharReceived()
{
	bool oldStatusRxR = StatusRxR;
	RxChar = RxShift;         // prijata slabika
	if (StatusRxR == true)
		OverrunError = true;    // CPU si neprevzalo predoslu slabiku
	StatusRxR = true;
	OnRxRSet();
	if (oldStatusRxR == false)
		OnRxRChange();
}
//---------------------------------------------------------------------------
