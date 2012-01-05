//---------------------------------------------------------------------------
#include "CommonUtils.h"
#include "TapeBrowser.h"
#include "GPMD85main.h"
//---------------------------------------------------------------------------
TTapeBrowser::TTapeBrowser(TSettings::SetTapeBrowser *set)
{
	blocks = NULL;
	currBlock = NULL;
	currBlockIdx = -1;
	stopBlockIdx = -1;
	totalBlocks = 0;
	data = NULL;
	ifTape = NULL;
	tapeFile = NULL;
	orgTapeFile = NULL;
	settings = set;

	debug("[TapeBrowser] Initializing...");

	buffer = new BYTE[MAX_TAPE_BLOCK_SIZE + 2];
	ProgressBar = new TProgressBar;
	ProgressBar->Max = 0;
	ProgressBar->Position = 0;
	ProgressBar->Active = &playing;

	playing = false;
	tapeChanged = false;
	preparedForSave = false;

	if (set->fileName) {
		char *file = ComposeFilePath(set->fileName);
		SetTapeFileName(file);
		delete [] file;
	}
}
//---------------------------------------------------------------------------
TTapeBrowser::~TTapeBrowser()
{
	debug("[TapeBrowser] Freeing...");

	if (tapeFile)
		delete[] tapeFile;

	if (buffer)
		delete[] buffer;

	FreeAllBlocks();
}
//---------------------------------------------------------------------------
void TTapeBrowser::FreeAllBlocks(bool cleanTmp)
{
	TAPE_BLOCK *blk = blocks, *nextb;
	char *prevFileName = NULL;

	while (blk) {
		if (blk->orgFile) {
			if (prevFileName != blk->orgFile) {
				if (prevFileName != NULL) {
					if (cleanTmp && strcmp(prevFileName + (strlen(prevFileName) - 4), ".tmp") == 0)
						unlink(prevFileName);
					delete [] prevFileName;
				}

				prevFileName = blk->orgFile;
			}
		}

		nextb = blk->next;
		delete blk;
		blk = nextb;
	}

	if (prevFileName != NULL) {
		if (cleanTmp && strcmp(prevFileName + (strlen(prevFileName) - 4), ".tmp") == 0)
			unlink(prevFileName);
		delete [] prevFileName;
	}

	blocks = NULL;
	currBlock = NULL;
	totalBlocks = 0;
	currBlockIdx = -1;
	stopBlockIdx = -1;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SetIfTape(IifTape *ifTape)
{
	this->ifTape = ifTape;
	ifTape->TapeCommand.connect(this, &TTapeBrowser::TapeCommand);
}
//---------------------------------------------------------------------------
bool TTapeBrowser::SetTapeFileName(char *fn)
{
	bool ret = false;

	if (FileExists(fn)) {
		if (tapeFile)
			delete [] tapeFile;

		tapeFile = new char[strlen(fn) + 1];
		strcpy(tapeFile, fn);

		if (playing)
			ActionStop();

		FreeAllBlocks();
		ret = !(PrepareFile(tapeFile, &blocks));
		SelectBlock(0);
		tapeChanged = false;
		preparedForSave = false;
	}

	return ret;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SetNewTape()
{
	if (playing)
		ActionStop();

	FreeAllBlocks();
	tapeChanged = false;
	preparedForSave = false;
}
//---------------------------------------------------------------------------
bool TTapeBrowser::PrepareFile(char *fn, TAPE_BLOCK **blks)
{
	FILE *hf = fopen(fn, "rb");
	if (hf == NULL || fn == NULL)
		return false;

	*blks = NULL;

	TAPE_BLOCK *lBlk, blkTmp;
	DWORD dwPosH, dwPosB;
	bool hdr, oldType, err;
	char *srcFile = new char[strlen(fn) + 1];
	strcpy(srcFile, fn);

	err = true;
	do {
		if (fseek(hf, 0, SEEK_END) != 0)
			break;

		dwFileSize = ftell(hf);
		if (dwFileSize == 0xFFFFFFFF || dwFileSize < 65)
			break;

		fseek(hf, 0, SEEK_SET);

		oldType = false;
		do {
			memset(buffer, 0, 65);
			memset(&blkTmp, 0, sizeof(blkTmp));
			hdr = false;

			dwPosH = ftell(hf);

			// read 2 + 63 + 2
			if ((dwTemp = fread(buffer, sizeof(BYTE), 67, hf)) < 67)
				break;

			wTemp = *(WORD *) buffer;
			if (dwTemp >= 63) {
				hdr = CheckHeader(buffer, &blkTmp);
				if (hdr) {
					oldType = true; // old tape file
					dwPosB = dwPosH + 63;
					wTemp = 0;
				}
				else if (wTemp == 63 && dwTemp == 67) {
					hdr = CheckHeader(buffer + 2, &blkTmp);
					if (hdr) {
						dwPosH += 2;
						dwPosB = dwPosH + 63 + 2;
						wTemp = *(WORD *) (buffer + 65);
					}
				}
			}

			if (hdr) {
				fseek(hf, dwPosB, SEEK_SET);
				dwTemp = ftell(hf);

				if ((DWORD) (blkTmp.wLength + 2) > dwFileSize - dwTemp || (!oldType && blkTmp.wLength + 2 != wTemp)) {
					if (!oldType) {
						blkTmp.bodyLengthError = wTemp - 2;
						fseek(hf, wTemp, SEEK_CUR);
						dwTemp = ftell(hf);
					}
					else {
						blkTmp.bodyLengthError = dwFileSize - dwTemp - 2;
						dwTemp = dwFileSize;
					}
				}
				else {
					dwTemp = fread(buffer, sizeof(BYTE), blkTmp.wLength + 2, hf);
					if (dwTemp < (DWORD) (blkTmp.wLength + 2))
						break;

					blkTmp.bodyLengthError = -1;
					blkTmp.bodyCrcError = !CheckCrc(buffer, blkTmp.wLength + 1, &blkTmp.bBodyCrc);
					dwTemp = ftell(hf);
				}

				blkTmp.dwOffsetHeader = dwPosH;
				blkTmp.dwOffsetBody = dwPosB;
			}
			else {
				if (oldType) {
					if ((dwFileSize - dwPosH) > MAX_TAPE_BLOCK_SIZE)
						break;

					blkTmp.wLength = (WORD) (dwFileSize - dwPosH);
					dwTemp = dwFileSize;
				}
				else {
					if ((DWORD) wTemp > (dwFileSize - dwPosH))
						break;

					blkTmp.wLength = wTemp;
					dwPosH += 2;

					fseek(hf, dwPosH + wTemp, SEEK_SET);
					dwTemp = ftell(hf);
				}

				blkTmp.cType = 0;
				blkTmp.dwOffsetBody = dwPosH;
				blkTmp.bodyLengthError = -1;
			}

			err = false;
			totalBlocks++;

			if (*blks == NULL) {
				*blks = new TAPE_BLOCK;
				lBlk = *blks;
				lBlk->prev = NULL;
			}
			else {
				lBlk->next = new TAPE_BLOCK;
				lBlk->next->prev = lBlk;
				lBlk = lBlk->next;
			}

			memcpy(lBlk, &blkTmp, sizeof(blkTmp) - 2 * sizeof(TapeBlock *) - sizeof(char *));

			lBlk->orgFile = srcFile;
			lBlk->rawFile = false;
			lBlk->next = NULL;

		} while (dwTemp < dwFileSize - 2);

		if (dwTemp != dwFileSize)
			err = true;

	} while (false);

	fclose(hf);
	return err;
}
//---------------------------------------------------------------------------
bool TTapeBrowser::CheckCrc(BYTE *buff, int len, BYTE *goodCrc)
{
	BYTE crc = 0;

	while (len-- > 0)
		crc += *buff++;

	if (goodCrc)
		*goodCrc = crc;

	return (crc == *buff);
}
//---------------------------------------------------------------------------
bool TTapeBrowser::CheckHeader(BYTE *buff, TAPE_BLOCK *blk)
{
	int ii;

	for (ii = 0; ii < 16; ii++)
		if (buff[ii] != 0xFF)
			return false;

	for (ii = 16; ii < 32; ii++)
		if (buff[ii] != 0x00)
			return false;

	for (ii = 32; ii < 48; ii++)
		if (buff[ii] != 0x55)
			return false;

	memcpy(blk, buff + 48, 15);
	blk->headCrcError = !CheckCrc(buff + 48, 14, NULL);

	return true;
}
//---------------------------------------------------------------------------
void TTapeBrowser::ActionPlay()
{
	if (playing || currBlock == NULL)
		return;

	PrepareData(true);
	ifTape->PrepareBlock(data, dataLen, head, settings->flash, true);
	ProgressBar->Position = 0;
	ProgressBar->Max = dataLen;

	playing = true;
}
//---------------------------------------------------------------------------
void TTapeBrowser::ActionStop()
{
	if (!playing)
		return;

	playing = false;

	ifTape->PrepareBlock(NULL, 0, false, false, false);
	ProgressBar->Position = 0;
	head = false;
}
//---------------------------------------------------------------------------
void TTapeBrowser::TapeCommand(int command, bool *result)
{
	switch (command) {
		case CMD_STOP:
			ActionStop();
			break;

		case CMD_NEXT:
			if (head == true && currBlock->cType != 0)
				PrepareData(false);
			else {
				if (currBlockIdx + 1 == totalBlocks)
					SelectBlock(0);
				else
					SelectBlock(currBlockIdx + 1);

				// autostop or rewind
				if (currBlockIdx == 0 ||
					(settings->autoStop == AS_CURSOR && currBlockIdx == stopBlockIdx) ||
					(settings->autoStop == AS_NEXTHEAD && currBlock->cType != 0)) {

					if (settings->autoStop == AS_CURSOR && currBlockIdx == stopBlockIdx)
						stopBlockIdx = -1;

					ActionStop();

					if (result)
						*result = false;
					return;
				}

				PrepareData(true);
			}

			ifTape->PrepareBlock(data, dataLen, head, settings->flash, false);
			ProgressBar->Position = 0;
			ProgressBar->Max = dataLen;
			break;

		case CMD_PROGRESS:
			ProgressBar->Position++;
			break;

		case CMD_MONITORING:
			if (result)
				*result = settings->monitoring;
			return;

		case CMD_PRE_SAVE:
			if (blocks == NULL || blocks->dwOffsetHeader == 0)
				NewTapeFile();
			break;

		case CMD_SAVE:
			SaveTapeBlock();
			break;
	}

	if (result)
		*result = true;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SelectBlock(int idx)
{
	if (idx >= 0) {
		if (currBlock && currBlock->next && idx == (currBlockIdx + 1))
			currBlock = currBlock->next;
		else if (blocks) {
			int i = idx;

			currBlock = blocks;
			while (i && currBlock->next) {
				currBlock = currBlock->next;
				i--;
			}

			idx -= i;
		}
		else {
			currBlock = NULL;
			idx = -1;
		}
	}
	else
		currBlock = NULL;

	currBlockIdx = idx;
}
//---------------------------------------------------------------------------
void TTapeBrowser::PrepareData(bool head)
{
	FILE *hFile = NULL;

	hFile = fopen(currBlock->orgFile, "rb");
	if (hFile == NULL) {
		warning("[TapeBrowser] PrepareData: '%s' not found", currBlock->orgFile);
		ActionStop();
		return;
	}

	this->head = head;
	this->data = buffer;
	if (head && currBlock->cType != 0) {
		fseek(hFile, currBlock->dwOffsetHeader, SEEK_SET);
		dataLen = 63;
	}
	else {
		this->head = false;
		fseek(hFile, currBlock->dwOffsetBody, SEEK_SET);
		dataLen = currBlock->wLength;
		if (currBlock->cType != 0)
			dataLen += (WORD) 2;
	}

	dwTemp = fread(buffer, sizeof(BYTE), dataLen, hFile);
	fclose(hFile);
}
//---------------------------------------------------------------------------
void TTapeBrowser::NewTapeFile()
{
	if (tapeFile)
		delete [] tapeFile;

	tapeFile = new char[64];
	sprintf(tapeFile, "%s/%s.%06x%04x.tmp",
		P_tmpdir, PACKAGE_TARNAME, SDL_GetTicks(), getpid());

	FILE *hDest = fopen(tapeFile, "wb");
	if (hDest == NULL) {
		warning("[TapeBrowser] NewTapeFile: Can't open temp file '%s'", tapeFile);
		delete [] tapeFile;
		tapeFile = NULL;
		return;
	}

	fclose(hDest);
	FreeAllBlocks();
	tapeChanged = false;
	preparedForSave = true;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SaveTapeBlock()
{
	FILE *hDest = fopen(tapeFile, "rb+");
	if (hDest == NULL) {
		warning("[TapeBrowser] SaveTapeBlock: Can't open file '%s'", tapeFile);
		return;
	}

	fseek(hDest, 0, SEEK_END);

	BYTE *buff;
	DWORD len = ifTape->GetSavedBlock(&buff);
	fwrite(buff, sizeof(BYTE), len, hDest);
	fclose(hDest);

	FreeAllBlocks(false);
	PrepareFile(tapeFile, &blocks);

	tapeChanged = true;
}
//---------------------------------------------------------------------------
BYTE TTapeBrowser::SaveTape(char *newFileName, TAPE_BLOCK *blks, bool asPTP)
{
	static BYTE bHeadLeader[48];
	TAPE_BLOCK *blk = blks;
	tmpFileName = NULL;
	BYTE flag = 0xFF;
	WORD wL;

	if (blk == NULL)
		blk = blocks;

	do {
		if (newFileName == NULL)
			break;

		tmpFileName = new char[strlen(newFileName) + 16];
		sprintf(tmpFileName, "%s.%06x%04x.tmp",
			newFileName, SDL_GetTicks(), getpid());

		FILE *hDest = fopen(tmpFileName, "wb");
		if (hDest == NULL) {
			warning("[TapeBrowser] SaveTape: Can't open temp file '%s'", tmpFileName);
			delete [] tmpFileName;
			break;
		}

		if (blk == NULL)
			break;

		flag = 0;
		while (blk) {
			if (blk->cType != 0) {
				wL = 63;
				if (asPTP)
					if (fwrite(&wL, sizeof(BYTE), 2, hDest) != 2) // length of PTP block
						flag = 1;

				if (bHeadLeader[0] != 0xFF || bHeadLeader[16] != 0 || bHeadLeader[32] != 0x55) {
					memset(bHeadLeader, 0xFF, 16);
					memset(bHeadLeader + 16, 0, 16);
					memset(bHeadLeader + 32, 0x55, 16);
				}

				if (fwrite(bHeadLeader, sizeof(BYTE), 48, hDest) != 48) // FF 00 55
					flag = 1;
				if (fwrite(&blk->bNumber, sizeof(BYTE), 15, hDest) != 15) // header
					flag = 1;
				wL = (WORD) (blk->wLength + 2);
			}
			else
				wL = blk->wLength;

			if (ReadFromFile(blk->orgFile, blk->dwOffsetBody, wL, buffer) != wL)
				flag = 1;

			if (blk->bodyCrcErrorFix || (blk->rawFile && blk->cType != 0))
				*(buffer + wL - 1) = blk->bBodyCrc;

			if (asPTP)
				if (fwrite(&wL, sizeof(BYTE), 2, hDest) != 2) // length of PTP block
					flag = 1;

			if (fwrite(buffer, sizeof(BYTE), wL, hDest) != wL) // body
				flag = 1;

			blk = blk->next;
		}

		fclose(hDest);
		unlink(newFileName);
		rename(tmpFileName, newFileName);
		delete [] tmpFileName;

	} while (false);

	return flag;
}
//---------------------------------------------------------------------------
void TTapeBrowser::FillFileList(char ***data, int *items, bool hex)
{
	FreeFileList(data, items);

	if (blocks == NULL || totalBlocks <= 0)
		return;

	*items = totalBlocks;

	TAPE_BLOCK *blk = blocks;
	char **newpt = (char **) malloc(totalBlocks * sizeof(char *)), name[9];

	name[8] = '\0';
	if (newpt != NULL) {
		for (int i = 0; blk && i < totalBlocks; i++) {
			newpt[i] = new char[30];
			newpt[i][29] = (blk->headCrcError || blk->bodyLengthError >= 0 || blk->bodyCrcError) ? '!' : '\0';

			if (blk->cType) {
				memcpy(name, blk->cName, 8);
				sprintf(newpt[i], "%02d/%c %s  ", blk->bNumber, blk->cType, name);
				sprintf(newpt[i] + 15, ((hex) ? "#%04X  " : "%5d  "), blk->wStart);
			}
			else
				sprintf(newpt[i], "%-22s", "\205");

			sprintf(newpt[i] + 22, ((hex) ? "#%04X" : "%5d"), blk->wLength);
			blk = blk->next;
		}

		*data = newpt;
	}
}
//---------------------------------------------------------------------------
void TTapeBrowser::FreeFileList(char ***data, int *items)
{
	if (*data) {
		char **newpt = *data;
		for (int i = 0; i < *items; i++)
			free(newpt[i]);

		free(newpt);
		*data = newpt = NULL;
	}

	*items = 0;
}
//---------------------------------------------------------------------------
