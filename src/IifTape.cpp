//---------------------------------------------------------------------------
#include "IifTape.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
IifTape::IifTape(TComputerModel model) : ChipUSART8251()
{
	OnRtsSet.connect(this, &IifTape::FnOnRtsSet);
	OnTxRChange.connect(this, &IifTape::FnOnTxRChange);

	this->model = model;
	TapeCommand.disconnect_all();
	PrepareSample.disconnect_all();

	tapeTicks = 0;
	tapeClkState = true;
	tapeRxState = TP_RX_IDLE;

	InitTapeTx();

	PeripheralSetRxC(tapeClkState);
	PeripheralSetTxC(tapeClkState);
	PeripheralSetDSR(true);
	PeripheralSetRxD(true);

	data = NULL;
	dataLen = 0;
	head = true;
	flashLoad = false;
}
//---------------------------------------------------------------------------
int IifTape::GetTapeIcon()
{
	if (tapeRxState != TP_RX_IDLE)
		return 9;
	if (tapeTxState != TP_TX_FF)
		return 10;

	return 0;
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri jeho resete.
 */
void IifTape::resetDevice(int ticks)
{
	ChipReset(false);
	TapeCommand(CMD_STOP, NULL);
}
//---------------------------------------------------------------------------
void IifTape::writeToDevice(BYTE port, BYTE value, int ticks)
{
	if (model == CM_ALFA || model == CM_ALFA2) {
		switch (port & IIF_TAPE_REG_MASK_A) {
			case IIF_TAPE_REG_DATA_A:
				CpuWrite(UR_DATA, value);
				break;

			case IIF_TAPE_REG_CWR_A:
				CpuWrite(UR_CTRL, value);
				break;
		}
	}
	else {
		switch (port & IIF_TAPE_REG_MASK) {
			case IIF_TAPE_REG_DATA:
				CpuWrite(UR_DATA, value);
				break;

			case IIF_TAPE_REG_CWR:
				CpuWrite(UR_CTRL, value);
				break;
		}
	}
}
//---------------------------------------------------------------------------
BYTE IifTape::readFromDevice(BYTE port, int ticks)
{
	BYTE retval;

	if (model == CM_ALFA || model == CM_ALFA2) {
		switch (port & IIF_TAPE_REG_MASK_A) {
			case IIF_TAPE_REG_DATA_A:
				retval = CpuRead(UR_DATA);
				break;

			case IIF_TAPE_REG_STAT_A:
				retval = CpuRead(UR_STATUS);
				break;

			default:
				retval = 0xFF;
				break;
		}
	}
	else {
		switch (port & IIF_TAPE_REG_MASK) {
			case IIF_TAPE_REG_DATA:
				retval = CpuRead(UR_DATA);
				break;

			case IIF_TAPE_REG_STAT:
				retval = CpuRead(UR_STATUS);
				break;

			default:
				retval = 0xFF;
				break;
		}
	}

	return retval;
}
//---------------------------------------------------------------------------
void IifTape::FnOnRtsSet()
{
	PeripheralSetCTS(PeripheralReadRTS());
}
//---------------------------------------------------------------------------
void IifTape::FnOnTxRChange()
{
	if (PeripheralReadTxR() == false) {
		if (tapeRxState != TP_RX_IDLE) {
			InitTapeTx();
			return;
		}

		bool ok = true;
		BYTE val = PeripheralReadByte();
//		debug("val=%u", val);

		buff[txByteCounter] = val;
		switch (tapeTxState) {
			case TP_TX_FF :
				if (val == 0xFF) {
					if (++txByteCounter == 18)
						tapeTxState = TP_TX_00;
				}
				else
					ok = false;
				break;

			case TP_TX_00 :
				if (val == 0x00) {
					if (++txByteCounter == 34)
						tapeTxState = TP_TX_55;
				}
				else
					ok = false;
				break;

			case TP_TX_55 :
				if (val == 0x55) {
					if (++txByteCounter == 50) {
						crc = 0;
						tapeTxState = TP_TX_HEAD;
					}
				}
				else
					ok = false;
				break;

			case TP_TX_HEAD :
				if (++txByteCounter == 65) {
//					debug("<- CRC HEAD -> %02X", crc);
//					if (crc != val)
//						ok = false;
//					else
//					{
						TapeCommand(CMD_PRE_SAVE, NULL);
						*((WORD *)buff) = 63;
						WORD len = (WORD)(*((WORD *)(buff + 54)) + 2);
						*((WORD *)(buff + txByteCounter)) = len;
						txByteCounter += 2;
						txBodyEnd = txByteCounter + len;
						crc = 0;
						tapeTxState = TP_TX_BODY;
						SetByteTransferMode(true);
//					}
				}
				else
					crc += val;
				break;

			case TP_TX_BODY :
				if (++txByteCounter == txBodyEnd) {
//					debug("<- CRC BODY -> %02X", crc);
//					if (crc == val)
//					{
						TapeCommand(CMD_SAVE, NULL);
						tapeTxState = TP_TX_WAIT_EB;
						txByteCounter = 2;
						txTickCounter = TC_EB_MAX;
//					}
//					else
//						ok = false;
				}
				else
					crc += val;
				break;

			case TP_TX_WAIT_EB :
				tapeTxState = TP_TX_EXT_BODY;
			case TP_TX_EXT_BODY :
				++txByteCounter;
				txTickCounter = TC_EB_GAP;
				break;
		}

		if (ok == false)
			InitTapeTx();
	}
}
//---------------------------------------------------------------------------
void IifTape::InitTapeTx()
{
	tapeTxState = TP_TX_FF;
	txByteCounter = 2;
	SetByteTransferMode(false);
}
//---------------------------------------------------------------------------
bool IifTape::GetTapeByte(BYTE *byte)
{
	if (data && tapeRxState == TP_RX_DATA) {
		*byte = *data++;
		TapeCommand(CMD_PROGRESS, NULL);
		if (--dataLen == 0) {
			tapeRxState = TP_RX_IDLE;
			data = NULL;
			TapeCommand(CMD_NEXT, NULL);
		}
		else
			rxTickCounter = TC_NO_BYTE;

		return false;
	}

	return true;
}
//---------------------------------------------------------------------------
bool IifTape::GetTapeBlock(BYTE **point, WORD *len)
{
//	debug("data=%08X, dataLen=%u, tapeRxState=%d", data, dataLen, tapeRxState);
	if (data && ((tapeRxState == TP_RX_DATA && model != CM_V1 && model != CM_ALFA)
			|| (tapeRxState == TP_RX_LEADER && (model == CM_V1 || model == CM_ALFA))))
	{
		*point = data;
		*len = dataLen;
		return true;
	}

	*point = NULL;
	return false;
}
//---------------------------------------------------------------------------
void IifTape::AcceptTapeBlock(WORD len)
{
	if (data && ((tapeRxState == TP_RX_DATA && model != CM_V1 && model != CM_ALFA)
	|| (tapeRxState == TP_RX_LEADER && (model == CM_V1 || model == CM_ALFA)))) {
		data += len;
		dataLen -= len;
		if (dataLen == 0) {
			tapeRxState = TP_RX_IDLE;
			data = NULL;
			TapeCommand(CMD_NEXT, NULL);
		}
		else
			rxTickCounter = TC_NO_BYTE;
	}
}
//---------------------------------------------------------------------------
void IifTape::TapeClockService123(int ticks, int dur)
{
	tapeTicks += dur;
	while (tapeTicks >= TAPE_HALF_CLOCK) {
		tapeTicks -= TAPE_HALF_CLOCK;

		tapeClkState = !tapeClkState;

		if (tapeRxState != TP_RX_IDLE && tapeRxState != TP_RX_GAP) {
			if (model == CM_V1 || model == CM_ALFA) {
				if (!flashLoad)
					PeripheralSetRxD(bit);
			}
			else
				PeripheralSetDSR(tapeClkState ^ bit);

			bool ret = false;
			TapeCommand(CMD_MONITORING, &ret);
			if (ret && !flashLoad)
				PrepareSample(CHNL_TAPE, tapeClkState ^ bit, ticks - tapeTicks);
		}

		if (model == CM_V1 || model == CM_ALFA) {
			PeripheralSetTxC(tapeClkState);
			PeripheralSetRxC(tapeClkState);
		}

		switch (tapeTxState) {
			case TP_TX_WAIT_EB :
				if (--txTickCounter == 0) {
					if (txByteCounter > 2) {
						*((WORD *)buff) = (WORD)(txByteCounter - 2);
						TapeCommand(CMD_SAVE, NULL);
					}
					InitTapeTx();
				}
				return;

			case TP_TX_EXT_BODY :
				if (--txTickCounter == 0) {
					*((WORD *)buff) = (WORD)(txByteCounter - 2);
					TapeCommand(CMD_SAVE, NULL);

					tapeTxState = TP_TX_WAIT_EB;
					txByteCounter = 2;
					txTickCounter = TC_EB_MAX - TC_EB_GAP;
				}
				return;
		}

		if (data == NULL)
			return;

		switch (tapeRxState) {
			case TP_RX_IDLE :
				break;

			case TP_RX_GAP :
				if (--rxTickCounter == 0) {
					tapeRxState = TP_RX_LEADER;
					rxTickCounter = TC_HEAD_LEADER;
					PeripheralSetDSR(true);
				}
				break;

			case TP_RX_LEADER :
				if (flashLoad && (model == CM_V1 || model == CM_ALFA)) {
					rxTickCounter--;
					if (data != NULL && (PeripheralReadRxR() == false || rxTickCounter == 0)) {
						rxTickCounter = TC_BYTE_CLOCK;
						PeripheralWriteByte(*data++);
						TapeCommand(CMD_PROGRESS, NULL);
						if (--dataLen == 0) {
							tapeRxState = TP_RX_IDLE;
							data = NULL;
							TapeCommand(CMD_NEXT, NULL);
						}
					}
					return;
				}

				if (--rxTickCounter == 0) {
					tapeRxState = TP_RX_START;
					rxTickCounter = TC_START;
					if (!tapeClkState && (model == CM_V1 || model == CM_ALFA))
						rxTickCounter++;
					bit = false;
				}
				break;

			case TP_RX_START :
				if (--rxTickCounter == 0) {
					tapeRxState = TP_RX_DATA;
					rxTickCounter = TC_DATA;
					if (!flashLoad) {
						byte = *data++;
						bit = (byte & 1);
					}
				}
				break;

			case TP_RX_DATA :
				if (flashLoad && model != CM_V1 && model != CM_ALFA) {
					if (--rxTickCounter == 0) {
						tapeRxState = TP_RX_START;
						rxTickCounter = TC_START;
						bit = false;
						flashLoad = false;
					}
					return;
				}

				if (--rxTickCounter == 0) {
					tapeRxState = TP_RX_STOP;
					rxTickCounter = TC_STOP;
					bit = true;
				}
				else if ((rxTickCounter & 1) == 0) {
					byte >>= 1;
					bit = (byte & 1);
				}
				break;

			case TP_RX_STOP :
				if (--rxTickCounter == 0) {
					TapeCommand(CMD_PROGRESS, NULL);
					if (--dataLen == 0) {
						if (head) {
							tapeRxState = TP_RX_IDLE;
							data = NULL;
							TapeCommand(CMD_NEXT, NULL);
						}
						else {
							tapeRxState = TP_RX_TAIL;
							rxTickCounter = TC_STOP_TAIL;
							PeripheralSetDSR(true);
						}
					}
					else {
						tapeRxState = TP_RX_START;
						rxTickCounter = TC_START;
						bit = false;
					}
				}
				break;

			case TP_RX_TAIL :
				if (--rxTickCounter == 0) {
					tapeRxState = TP_RX_IDLE;
					data = NULL;
					TapeCommand(CMD_NEXT, NULL);
				}
				break;
		}
	}
}
//---------------------------------------------------------------------------
void IifTape::TapeClockService23(TPITCounter counter, bool outState)
{
	PeripheralSetTxC(outState);
}
//---------------------------------------------------------------------------
void IifTape::PrepareBlock(BYTE *data, WORD dataLen, bool head, bool flash, bool onPlay)
{
	// preventivne najprv nastav IDLE stav
	tapeRxState = TP_RX_IDLE;

	this->data = data;
	this->dataLen = dataLen;
	this->head = head;

	flashLoad = false;
	if (data == NULL || dataLen == 0)
		tapeRxState = TP_RX_IDLE;
	else {
		if (head) {
			if (onPlay) {
				tapeRxState = TP_RX_LEADER;
				rxTickCounter = TC_HEAD_LEADER;
			}
			else {
				tapeRxState = TP_RX_GAP;
				rxTickCounter = TC_GAP_SIZE;
			}
		}
		else {
			tapeRxState = TP_RX_LEADER;
			rxTickCounter = TC_BODY_LEADER;
			flashLoad = flash;
			if (flashLoad && model != CM_V1 && model != CM_ALFA) {
				tapeRxState = TP_RX_DATA;
				rxTickCounter = TC_NO_BYTE;
			}
		}
		bit = true;
	}
}
//---------------------------------------------------------------------------
int IifTape::GetSavedBlock(BYTE **pbuf)
{
	*pbuf = buff;
	return txByteCounter;
}
//---------------------------------------------------------------------------
