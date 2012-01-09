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
			BYTE bNumber;
			char cType;
			WORD wStart;
			WORD wLength;
			char cName[8];
			BYTE bCrc;
			BYTE bBodyCrc;
			DWORD dwOffsetHeader;
			DWORD dwOffsetBody;
			bool headCrcError;
			bool bodyCrcError;
			bool bodyCrcErrorFix;
			bool rawFile;
			int bodyLengthError;
			char *orgFile;
			TapeBlock *prev;
			TapeBlock *next;
		} TAPE_BLOCK;

		typedef struct TProgressBar {
			bool *Active;
			int   Max;
			int   Position;
		} TProgressBar;
#pragma pack(pop)

	private:
		IifTape *ifTape;
		TSettings::SetTapeBrowser *settings;

		char *tapeFile;
		char *orgTapeFile;
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
		void PrepareData(bool head);
		void PrepareSaveNewBlocks();
		void SaveNewBlock();

	public:
		TTapeBrowser(TSettings::SetTapeBrowser *set);
		virtual ~TTapeBrowser();

		bool playing;
		bool tapeChanged;
		bool preparedForSave;

		int currBlockIdx;
		int stopBlockIdx;
		int totalBlocks;

		TProgressBar *ProgressBar;

		void SetIfTape(IifTape *ifTape);
		bool SetTapeFileName(char *fn);
		void SetNewTape();
		void ActionPlay();
		void ActionStop();
		void SelectBlock(int idx);
		void TapeCommand(int command, bool *result);
		BYTE SaveTape(char *newFileName, TAPE_BLOCK *blks, bool asPTP);
		void FillFileList(char ***data, int *items, bool hex = false);
		void FreeFileList(char ***data, int *items);
};
//---------------------------------------------------------------------------
#endif
