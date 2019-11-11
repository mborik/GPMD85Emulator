/*	SoundDriver.h: Sound signal generation and audio output
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2018 Martin Borik <mborik@users.sourceforge.net>

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
#ifndef SOUNDDRIVER_H_
#define SOUNDDRIVER_H_
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
class SoundDriver : public sigslot::has_slots<>
{
	public:
		SoundDriver(int numChn, char totalAmpl);
		virtual ~SoundDriver();

		void SetVolume(char vol);
		void SoundMute();
		void SoundOn();

		void PrepareSample(int chn, bool state, int ticks);
		void PrepareBuffer();
		void FillSoundBuffer(BYTE *data, DWORD len);

		bool CreateWaveFile(const char* fileName);
		bool CloseWaveFile();

	private:
		char FadeoutChannel(int chn);
		int  channelFadeout;

		bool initOK;
		bool playOK;
		int  writePos;

		#pragma pack(push, 1)
		typedef struct {
			BYTE  riff[4];
			DWORD totLength;
			BYTE  wave[4];
			BYTE  fmt[4];
			DWORD hdrLength;
			WORD  wFormatTag;
			WORD  nChannels;
			DWORD nSamplesPerSec;
			DWORD nAvgBytesPerSec;
			WORD  nBlockAlign;
			WORD  wBitsPerSample;
			BYTE  data[4];
			DWORD dataLength;
		} WAVE_HEADER;
		#pragma pack(pop)

		typedef struct {
			char *sampleBuff;
			char curVal;
			char oldVal;
			int fillPos;
			int tick;
		} CHANNEL;

		BYTE silence;
		DWORD frameSize;
		int numChannels;
		CHANNEL *channels;
		char totalVolume;
		char channelVolume;
		BYTE *soundBuff;

		FILE *hFile;
		WAVE_HEADER head;
};
//---------------------------------------------------------------------------
inline void SoundDriver_MixerCallback(void *param, BYTE *buf, int len)
{
	((SoundDriver *) param)->FillSoundBuffer(buf, len);
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
