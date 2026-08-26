/*	SoundDriver.cpp: Sound signal generation and audio output
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
#include "CommonUtils.h"
#include "SoundDriver.h"
#include "Mif85.h"
//---------------------------------------------------------------------------
SoundDriver::SoundDriver(char totalAmpl)
{
	audioDevice = 0;
	initOK = false;
	playOK = true;
	enabledMIF85 = false;
	channels = NULL;

	SDL_AudioSpec desired;
	SDL_zero(desired);

	desired.freq = SAMPLE_RATE;
	desired.format = AUDIO_U8;
	desired.channels = BYTES_PER_SAMPLE;
	desired.samples = AUDIO_BUFF_SIZE;
	desired.callback = NULL;
	desired.userdata = this;

	initOK = (SDL_OpenAudio(&desired, NULL) != -1);
	if (initOK) {
		audioDevice = 1; // always 1 after SDL_OpenAudio
		debug("Sound", "Initialized device to %dHz/%dbit with %dB (%dB) buffer",
				desired.freq, desired.format, desired.samples, desired.size);

		silence = desired.silence;

		int frameSize = desired.size / BYTES_PER_SAMPLE;
		if (frameSize != AUDIO_BUFF_SIZE)
			warning("Sound", "Initialized sound buffer size different from desired! (%d vs %d)",
					AUDIO_BUFF_SIZE, frameSize);

		size_t len = SAMPS_PER_CPU_FRAME * BYTES_PER_SAMPLE;
		soundBuff = new BYTE[len];
		memset(soundBuff, silence, sizeof(BYTE) * len);

		channels = new CHANNEL[AUDIO_MAX_CHANNELS];
		for (int ii = 0; ii < AUDIO_MAX_CHANNELS; ii++) {
			memset(&channels[ii], 0, sizeof(CHANNEL));
			channels[ii].sampleBuff = new char[len];
			memset(channels[ii].sampleBuff, silence, len);
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

	int curPos = (ticks * SAMPS_PER_CPU_FRAME) / TCYCLES_PER_FRAME;

	// filling a gap from the last position
	char *ptr = channels[chn].sampleBuff + (channels[chn].fillPos * BYTES_PER_SAMPLE);
	for (int ii = channels[chn].fillPos; ii < curPos && ii < SAMPS_PER_CPU_FRAME; ii++) {
		*ptr++ = FadeoutChannel(chn);
		*ptr++ = FadeoutChannel(chn);
	}

	// writting to the new position
	if (curPos < SAMPS_PER_CPU_FRAME) {
		ptr = channels[chn].sampleBuff + (curPos * BYTES_PER_SAMPLE);
		*ptr++ = val;
		*ptr++ = val;
	}

	channels[chn].fillPos = curPos + 1;
	channels[chn].curVal = val;
	channels[chn].oldVal = val;
}
//---------------------------------------------------------------------------
void SoundDriver::PrepareBuffer()
{
	char *ptr;
	int ii, jj;
	size_t len = SAMPS_PER_CPU_FRAME * BYTES_PER_SAMPLE;

	if (!initOK || !playOK)
		return;

	memset(soundBuff, silence, len);
	for (ii = 0; ii < numChannels; ii++) {
		if (ii == CHNL_MIF85) {
			if (SAA1099 != NULL)
				SAA1099->GenerateMany((BYTE *) channels[ii].sampleBuff, SAMPS_PER_CPU_FRAME);
			else
				memset(channels[ii].sampleBuff, silence, len);
		}
		else {
			// fadeout from actual position to the end of buffer
			ptr = channels[ii].sampleBuff + (channels[ii].fillPos * BYTES_PER_SAMPLE);
			for (jj = channels[ii].fillPos; jj < SAMPS_PER_CPU_FRAME; jj++) {
				*ptr++ = FadeoutChannel(ii);
				*ptr++ = FadeoutChannel(ii);
			}
		}

		channels[ii].fillPos = 0;

		SDL_MixAudioFormat(
			soundBuff,
			(const BYTE *) channels[ii].sampleBuff,
			(ii < AUDIO_BEEP_CHANNELS) ? AUDIO_S8 : AUDIO_U8,
			len, totalVolume
		);
	}

	SDL_QueueAudio(audioDevice, (const void *) soundBuff, len);
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
