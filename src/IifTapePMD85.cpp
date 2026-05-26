/*	IifTapePMD85.cpp: Derived class for emulation of PMD 85 tape interface
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
#include "IifTapePMD85.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//---------------------------------------------------------------------------
IifTapePMD85::IifTapePMD85(TComputerModel model, TTapeIfType ifType)
	: IifTape(model, ifType), ChipUSART8251()
{
	OnRtsSet.connect(this, &IifTapePMD85::FnOnRtsSet);
	OnTxRChange.connect(this, &IifTapePMD85::FnOnTxRChange);

	tapeTicks = 0;
	tapeClkState = true;
	tapeRxState = TP_RX_IDLE;

	InitTapeTx();

	PeripheralSetRxC(tapeClkState);
	PeripheralSetTxC(tapeClkState);
	PeripheralSetDSR(true);
	PeripheralSetRxD(true);
}
//---------------------------------------------------------------------------
void IifTapePMD85::ResetDevice(int ticks)
{
	ChipReset(false);
	TapeCommand(CMD_STOP, NULL);
}
//---------------------------------------------------------------------------
void IifTapePMD85::WriteToDevice(BYTE port, BYTE value, int ticks)
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
BYTE IifTapePMD85::ReadFromDevice(BYTE port, int ticks)
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
void IifTapePMD85::FnOnRtsSet()
{
	PeripheralSetCTS(PeripheralReadRTS());
}
//---------------------------------------------------------------------------
void IifTapePMD85::FnOnTxRChange()
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
void IifTapePMD85::InitTapeTx()
{
	tapeTxState = TP_TX_FF;
	txByteCounter = 2;
	SetByteTransferMode(false);
}
//---------------------------------------------------------------------------
void IifTapePMD85::TapeClockService123(int ticks, int dur)
{
	tapeTicks += dur;
	while (tapeTicks >= TAPE_HALF_CLOCK) {
		tapeTicks -= TAPE_HALF_CLOCK;

		tapeClkState = !tapeClkState;

		if (tapeRxState != TP_RX_IDLE && tapeRxState != TP_RX_GAP) {
			if (ifType == TIT_V1) {
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

		if (ifType == TIT_V1) {
			PeripheralSetTxC(tapeClkState);
			PeripheralSetRxC(tapeClkState);
		}

		switch (tapeTxState) {
			case TP_TX_WAIT_EB:
				if (--txTickCounter == 0) {
					if (txByteCounter > 2) {
						*((WORD *) buff) = (WORD)(txByteCounter - 2);
						TapeCommand(CMD_SAVE, NULL);
					}
					InitTapeTx();
				}
				return;

			case TP_TX_EXT_BODY:
				if (--txTickCounter == 0) {
					*((WORD *) buff) = (WORD)(txByteCounter - 2);
					bool ret = false;
					TapeCommand(CMD_SAVE, &ret);
					if (!ret)
						InitTapeTx();
					else {
						tapeTxState = TP_TX_WAIT_EB;
						txByteCounter = 2;
						txTickCounter = TC_EB_MAX - TC_EB_GAP;
					}
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
				if (flashLoad && ifType == TIT_V1) {
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
					if (!tapeClkState && ifType == TIT_V1)
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
				if (flashLoad && ifType == TIT_V2) {
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
void IifTapePMD85::TapeClockService23(TPITCounter counter, bool outState)
{
	PeripheralSetTxC(outState);
}
//---------------------------------------------------------------------------
#pragma GCC diagnostic pop
//---------------------------------------------------------------------------
