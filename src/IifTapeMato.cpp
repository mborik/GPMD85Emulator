/*	IifTapeMato.cpp: Derived class for emulation of Mato tape interface
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
#include "IifTapeMato.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
//---------------------------------------------------------------------------
IifTapeMato::IifTapeMato(SystemPIO *systemPIO) : IifTape(CM_MATO, TIT_MATO)
{
	this->systemPIO = systemPIO;

	tapeTicks = 0;
	tapeRxState = TP_RX_IDLE;
	counter = -1;

	tapeTxState = TP_TX_WAIT_HEAD;
	txByteCounter = 0;

	systemPIO->SetMatoTapeIn(false);
	bit = false;
}
//---------------------------------------------------------------------------
void IifTapeMato::TapeClockService(int ticks, int dur)
{
	bool ret = false;

	tapeTicks += dur;
	if (tapeRxState != TP_RX_IDLE && tapeRxState != TP_RX_GAP) {
		TapeCommand(CMD_MONITORING, &ret);
		if (ret && !flashLoad) {
			PrepareSample(CHNL_TAPE, bit, ticks);
		}
	}

	switch (tapeTxState) {
		case TP_TX_HEAD :
			TapeCommand(CMD_PRE_SAVE, &ret);
			if (!ret) {
				tapeTxState = TP_TX_WAIT_HEAD;
				txByteCounter = 0;
			}
			else {
				txByteCounter = 65;
				WORD len = (WORD) (*((WORD *) (buff + 54)) + 2);
				txBodyEnd = txByteCounter + len + 2;
				tapeTxState = TP_TX_WAIT_BODY;
			}
			break;

		case TP_TX_BODY :
			txByteCounter = txBodyEnd;
			TapeCommand(CMD_SAVE, NULL);
			tapeTxState = TP_TX_WAIT_HEAD;
			txByteCounter = 0;
			break;
	}

	switch (tapeRxState) {
		case TP_RX_IDLE :
			systemPIO->SetMatoTapeIn(false);
			bit = false;
			tapeTicks = 0;
			counter = -1;
			break;

		case TP_RX_LEADER :
		case TP_RX_TAIL :
		case TP_RX_BLOCK_END :
			if (counter == -1) {
				tapeRxStateNext = tapeRxState;
				tapeRxState = TP_RX_LOG_0;
				counter = (tapeRxStateNext == TP_RX_LEADER) ? 510 : 128;
//				debug("leader: %d, %d", tapeRxStateNext, counter);
			}
			else if (--counter == 0) {
				if (tapeRxState == TP_RX_LEADER) {
					tapeRxState = TP_RX_BYTE;
					byte = 0x55;
					counter = -2;
//					debug("leader: 0x55");
//					debug("block: %d", (int) dataLen);
				}
				else if (tapeRxState == TP_RX_BLOCK_END) {
					TapeCommand(CMD_NEXT, NULL);
					tapeRxState = TP_RX_IDLE;
				}
				else {
					tapeRxState = TP_RX_LOG_1;
					tapeRxStateNext = TP_RX_BLOCK;
				}
			}
			else
				tapeRxState = TP_RX_LOG_0;
			break;

		case TP_RX_PULSE_0 :
			systemPIO->SetMatoTapeIn(false);
			bit = false;
			if (tapeTicks > tapeTicksNext)
				tapeRxState = tapeRxStateNext;
			break;

		case TP_RX_PULSE_1 :
			systemPIO->SetMatoTapeIn(true);
			bit = true;
			if (tapeTicks > tapeTicksNext) {
				tapeRxState = TP_RX_PULSE_0;
				tapeTicks = 0;
				tapeTicksNext = TC_PULSE_LONG;
			}
			break;

		case TP_RX_LOG_0 :
			tapeRxState = TP_RX_PULSE_1;
			tapeTicks = 0;
			tapeTicksNext = TC_PULSE_SHORT;
			break;

		case TP_RX_LOG_1 :
			tapeRxState = TP_RX_PULSE_1;
			tapeTicks = 0;
			tapeTicksNext = TC_PULSE_SHORT + TC_PULSE_LONG;
			break;

		case TP_RX_BYTE :
			if (counter == -2) {
				counter = 1 + 8;
				tapeRxState = TP_RX_LOG_1;
				tapeRxStateNext = TP_RX_BYTE;
			}
			else if (--counter == 0) {
				tapeRxState = TP_RX_BLOCK;
			}
			else {
				tapeRxState = (byte & 1) ? TP_RX_LOG_1 : TP_RX_LOG_0;
				byte >>= 1;
			}
			break;

		case TP_RX_BLOCK :
			if (dataLen == 0) {
				if (head) {
					counter = -1;
					data = NULL;
					TapeCommand(CMD_NEXT, NULL);
					tapeRxState = TP_RX_TAIL;
//					debug("block: %d", (int) dataLen);
				}
				else {
					counter = -1;
					data = NULL;
					tapeRxState = TP_RX_BLOCK_END;
				}
			}
			else {
				tapeRxState = TP_RX_BYTE;
				counter = -2;
				byte = *data++;
				dataLen--;
			}
			break;
	}
}
//---------------------------------------------------------------------------
#pragma GCC diagnostic pop
//---------------------------------------------------------------------------
