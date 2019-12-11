/*	SoundDriver.cpp: Sound signal generation and audio output
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2019 Martin Borik <mborik@users.sourceforge.net>

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
#include "Mif85.h"
//---------------------------------------------------------------------------
SoundDriver::SoundDriver(char totalAmpl)
{
	initOK = false;
	playOK = true;
	enabledMIF85 = false;
	writePos = -1;
	channels = NULL;
	hFile = NULL;

	SDL_AudioSpec desired;
	SDL_zero(desired);

	desired.freq = SAMPLE_RATE;
	desired.format = AUDIO_U8;
	desired.channels = 2;
	desired.samples = AUDIO_BUFF_SIZE;
	desired.callback = SoundDriver_MixerCallback;
	desired.userdata = this;

	initOK = (SDL_OpenAudio(&desired, NULL) != -1);
	if (initOK) {
		debug("Sound", "Initialized device to %dHz/%dbit with %dB buffer",
				desired.freq, desired.format, desired.samples);

		frameSize = desired.size;
		silence = desired.silence;

		soundBuff = new BYTE[frameSize];
		channels = new CHANNEL[AUDIO_MAX_CHANNELS];
		for (int ii = 0; ii < AUDIO_MAX_CHANNELS; ii++) {
			memset(&channels[ii], 0, sizeof(CHANNEL));
			channels[ii].sampleBuff = new char[frameSize * 2];
			memset(channels[ii].sampleBuff, silence, frameSize * 2);
		}

		initOK = true;
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
		for (int ii = 0; ii < AUDIO_MAX_CHANNELS; ii++) {
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
void SoundDriver::SetVolume(char vol)
{
	numChannels = enabledMIF85 ? AUDIO_MAX_CHANNELS : AUDIO_BEEP_CHANNELS;

	totalVolume = (char) (vol & 0x7F);
	channelVolume = (char) (totalVolume / AUDIO_BEEP_CHANNELS);
	if (channelVolume == 0 && totalVolume > 0)
		channelVolume = 1;

	if (channelVolume == 0)
		channelFadeout = FADEOUT_RATE;
	else
		channelFadeout = FADEOUT_RATE / channelVolume;
}
//---------------------------------------------------------------------------
void SoundDriver::EnableMIF85(bool enabled)
{
	enabledMIF85 = enabled;
	SetVolume(totalVolume);
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
	if (!initOK || !playOK)
		return;

	char val = (char) (state ? channelVolume : -channelVolume);
	if (channels[chn].oldVal == val)
		return;

	int curPos = (ticks * frameSize) / TCYCLES_PER_FRAME;

	// filling a gap from the last position
	char *ptr = channels[chn].sampleBuff + channels[chn].fillPos;
	for (int ii = channels[chn].fillPos; ii < curPos && ii < frameSize; ii++) {
		*ptr++ = FadeoutChannel(chn);
		*ptr++ = FadeoutChannel(chn);
	}

	// writting to the new position
	if (curPos < frameSize) {
		channels[chn].sampleBuff[curPos] = val;
		channels[chn].sampleBuff[curPos + 1] = val;
	}

	channels[chn].fillPos = curPos + 2;
	channels[chn].curVal = val;
	channels[chn].oldVal = val;
}
//---------------------------------------------------------------------------
void SoundDriver::PrepareBuffer()
{
	char *ptr, valL, valR;
	int ii, jj;

	if (!initOK || !playOK)
		return;

	// fadeout from actual position to the end of buffer
	for (ii = 0; ii < numChannels; ii++) {
		if (ii == CHNL_MIF85) {
			if (SAA1099 != NULL)
				SAA1099->GenerateMany((BYTE *) channels[ii].sampleBuff, AUDIO_BUFF_SIZE);
			else
				memset(channels[ii].sampleBuff, silence, frameSize);
		}
		else {
			ptr = channels[ii].sampleBuff + channels[ii].fillPos;
			for (jj = channels[ii].fillPos; jj < frameSize; jj += 2) {
				*ptr++ = FadeoutChannel(ii);
				*ptr++ = FadeoutChannel(ii);
			}

			channels[ii].fillPos = 0;
		}
	}

	SDL_LockAudio();
	writePos = 0;
	SDL_UnlockAudio();

	// file writer with channel mixing
	if (hFile != NULL) {
		BYTE *data = soundBuff;

		for (jj = 0; jj < frameSize; jj += 2) {
			valL = 0;
			valR = 0;

			for (ii = 0; ii < numChannels; ii++) {
				valL += (channels[ii].sampleBuff[jj] / numChannels);
				valR += (channels[ii].sampleBuff[jj + 1] / numChannels);
			}

			*data++ = (BYTE) valL;
			*data++ = (BYTE) valR;
		}

		fwrite(soundBuff, frameSize, 1, hFile);
	}
}
//---------------------------------------------------------------------------
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
//---------------------------------------------------------------------------
void SoundDriver::FillSoundBuffer(BYTE *data, DWORD len)
{
	memset(data, silence, len);

	if ((writePos + ((int) len)) > frameSize)
		len = (DWORD) (frameSize - writePos);

	for (int ii = 0; ii < numChannels; ii++) {
		SDL_MixAudioFormat(
			data,
			(const BYTE *) (channels[ii].sampleBuff + writePos),
			(ii < AUDIO_BEEP_CHANNELS) ? AUDIO_S8 : AUDIO_U8,
			len, totalVolume
		);
	}

	int bufLen = (int) len;
	if ((writePos + bufLen) < frameSize)
		writePos += bufLen;
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
	head.nChannels = 2;
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
	size_t rr, wr = 0, hlen = sizeof(WAVE_HEADER);

	fseek(hFile, 0L, SEEK_SET);
	rr = fread(&head, hlen, 1, hFile);

	if (rr == 1) {
		head.totLength = flen - 8;
		head.dataLength = flen - hlen;

		fseek(hFile, 0L, SEEK_SET);
		wr = fwrite(&head, hlen, 1, hFile);
	}

	fclose(hFile);
	hFile = NULL;

	return (rr == wr);
}
//---------------------------------------------------------------------------
