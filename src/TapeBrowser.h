//---------------------------------------------------------------------------
#ifndef TAPEBROWSER_H_
#define TAPEBROWSER_H_
//---------------------------------------------------------------------------
#include "globals.h"
#include "IifTape.h"
#include <vector>
//---------------------------------------------------------------------------
class TTapeBrowser
{
	public:
#pragma pack(push, 1)
		typedef struct TapeBlock {
			BYTE  bNumber;
			char  cType;
			WORD  wStart;
			WORD  wLength;
			char  cName[8];
			BYTE  bCrc;
			BYTE  bBodyCrc;
			DWORD dwOffsetHeader;
			DWORD dwOffsetBody;
			bool  headCrcError;
			bool  bodyCrcError;
			bool  bodyCrcErrorFix;
			bool  rawFile;
			int   bodyLengthError;
			bool  selected;
			char *orgFile;
			TapeBlock *prev;
			TapeBlock *next;
		} TAPE_BLOCK;
#pragma pack(pop)

		typedef struct TProgressBar {
			bool *Active;
			int   Max;
			int   Position;
		} TProgressBar;

		typedef struct TTapeSelection {
			bool continuity;
			int  first;
			int  last;
			int  total;
		} TTapeSelection;

		typedef struct TDialogItem {
			char name[14];
			int start;
			int length;
			bool headCrcError;
		} TDialogItem;

	private:
		IifTape *ifTape;

		char *tapeFile;
		char *tmpFileName;

		TAPE_BLOCK *blocks;
		TAPE_BLOCK *currBlock;

		BYTE  bHeadLeader[48];
		BYTE *buffer;
		BYTE *data;
		WORD  dataLen;
		bool  head;

		void FreeAllBlocks();
		int  ParseFile(const char *fn, TAPE_BLOCK **blks, DWORD seek = 0);
		bool CheckCrc(BYTE *buff, int length, BYTE *goodCrc);
		bool CheckHeader(BYTE *buff, TAPE_BLOCK *blk);
		TAPE_BLOCK *DeleteBlock(int idx, TAPE_BLOCK *blk = NULL);
		void CheckSelectionContinuity();
		void PrepareData(bool head);
		void PrepareSaveNewBlocks();
		void SaveNewBlock();

	public:
		TTapeBrowser();
		virtual ~TTapeBrowser();

		bool playing;
		bool tapeChanged;
		bool preparedForSave;
		bool shouldUpdateEntries;

		char *orgTapeFile;

		int currBlockIdx;
		int stopBlockIdx;
		int totalBlocks;

		TProgressBar *ProgressBar;
		TTapeSelection *Selection;

		void SetIfTape(IifTape *ifTape);
		int  SetTapeFileName(const char *fn);
		int  ImportFileName(const char *fn);
		void SetNewTape();
		void ActionPlay();
		void ActionStop();

		TAPE_BLOCK *GetBlock(int idx);
		void SetCurrentBlock(int idx);

		void DoSelection(int idx = -1, bool select = false);
		void MoveSelected(bool up, int *cursor);
		void DeleteSelected(int idx = -1);
		void TapeCommand(int command, bool *result);
		int  SaveTape(const char *newFileName, TAPE_BLOCK *blks = NULL, bool asPTP = true);
		void FillFileList(std::vector<TDialogItem> &data);
};
//---------------------------------------------------------------------------
#endif
