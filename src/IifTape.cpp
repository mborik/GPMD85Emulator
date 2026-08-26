/*	IifTape.cpp: Core of emulation of tape interface
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

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
		TapeCommand(CMD_PROGRESS, nullptr);
		if (--dataLen == 0) {
			tapeRxState = TP_RX_IDLE;
			data = NULL;
			TapeCommand(CMD_NEXT, nullptr);
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
			TapeCommand(CMD_NEXT, nullptr);
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
