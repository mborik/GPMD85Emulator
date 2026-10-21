/*	Snapshot.cpp: Structures and handling of emulator snapshot files.
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
//-----------------------------------------------------------------------------
#include "Snapshot.h"
#include "Emulator.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
// Map addresses of words with lengths of individual blocks in the Snap structure
void PrepareBlockLengths(Snap *snap, WORD** blockLengths)
{
	blockLengths[0] = &snap->block0Length;
	blockLengths[1] = &snap->block1Length;
	blockLengths[2] = &snap->block2Length;
	blockLengths[3] = &snap->block3Length;

	if (snap->version > 1) {
		blockLengths[4] = &snap->block4Length;
		blockLengths[5] = &snap->block5Length;
		blockLengths[6] = &snap->block6Length;
		blockLengths[7] = &snap->block7Length;
		blockLengths[8] = &snap->block8Length;
		blockLengths[9] = &snap->block9Length;
		blockLengths[10] = &snap->block10Length;
		blockLengths[11] = &snap->block11Length;
		blockLengths[12] = &snap->block12Length;
		blockLengths[13] = &snap->block13Length;
		blockLengths[14] = &snap->block14Length;
		blockLengths[15] = &snap->block15Length;
	}
}
//---------------------------------------------------------------------------
void TEmulator::SaveSnapshot(const char *fileName) {
	if (!cpu || !memory)
		return;

	// Remember the current memory mapping/paging
	bool allRAM = memory->IsAllRAM();
	bool remapped = memory->IsRemapped();
	BYTE page = memory->GetPage();

	Snap snap;
	BYTE *src = nullptr;
	BYTE *romBuf = nullptr;
	BYTE *ramBuf = nullptr;

	memset(&snap, 0, sizeof(Snap));
	memcpy(snap.signature, SNAP_SIGNATURE, SNAP_SIGNATURE_LEN);

	// set version
	snap.version = 2;
	snap.blocksOffset = SNAP_HEADER_V2_LEN;
	if (Settings->Snapshot->saveOldVersion ||
		(mif85 == nullptr && !memory->IsMem256())) {

		snap.version = 1;
		snap.blocksOffset = SNAP_HEADER_V1_LEN;
	}

	snap.model = static_cast<BYTE>(model);

	// prepare the state of the processor and individual chips
	cpu->GetChipState(&snap.interrupt);
	if (systemPIO) {
		systemPIO->GetChipState(&snap.systemPioCwr, false);
		snap.systemPioA = snap.systemPioCwr; // copy PioA state from next byte
	}
	if (ifGpio)
		ifGpio->GetChipState(&snap.gpioCwr, false);
	if (ifTimer)
		ifTimer->GetChipState(&snap.timer0Cwr, 9);
	if (ifTape && ifTape->GetModel() != CM_MATO)
		((IifTapePMD85 *) ifTape)->GetChipState(&snap.usartCwr, false);

	if (snap.version > 1) {
		snap.videoIntVGA = 0xff;
		snap.memMap = (memory->IsMem256()) ? page : 0xff;

		if (mif85) {
			snap.mif85Int = static_cast<BYTE>(mif85->InterruptEnabled());
			mif85->GetDeviceState(snap.mif85Regs);
		}
		else
			snap.mif85Int = 0xff;

		// initialize unused timer states
		memset(&snap.musicaTimer0Cwr, 0xff, 9);
	}

	TSnapshotError err = SNAP_OK;

	do {
		// preparation of individual blocks
		int ramBlocks = (snap.version > 1 && memory->IsMem256()) ? 16 : 4;

		// Addresses of block length items in the Snap structure
		WORD* blockLengthsPtrs[16];
		PrepareBlockLengths(&snap, blockLengthsPtrs);

		int len;
		src = new BYTE[SNAP_BLOCK_LEN];
		romBuf = new BYTE[SNAP_BLOCK_LEN];
		ramBuf = new BYTE[SNAP_BLOCK_LEN * ramBlocks];

		memory->SetAllRAM(false);
		memory->SetRemapped(false);

		if (Settings->Snapshot->saveWithMonitor) {
			memory->GetMem(src, (model == CM_V3) ? 0xE000 : 0x8000, monitorLength);
			if (Settings->Snapshot->saveCompressed)
				len = PackBlock(romBuf, src, monitorLength);
			else {
				len = monitorLength;
				memcpy(romBuf, src, len);
			}

			snap.romLength = static_cast<WORD>(
				len | ((len == monitorLength) ? 0x8000 : 0)
			);
		}

		memory->SetAllRAM(true);

		for (int i = 0; i < ramBlocks; i++) {
			// skip block 8000h-BFFFh on non-AllRAM models
			if (i == 2 && !(model == CM_V2A || model == CM_V3 || model == CM_C2717))
				continue;

			memory->SetPage(static_cast<BYTE>(((i & 3) << 4) | i));
			memory->GetMem(src, ((i & 3) * SNAP_BLOCK_LEN), SNAP_BLOCK_LEN);
			if (Settings->Snapshot->saveCompressed)
				len = PackBlock(ramBuf + (i * SNAP_BLOCK_LEN), src, SNAP_BLOCK_LEN);
			else {
				len = SNAP_BLOCK_LEN;
				memcpy(ramBuf + (i * SNAP_BLOCK_LEN), src, len);
			}

			*blockLengthsPtrs[i] = static_cast<WORD>(len);
		}

		// restore the original memory mapping/paging settings
		memory->SetAllRAM(allRAM);
		memory->SetRemapped(remapped);
		memory->SetPage(page);

		// write the snapshot header
		len = static_cast<int>(snap.blocksOffset);
		int offset = WriteToFile(fileName, 0, len, reinterpret_cast<BYTE *>(&snap), true);
		if (offset != len) {
			err = SNAP_WRITE_ERROR;
			break;
		}

		// ROM (Monitor)
		if (Settings->Snapshot->saveWithMonitor) {
			len = static_cast<int>(snap.romLength & 0x7FFF);
			if (WriteToFile(fileName, offset, len, romBuf, false) != len) {
				err = SNAP_WRITE_ERROR;
				break;
			}

			offset += len;
		}

		for (int i = 0; i < ramBlocks; i++, offset += len) {
			// skip block 8000h-BFFFh on non-AllRAM models
			if (i == 2 && !(model == CM_V2A || model == CM_V3 || model == CM_C2717))
				continue;

			len = static_cast<int>(*blockLengthsPtrs[i]);
			if (WriteToFile(fileName, offset, len, ramBuf + (i * SNAP_BLOCK_LEN), false) != len) {
				err = SNAP_WRITE_ERROR;
				break;
			}
		}
	} while (false);

	if (src)
		delete [] src;
	if (romBuf)
		delete [] romBuf;
	if (ramBuf)
		delete [] ramBuf;

	if (err != SNAP_OK)
		GUI->MessageBox(SNAPSHOT_ERR_MSG[err]);
	else {
		if (Settings->Snapshot->fileName)
			delete [] Settings->Snapshot->fileName;
		Settings->Snapshot->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->Snapshot->fileName, fileName);
	}
}
//---------------------------------------------------------------------------
bool TEmulator::LoadSnapshot(const char *fileName) {
	BYTE *src = nullptr;
	BYTE *dest = nullptr;
	Snap *snap = new Snap;

	TSnapshotError err = SNAP_OK;
	TComputerModel oldModel = model;
	bool oldMem256k = Settings->CurrentModel->ramExpansion256k;
	bool oldVideoIntVGA = Settings->Screen->videoInterrupt;
	bool oldMif85 = Settings->Sound->ifMIF85;

	do {
		// read Snapshot header signature
		if (ReadFromFile(fileName, 0, 7, reinterpret_cast<BYTE *>(snap)) != 7) {
			err = SNAP_READ_ERROR;
			break;
		}

		// signature check
		if (memcmp(snap->signature, SNAP_SIGNATURE, SNAP_SIGNATURE_LEN) != 0) {
			err = SNAP_BAD_HEADER;
			break;
		}

		// version check
		if (snap->version == 0 || snap->version > SNAP_VERSION_MAX) {
			err = SNAP_UNKNOWN_VERSION;
			break;
		}

		// header size check
		int headerLen = static_cast<int>(snap->blocksOffset);
		if (headerLen < SNAP_MIN_HEADER_LEN || headerLen > SNAP_MAX_HEADER_LEN) {
			err = SNAP_BAD_HEADER;
			break;
		}

		// model check
		model = static_cast<TComputerModel>(snap->model);
		if (model > CM_LAST) {
			err = SNAP_UNKNOWN_MODEL;
			break;
		}

		// read the entire Snapshot header
		int offset = ReadFromFile(fileName, 0, headerLen, reinterpret_cast<BYTE *>(snap));
		if (offset != headerLen) {
			err = SNAP_READ_ERROR;
			break;
		}

		// block size check
		if ((snap->romLength & 0x7FFF) > SNAP_BLOCK_LEN ||
		     snap->block0Length > SNAP_BLOCK_LEN ||
		     snap->block1Length > SNAP_BLOCK_LEN ||
		     snap->block2Length > SNAP_BLOCK_LEN ||
		     snap->block3Length > SNAP_BLOCK_LEN
		) {
			err = SNAP_BAD_HEADER;
			break;
		}

		src = new BYTE[SNAP_BLOCK_LEN];
		dest = new BYTE[SNAP_BLOCK_LEN];

		// process the Monitor ROM block
		int len, lenX = static_cast<int>(snap->romLength);
		if (lenX > 0) {
			len = lenX & 0x7FFF; // lenX.b15 = 1, if the ROM is uncompressed
			if (ReadFromFile(fileName, offset, len, src) != len) {
				err = SNAP_READ_ERROR;
				break;
			}

			offset += len;
			lenX = UnpackBlock(dest, (lenX & 0x8000) ? len : SNAP_BLOCK_LEN, src, len);
			if (lenX < 0) {
				err = SNAP_BAD_MEM_BLOCK;
				break;
			}
		}

		// apply emulator settings for the current model
		for (int i = 0; i < Settings->modelsCount; i++) {
			if (Settings->AllModels[i]->type == model) {
				Settings->CurrentModel = Settings->AllModels[i];
				break;
			}
		}
		if (snap->version > 1) {
			if (model == CM_V2A || model == CM_V3)
				Settings->CurrentModel->ramExpansion256k = (snap->memMap != 0xff);
			Settings->Screen->videoInterrupt =
				(model <= CM_V3) ? (snap->videoIntVGA != 0xff) : false;
			if (snap->mif85Int != 0xff)
				Settings->Sound->ifMIF85 = true;
		}
		else {
			Settings->CurrentModel->ramExpansion256k = false;
			Settings->Screen->videoInterrupt = false;
		}

		// set the computer model, cpu state
		SetComputerModel((lenX > 0), lenX, dest);

		if (!memory || !cpu) {
			err = SNAP_INTERNAL_ERROR;
			break;
		}

		Debugger->SetParams(cpu, memory, model);

		if (cpu)
			cpu->SetChipState(&snap->interrupt);
		if (systemPIO) {
			systemPIO->ResetDevice(0);
			systemPIO->WriteToDevice(SYSTEM_REG_CWR, snap->systemPioCwr, 0);
			systemPIO->WriteToDevice(SYSTEM_REG_C, snap->systemPioC, 0);
			systemPIO->WriteToDevice(SYSTEM_REG_A, snap->systemPioA, 0);
			systemPIO->ReadFromDevice(SYSTEM_REG_B, 0); // set for C2717 repagination
		}
		if (ifGpio)
			ifGpio->SetChipState(&snap->gpioCwr);
		if (ifTimer)
			ifTimer->SetChipState(&snap->timer0Cwr);
		if (ifTape) {
			if (ifTape->GetModel() != CM_MATO)
				((IifTapePMD85 *) ifTape)->SetChipState(&snap->usartCwr);
		}
		if (snap->version > 1) {
			if (mif85)
				mif85->SetDeviceState(snap->mif85Regs, snap->mif85Int);
			if ((model == CM_V2A || model == CM_V3) && snap->memMap != 0xff && memory->IsMem256())
				memory->SetPage(snap->memMap);
/*			// not implemented yet
			if ((model <= CM_V3) && snap->videoIntVGA != 0xff)
				video->SetIntStatus(snap->videoIntVGA);
*/
		}

		if (model != CM_V3)
			memory->ResetOn();

		// preparation of individual blocks
		int ramBlocks = (snap->version > 1 && memory->IsMem256()) ? 16 : 4;

		// block length mapper within Snap structure
		WORD *blockLengthsPtrs[16];
		PrepareBlockLengths(snap, blockLengthsPtrs);

		// remember the current memory mapping/paging
		bool allRAM = memory->IsAllRAM();
		bool remapped = memory->IsRemapped();
		BYTE page = memory->GetPage();

		memory->SetAllRAM(true);
		memory->SetRemapped(false);

		for (int i = 0; i < ramBlocks; i++) {
			// skip block 8000h-BFFFh on non-AllRAM models
			if (i == 2 && !(model == CM_V2A || model == CM_V3 || model == CM_C2717))
				continue;

			len = (WORD) (*blockLengthsPtrs[i]);
			if (len == 0)
				continue;

			if (ReadFromFile(fileName, offset, len, src) != len) {
				err = SNAP_READ_ERROR;
				break;
			}

			offset += len;
			len = UnpackBlock(dest, SNAP_BLOCK_LEN, src, len);
			if (len < 0 || len != SNAP_BLOCK_LEN) {
				err = SNAP_BAD_MEM_BLOCK;
				break;
			}

			memory->SetPage(static_cast<BYTE>(((i & 3) << 4) | i));
			memory->PutMem(((i & 3) * SNAP_BLOCK_LEN), dest, SNAP_BLOCK_LEN);
		}

		// restore the original memory mapping/paging settings
		memory->SetAllRAM(allRAM);
		memory->SetRemapped(remapped);
		memory->SetPage(page);

	} while (false);

	if (err != SNAP_OK) {
		GUI->MessageBox(SNAPSHOT_ERR_MSG[err]);

		for (int i = 0; i < Settings->modelsCount; i++) {
			if (Settings->AllModels[i]->type == oldModel) {
				Settings->CurrentModel = Settings->AllModels[i];
				break;
			}
		}

		model = oldModel;
		SetComputerModel();
		Debugger->SetParams(cpu, memory, model);
	}
	else {
		if (Settings->Snapshot->fileName)
			delete [] Settings->Snapshot->fileName;
		Settings->Snapshot->fileName = new char[(strlen(fileName) + 1)];
		strcpy(Settings->Snapshot->fileName, fileName);
	}

	if (snap)
		delete snap;
	if (src)
		delete [] src;
	if (dest)
		delete [] dest;

	return (err == SNAP_OK);
}
//---------------------------------------------------------------------------
