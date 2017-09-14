//---------------------------------------------------------------------------
#include "CommonUtils.h"
#include "TapeBrowser.h"
#include "GPMD85main.h"
//---------------------------------------------------------------------------
#define isTemp(x) (strcmp(x + (strlen(x) - 4), ".tmp") == 0)
//---------------------------------------------------------------------------
TTapeBrowser *TapeBrowser;
//---------------------------------------------------------------------------
TTapeBrowser::TTapeBrowser()
{
	blocks = NULL;
	currBlock = NULL;
	currBlockIdx = -1;
	stopBlockIdx = -1;
	totalBlocks = 0;
	data = NULL;
	ifTape = NULL;
	tapeFile = NULL;
	tmpFileName = NULL;
	orgTapeFile = NULL;

	debug("TapeBrowser", "Initializing...");

	buffer = new BYTE[MAX_TAPE_BLOCK_SIZE + 2];
	memset(bHeadLeader, 0xFF, 16);
	memset(bHeadLeader + 16, 0, 16);
	memset(bHeadLeader + 32, 0x55, 16);

	ProgressBar = new TProgressBar;
	ProgressBar->Max = 0;
	ProgressBar->Position = 0;
	ProgressBar->Active = &playing;

	Selection = new TTapeSelection;
	CheckSelectionContinuity();

	playing = false;
	tapeChanged = false;
	preparedForSave = false;

	if (Settings->TapeBrowser->fileName) {
		char *file = ComposeFilePath(Settings->TapeBrowser->fileName);

		if (SetTapeFileName(file) == 0xFF) {
			delete [] Settings->TapeBrowser->fileName;
			Settings->TapeBrowser->fileName = NULL;
		}

		delete [] file;
	}
}
//---------------------------------------------------------------------------
TTapeBrowser::~TTapeBrowser()
{
	debug("TapeBrowser", "Freeing...");

	FreeAllBlocks();

	if (tapeFile)
		delete [] tapeFile;
	tapeFile = NULL;

	if (orgTapeFile)
		delete [] orgTapeFile;
	orgTapeFile = NULL;

	if (tmpFileName)
		delete [] tmpFileName;
	tmpFileName = NULL;

	if (Selection)
		delete Selection;
	Selection = NULL;

	if (ProgressBar)
		delete ProgressBar;
	ProgressBar = NULL;

	if (buffer)
		delete [] buffer;
	buffer = NULL;
}
//---------------------------------------------------------------------------
void TTapeBrowser::FreeAllBlocks()
{
	TAPE_BLOCK *blk = blocks, *nextb;
	char *prevFileName = NULL;

	while (blk) {
		if (blk->orgFile) {
			if (prevFileName != blk->orgFile) {
				if (prevFileName != NULL) {
					if (isTemp(prevFileName))
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
		if (isTemp(prevFileName))
			unlink(prevFileName);
		delete [] prevFileName;
	}

	blocks = NULL;
	currBlock = NULL;
	totalBlocks = 0;
	currBlockIdx = -1;
	stopBlockIdx = -1;

	CheckSelectionContinuity();
}
//---------------------------------------------------------------------------
void TTapeBrowser::SetIfTape(IifTape *ifTape)
{
	this->ifTape = ifTape;
	ifTape->TapeCommand.connect(this, &TTapeBrowser::TapeCommand);
}
//---------------------------------------------------------------------------
BYTE TTapeBrowser::SetTapeFileName(char *fn)
{
	BYTE ret = 0xFF;

	if (FileExists(fn)) {
		if (playing)
			ActionStop();

		if (tapeFile)
			delete [] tapeFile;
		tapeFile = new char[strlen(fn) + 1];
		strcpy(tapeFile, fn);

		if (tmpFileName)
			delete [] tmpFileName;
		tmpFileName = NULL;

		FreeAllBlocks();
		ret = ParseFile(tapeFile, &blocks);
		SetCurrentBlock(0);
		tapeChanged = false;
		preparedForSave = false;
	}

	return ret;
}
//---------------------------------------------------------------------------
BYTE TTapeBrowser::ImportFileName(char *fn)
{
	BYTE ret = 0xFF;

	if (FileExists(fn)) {
		if (playing)
			ActionStop();

		ret = ParseFile(fn, &blocks);
		tapeChanged = true;
	}

	return ret;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SetNewTape()
{
	if (playing)
		ActionStop();

	FreeAllBlocks();

	if (tapeFile)
		delete [] tapeFile;
	tapeFile = NULL;

	if (tmpFileName)
		delete [] tmpFileName;
	tmpFileName = NULL;

	tapeChanged = false;
	preparedForSave = false;
}
//---------------------------------------------------------------------------
BYTE TTapeBrowser::ParseFile(char *fn, TAPE_BLOCK **blks, DWORD seek)
{
	FILE *hf = fopen(fn, "rb");
	if (hf == NULL || fn == NULL)
		return 0xFF;

	TAPE_BLOCK *lBlk = *blks, blkTmp;
	DWORD dwFileSize, dwPosH, dwPosB, dwTemp;
	WORD  wTemp;
	BYTE  err = 0xFF;
	bool  hdr, oldType;
	char *srcFile = new char[strlen(fn) + 1];
	strcpy(srcFile, fn);

	do {
		if (fseek(hf, 0, SEEK_END) != 0)
			break;

		dwFileSize = ftell(hf);
		if (dwFileSize == (DWORD) -1)
			break;

		if (fseek(hf, seek, SEEK_SET) != 0)
			break;

		oldType = false;
		do {
			memset(buffer, 0, 65);
			memset(&blkTmp, 0, sizeof(blkTmp));
			hdr = false;

			dwPosH = ftell(hf);

			// read 2 + 63 + 2
			if ((dwTemp = fread(buffer, sizeof(BYTE), 67, hf)) < 2)
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

			err = 0;
			totalBlocks++;

			if (lBlk == NULL) {
				lBlk = *blks = new TAPE_BLOCK;
				lBlk->prev = NULL;
			}
			else {
				while (lBlk->next != NULL)
					lBlk = lBlk->next;

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
			err = 1;

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
	if (memcmp(buff, bHeadLeader, 48) != 0)
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
	ifTape->PrepareBlock(data, dataLen, head, Settings->TapeBrowser->flash, true);
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
			if (head && currBlock->cType != 0)
				PrepareData(false);
			else {
				if (currBlockIdx + 1 == totalBlocks)
					SetCurrentBlock(0);
				else
					SetCurrentBlock(currBlockIdx + 1);

				// autostop or rewind
				if (currBlockIdx == 0 ||
					(Settings->TapeBrowser->autoStop == AS_CURSOR && currBlockIdx == stopBlockIdx) ||
					(Settings->TapeBrowser->autoStop == AS_NEXTHEAD && currBlock->cType != 0)) {

					if (Settings->TapeBrowser->autoStop == AS_CURSOR && currBlockIdx == stopBlockIdx)
						stopBlockIdx = -1;

					ActionStop();

					if (result)
						*result = false;
					return;
				}

				PrepareData(true);
			}

			ifTape->PrepareBlock(data, dataLen, head, Settings->TapeBrowser->flash, false);
			ProgressBar->Position = 0;
			ProgressBar->Max = dataLen;
			break;

		case CMD_PROGRESS:
			ProgressBar->Position++;
			break;

		case CMD_MONITORING:
			if (result)
				*result = Settings->TapeBrowser->monitoring;
			return;

		case CMD_PRE_SAVE:
			PrepareSaveNewBlocks();
			break;

		case CMD_SAVE:
			SaveNewBlock();
			break;
	}

	if (result)
		*result = true;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SetCurrentBlock(int idx)
{
	if (idx >= 0) {
		if (currBlock && currBlock->prev && idx == (currBlockIdx - 1))
			currBlock = currBlock->prev;
		else if (currBlock && currBlock->next && idx == (currBlockIdx + 1))
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
void TTapeBrowser::ToggleSelection(int idx)
{
	if (idx >= 0) {
		TAPE_BLOCK *sBlk = NULL;

		if (currBlock && currBlock->prev && idx == (currBlockIdx - 1))
			sBlk = currBlock->prev;
		else if (currBlock && currBlock->next && idx == (currBlockIdx + 1))
			sBlk = currBlock->next;
		else if (blocks) {
			sBlk = blocks;
			while (idx && sBlk->next) {
				sBlk = sBlk->next;
				idx--;
			}
		}

		if (sBlk) {
			sBlk->selected = !sBlk->selected;
			CheckSelectionContinuity();
		}
	}
}
//---------------------------------------------------------------------------
void TTapeBrowser::MoveSelected(bool up, int *cursor)
{
	int fidx = Selection->first,
		lidx = Selection->last,
		idx  = 0;

	if (Selection->total == 0)
		fidx = lidx = *cursor;
	else if (!Selection->continuity)
		return;

	if ((up && fidx == 0) || (!up && lidx == (totalBlocks - 1)))
		return;

	TAPE_BLOCK *first = blocks, *last = NULL, *tb = NULL;

	first = blocks;
	while (idx < fidx && first->next) {
		first = first->next;
		idx++;
	}

	last = first;
	while (idx < lidx && last->next) {
		last = last->next;
		idx++;
	}

	if (up) {
		tb = first->prev;
		first->prev = tb->prev;
		if (first->prev)
			first->prev->next = first;
		else
			blocks = first;
		if (last->next)
			last->next->prev = tb;
		tb->next = last->next;
		last->next = tb;
		tb->prev = last;

		if (*cursor >= fidx && *cursor <= lidx)
			*cursor = *cursor - 1;
	}
	else {
		tb = last->next;
		last->next = tb->next;
		if (last->next)
			last->next->prev = last;
		tb->prev = first->prev;
		if (first->prev)
			first->prev->next = tb;
		else
			blocks = tb;
		first->prev = tb;
		tb->next = first;

		if (*cursor >= fidx && *cursor <= lidx)
			*cursor = *cursor + 1;
	}

	tapeChanged = true;
	CheckSelectionContinuity();
}
//---------------------------------------------------------------------------
void TTapeBrowser::DeleteSelected(int idx)
{
	TAPE_BLOCK *tb = blocks, *tbs = NULL;

	int i = 0, count = 0;
	while (tb) {
		if (tb->selected) {
			DeleteBlock(i, tb);
			count++;
		}
		else if (i == idx)
			tbs = tb;

		tb = tb->next;
		i++;
	}

	if (count == 0 && tbs) {
		DeleteBlock(idx, tbs);
		count++;
	}

	if (count)
		tapeChanged = true;

	CheckSelectionContinuity();
}
//---------------------------------------------------------------------------
void TTapeBrowser::DeleteBlock(int idx, TAPE_BLOCK *tb)
{
	if (tb == NULL) {
		int i = idx;
		tb = blocks;
		while (i && tb->next) {
			tb = tb->next;
			i--;
		}
	}

	if (tb->prev)
		tb->prev->next = tb->next;
	else
		blocks = tb->next;

	if (tb->next)
		tb->next->prev = tb->prev;
	else if (tb->prev)
		tb->prev->next = NULL;

	TAPE_BLOCK *tbx = tb->next;
	if (currBlock == tb)
		currBlock = tbx;

	delete tb;
	tb = tbx;

	totalBlocks--;
	if (currBlockIdx > idx)
		currBlockIdx--;
	if (stopBlockIdx > idx)
			stopBlockIdx--;
	if (stopBlockIdx == currBlockIdx)
		stopBlockIdx = -1;
}
//---------------------------------------------------------------------------
void TTapeBrowser::CheckSelectionContinuity()
{
	int i = 0, c = 0, idx = 0;
	TAPE_BLOCK *tb = blocks;

	Selection->total = 0;
	Selection->first = -1;
	Selection->last = -1;
	Selection->continuity = false;

	if (!tb)
		return;

	do {
		do {
			i++;
			if (tb->selected) {
				if (c == 0) {
					Selection->first = (i - 1);
					idx = i;
				}
				c++;
				break;
			}

			tb = tb->next;
		} while (tb);

		if (tb) {
			if (idx == i)
				idx++;
			else
				idx = -1;

			tb = tb->next;
		}
	} while (tb);

	Selection->total = c;
	Selection->last = (idx - 2);
	Selection->continuity = (idx > 0);
}
//---------------------------------------------------------------------------
void TTapeBrowser::PrepareData(bool head)
{
	FILE *hFile = NULL;

	hFile = fopen(currBlock->orgFile, "rb");
	if (hFile == NULL) {
		warning("TapeBrowser", "PrepareData: '%s' not found", currBlock->orgFile);
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

	if (fread(buffer, sizeof(BYTE), dataLen, hFile) < dataLen)
		warning("TapeBrowser", "PrepareData: File read error!");

	fclose(hFile);
}
//---------------------------------------------------------------------------
void TTapeBrowser::PrepareSaveNewBlocks()
{
	char *file = NULL;
	bool toTemp = false;

	if (blocks == NULL || blocks->dwOffsetHeader == 0)
		file = tapeFile;
	else if (tapeFile && isTemp(tapeFile))
		return;
	else if (tmpFileName == NULL)
		toTemp = true;
	else
		return;

	if (file)
		delete [] file;

	file = new char[64];
	sprintf(file, "%s/%s.%08x%04x.tmp",
		P_tmpdir, PACKAGE_TARNAME, SDL_GetTicks(), getpid());

	FILE *hDest = fopen(file, "wb");
	if (hDest == NULL) {
		warning("TapeBrowser", "PrepareSaveNewBlocks: Can't open temp file '%s'", file);
		delete [] file;
		file = NULL;
		return;
	}

	fclose(hDest);

	if (toTemp) {
		tmpFileName = file;
		return;
	}

	FreeAllBlocks();
	tapeFile = file;
	tapeChanged = false;
	preparedForSave = true;
}
//---------------------------------------------------------------------------
void TTapeBrowser::SaveNewBlock()
{
	char *file = (tmpFileName) ? tmpFileName : tapeFile;

	FILE *hDest = fopen(file, "rb+");
	if (hDest == NULL) {
		warning("TapeBrowser", "SaveNewBlock: Can't open file '%s'", file);
		return;
	}

	fseek(hDest, 0, SEEK_END);

	BYTE *buff;
	DWORD len = ifTape->GetSavedBlock(&buff),
	     seek = ftell(hDest);

	fwrite(buff, sizeof(BYTE), len, hDest);
	fclose(hDest);

	ParseFile(file, &blocks, seek);
	tapeChanged = true;
}
//---------------------------------------------------------------------------
BYTE TTapeBrowser::SaveTape(char *newFileName, TAPE_BLOCK *blks, bool asPTP)
{
	TAPE_BLOCK *blk = blks;
	char *tmpFile = NULL;
	BYTE flag = 0xFF;
	WORD wL;

	if (blk == NULL)
		blk = blocks;

	do {
		if (newFileName == NULL)
			break;

		tmpFile = new char[strlen(newFileName) + 16];
		sprintf(tmpFile, "%s.%08x%04x.tmp",
			newFileName, SDL_GetTicks(), getpid());

		FILE *hDest = fopen(tmpFile, "wb");
		if (hDest == NULL) {
			warning("TapeBrowser", "SaveTape: Can't open temp file '%s'", tmpFile);
			delete [] tmpFile;
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
		rename(tmpFile, newFileName);
		delete [] tmpFile;

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
	char **newpt = (char **) malloc(totalBlocks * sizeof(char *));

	char name[9];
	name[8] = '\0';

	if (newpt != NULL) {
		for (int i = 0; blk && i < totalBlocks; i++) {
			newpt[i] = new char[30];
			newpt[i][28] = (blk->selected) ? '|' : '\0';
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
			delete [] newpt[i];

		free(newpt);
		*data = newpt = NULL;
	}

	*items = 0;
}
//---------------------------------------------------------------------------
