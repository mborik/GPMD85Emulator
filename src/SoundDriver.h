/*	SoundDriver.h: Sound signal generation and audio output
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2024 Martin Borik <martin@borik.net>

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
#ifndef SOUNDDRIVER_H_
#define SOUNDDRIVER_H_
//---------------------------------------------------------------------------
#include "globals.h"
//---------------------------------------------------------------------------
class SoundDriver
{
	public:
		SoundDriver(char totalAmpl);
		virtual ~SoundDriver();

		void SetVolume(char vol);
		void EnableMIF85(bool enabled);
		void SoundMute();
		void SoundOn();

		void PrepareSample(int chn, bool state, int ticks);
		void PrepareBuffer();

	private:
		char FadeoutChannel(int chn);
		int  channelFadeout;

		bool initOK;
		bool playOK;
		bool enabledMIF85;

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
		int  numChannels;
		CHANNEL *channels;
		char totalVolume;
		char channelVolume;
		BYTE *soundBuff;

		SDL_AudioDeviceID audioDevice;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
