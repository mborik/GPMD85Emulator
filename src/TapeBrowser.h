//---------------------------------------------------------------------------
#ifndef TAPEBROWSER_H_
#define TAPEBROWSER_H_
//---------------------------------------------------------------------------
#include "globals.h"
#include "IifTape.h"
#include "Settings.h"
//---------------------------------------------------------------------------
class TTapeBrowser : public sigslot::has_slots<>
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
#pragma pack(pop)

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
		bool ParseFile(char *fn, TAPE_BLOCK **blks, DWORD seek = 0);
		bool CheckCrc(BYTE *buff, int length, BYTE *goodCrc);
		bool CheckHeader(BYTE *buff, TAPE_BLOCK *blk);
		void DeleteBlock(int idx, TAPE_BLOCK *blk = NULL);
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

		char *orgTapeFile;

		int currBlockIdx;
		int stopBlockIdx;
		int totalBlocks;

		TProgressBar *ProgressBar;
		TTapeSelection *Selection;

		void SetIfTape(IifTape *ifTape);
		bool SetTapeFileName(char *fn);
		bool ImportFileName(char *fn);
		void SetNewTape();
		void ActionPlay();
		void ActionStop();
		void SetCurrentBlock(int idx);
		void ToggleSelection(int idx);
		void MoveSelected(bool up, int *cursor);
		void DeleteSelected(int idx);
		void TapeCommand(int command, bool *result);
		BYTE SaveTape(char *newFileName, TAPE_BLOCK *blks, bool asPTP);
		void FillFileList(char ***data, int *items, bool hex = false);
		void FreeFileList(char ***data, int *items);
};
//---------------------------------------------------------------------------
#endif
