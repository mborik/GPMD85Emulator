/*	SoundDriver.cpp: Sound signal generation and audio output
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011 Martin Borik <mborik@users.sourceforge.net>

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
#include "CommonUtils.h"
#include "SoundDriver.h"
//---------------------------------------------------------------------------
SoundDriver::SoundDriver(int numChn, char totalAmpl)
{
	initOK = false;
	playOK = true;
	writePos = -1;
	channels = NULL;
	hFile = NULL;

	numChannels = numChn;
	channels = new CHANNEL[numChn];
	soundBuff = new BYTE[FRAME_SIZE];

	for (int ii = 0; ii < numChn; ii++) {
		memset(&channels[ii], 0, sizeof(CHANNEL));
		channels[ii].sampleBuff = new char[FRAME_SIZE];
		memset(channels[ii].sampleBuff, 0, FRAME_SIZE);
	}

	SDL_AudioSpec desired;
	memset(&desired, 0, sizeof(desired));

	desired.freq = SAMPLE_RATE;
	desired.format = AUDIO_U8;
	desired.channels = 1;
	desired.samples = AUDIO_BUFF_SIZE;
	desired.callback = SoundDriver_MixerCallback;
	desired.userdata = this;

	initOK = (SDL_OpenAudio(&desired, NULL) != -1);
	if (initOK) {
		silence = desired.silence;
		SetVolume(totalAmpl);
		SDL_PauseAudio(0);
		playOK = true;
	}
	else {
		warning("Unable to open audio:\n%s", SDL_GetError());
		SoundMute();
	}
}
//---------------------------------------------------------------------------
SoundDriver::~SoundDriver()
{
	if (hFile != NULL)
		CloseWaveFile();

	playOK = false;
	SDL_PauseAudio(1);
	initOK = false;
	SDL_CloseAudio();

	if (channels) {
		for (int ii = 0; ii < numChannels; ii++) {
			if (channels[ii].sampleBuff)
				delete[] channels[ii].sampleBuff;
		}

		delete[] channels;
		channels = NULL;
	}

	delete[] soundBuff;
	soundBuff = NULL;
}
//---------------------------------------------------------------------------
// public metody
//---------------------------------------------------------------------------
void SoundDriver::SetVolume(char vol)
{
	totalVolume = (char) (vol & 0x7F);
	channelVolume = (char) (totalVolume / numChannels);
	if (channelVolume == 0 && totalVolume > 0)
		channelVolume = 1;
#if FADEOUT_ON
	if (channelVolume == 0)
		channelFadeout = FADEOUT_RATE;
	else
		channelFadeout = FADEOUT_RATE / channelVolume;
#endif
}
//---------------------------------------------------------------------------
void SoundDriver::SoundMute()
{
	playOK = false;
	SDL_PauseAudio(1);
	channelVolume = 0;
}
//---------------------------------------------------------------------------
void SoundDriver::SoundOn()
{
	SetVolume(totalVolume);
	SDL_PauseAudio(0);
	playOK = true;
}
//---------------------------------------------------------------------------
void SoundDriver::PrepareSample(int chn, bool state, int ticks)
{
	char *ptr, val;
	int curPos;

	if (!initOK || !playOK)
		return;

	val = (char) ((state == true) ? channelVolume : -channelVolume);
	if (channels[chn].oldVal == val)
		return;

	curPos = (ticks * FRAME_SIZE) / TCYCLES_PER_FRAME;

	// vyplnenie medzery od poslednej pozicie
	ptr = channels[chn].sampleBuff + channels[chn].fillPos;
	for (int ii = channels[chn].fillPos; ii < curPos && ii < FRAME_SIZE; ii++) {
#if FADEOUT_ON
		*ptr++ = FadeoutChannel(chn);
#else
		*ptr++ = channels[chn].curVal;
#endif
	}

	// zapis na novu poziciu
	if (curPos < FRAME_SIZE)
		channels[chn].sampleBuff[curPos] = val;

	channels[chn].fillPos = curPos + 1;
	channels[chn].curVal = val;
	channels[chn].oldVal = val;
}
//---------------------------------------------------------------------------
void SoundDriver::PrepareBuffer()
{
	char *ptr, val;
	int ii, jj;

	if (!initOK || !playOK)
		return;

	// stisovanie od aktualnej pozicie do konca buffra
	for (ii = 0; ii < numChannels; ii++) {
		ptr = channels[ii].sampleBuff + channels[ii].fillPos;
		for (jj = channels[ii].fillPos; jj < FRAME_SIZE; jj++) {
#if FADEOUT_ON
			*ptr++ = FadeoutChannel(ii);
#else
			*ptr++ = channels[ii].curVal;
#endif
		}

		channels[ii].fillPos = 0;
	}

	SDL_LockAudio();

	// mixovanie kanalov
	BYTE *data = soundBuff;
	for (jj = 0; jj < FRAME_SIZE; jj++) {
		val = 0;
		for (ii = 0; ii < numChannels; ii++)
			val += channels[ii].sampleBuff[jj];

		*data++ = ((BYTE)val) + silence;
	}

	writePos = 0;

	// zaznam do suboru
	if (hFile != NULL)
		fwrite(soundBuff, FRAME_SIZE, 1, hFile);

	SDL_UnlockAudio();
}
//---------------------------------------------------------------------------
void SoundDriver::FillSoundBuffer(BYTE *data, DWORD len)
{
	if (writePos < 0)
		return;

	if ((writePos + len) > FRAME_SIZE)
		len = FRAME_SIZE - writePos;

	memcpy(data, soundBuff + writePos, len);

	writePos += len;
	if (writePos >= FRAME_SIZE)
		writePos = -1;
}
//---------------------------------------------------------------------------
bool SoundDriver::CreateWaveFile(const char* fileName)
{
	if (!initOK || hFile != NULL)
		return false;

	hFile = fopen(fileName, "w+");
	if (hFile == NULL)
		return false;

	memcpy(head.riff, "RIFF", 4);
	memcpy(head.wave, "WAVE", 4);
	memcpy(head.fmt,  "fmt ", 4);
	head.hdrLength = 4 * sizeof(WORD) + 2 * sizeof(DWORD);
	head.wFormatTag = 1;
	head.nChannels = 1;
	head.nSamplesPerSec = SAMPLE_RATE;
	head.nAvgBytesPerSec = SAMPLE_RATE;
	head.nBlockAlign = 1;
	head.wBitsPerSample = 8;
	memcpy(head.data, "data", 4);
	fwrite(&head, sizeof(WAVE_HEADER), 1, hFile);

	return true;
}
//---------------------------------------------------------------------------
bool SoundDriver::CloseWaveFile()
{
	if (!initOK || hFile == NULL)
		return false;

	fseek(hFile, 0L, SEEK_END);
	DWORD flen = ftell(hFile);

	fseek(hFile, 0L, SEEK_SET);
	fread(&head, sizeof(WAVE_HEADER), 1, hFile);

	head.totLength = flen - 8;
	head.dataLength = flen - sizeof(WAVE_HEADER);

	fseek(hFile, 0L, SEEK_SET);
	fwrite(&head, sizeof(WAVE_HEADER), 1, hFile);

	fclose(hFile);
	hFile = NULL;

	return true;
}
//---------------------------------------------------------------------------
#if FADEOUT_ON
char SoundDriver::FadeoutChannel(int chn)
{
	channels[chn].tick += SAMPLE_TICK_INC;
	if (channels[chn].tick >= channelFadeout) {
		channels[chn].tick -= channelFadeout;
		if (channels[chn].curVal > 0)
			channels[chn].curVal--;
		else if (channels[chn].curVal < 0)
			channels[chn].curVal++;
	}

	return channels[chn].curVal;
}
#endif
//---------------------------------------------------------------------------
