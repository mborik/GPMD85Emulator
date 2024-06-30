/*	SoundDriver.cpp: Sound signal generation and audio output
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>
	Copyright (c) 2011-2024 Martin Borik <mborik@users.sourceforge.net>

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
