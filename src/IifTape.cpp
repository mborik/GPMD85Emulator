/*	IifTape.cpp: Core of emulation of tape interface
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
#include "IifTape.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
IifTape::IifTape(TComputerModel model, TTapeIfType ifType)
{
	this->model = model;
	this->ifType = ifType;

	TapeCommand.disconnect_all();
	PrepareSample.disconnect_all();

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
	if (tapeTxState != TP_TX_FF && tapeTxState != TP_TX_WAIT_HEAD)
		return 10;

	return 0;
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
	if (data && (
		(tapeRxState == TP_RX_DATA && ifType == TIT_V2) ||
		(tapeRxState == TP_RX_LEADER && ifType == TIT_V1) ||
		(tapeRxState == TP_RX_PULSE_1 && ifType == TIT_MATO)
	))
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
	if (data && (
		(tapeRxState == TP_RX_DATA && ifType == TIT_V2) ||
		(tapeRxState == TP_RX_LEADER && ifType == TIT_V1) ||
		(tapeRxState == TP_RX_PULSE_1 && ifType == TIT_MATO)
	)) {
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
void IifTape::PrepareBlock(BYTE *data, WORD dataLen, bool head, bool flash, bool onPlay)
{
	// at first, set IDLE state (preventive)
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
			if (flashLoad && ifType == TIT_V2) {
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
