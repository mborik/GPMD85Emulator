/*	ScreenPMD85.h: Core of graphical output and screen generation
	Copyright (c) 2010-2018 Martin Borik <mborik@users.sourceforge.net>

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
//-----------------------------------------------------------------------------
#ifndef SCREENPMD85_H_
#define SCREENPMD85_H_
//-----------------------------------------------------------------------------
#include "globals.h"
#include "UserInterface.h"
#include "TapeBrowser.h"
//-----------------------------------------------------------------------------
#define BORDER_MULTIPLIER 12
#define STATUSBAR_ICON    10
#define STATUSBAR_SPACING 14
#define STATUSBAR_HEIGHT  20
#define WEAK_REFRESH_TIME 200
//-----------------------------------------------------------------------------
class ScreenPMD85
{
public:
	ScreenPMD85(TDisplayMode dispMode, int border);
	virtual ~ScreenPMD85();

	void SetDisplayMode(TDisplayMode dispMode, int border);
	inline void SetDisplayMode(TDisplayMode dispMode) { SetDisplayMode(dispMode, borderSize / BORDER_MULTIPLIER); }
	inline TDisplayMode GetDisplayMode() { return DispMode; }

	void SetWidth384(bool mode384);
	inline bool IsWidth384() { return Width384mode; }

	void SetHalfPassMode(THalfPassMode halfPass);
	inline THalfPassMode GetHalfPassMode() { return HalfPass; }

	void SetLcdMode(bool lcdMode);
	inline bool IsLcdMode() { return LCDmode; }

	inline void SetBlinkStatus(bool dotsDisplayed) { BlinkState = dotsDisplayed; }
	inline void ToggleBlinkStatus() { BlinkState = !BlinkState;}
	inline bool GetBlinkStatus() { return BlinkState; }

	inline int GetMultiplier() { return BlitRectDest->w / bufferWidth; }

	void SetColorProfile(TColorProfile ColProf);
	inline TColorProfile GetColorProfile() { return ColorProfile; }

	void SetColorAttr(int Index, TColor Attr);
	TColor GetColorAttr(int Index);

	void SetLedState(int led);
	void SetIconState(int icon);
	void SetComputerModel(TComputerModel model);

	inline void SetStatusPercentage(int val) { statusPercentage = val; }
	inline void SetStatusFPS(int val) { statusFPS = val; }

	void RefreshDisplay();
	void FillBuffer(BYTE *videoRam);

private:
	SDL_Renderer *Renderer;
	SDL_Texture *ScreenBuffer;
	SDL_Rect *BlitRectDest;

	int bufferWidth;
	int bufferHeight;
	int Width;
	int Height;

	int borderSize;
	int ledState;
	int iconState;
	int statusPercentage;
	int statusFPS;
	char computerModel[8];

	TDisplayMode DispMode;
	TDisplayMode FullScreenScaleMode;
	TColorProfile ColorProfile;
	TColor CAttr[4];
	TColor PAttr[8];
	THalfPassMode HalfPass;

	bool BlinkState;
	bool BlinkingEnabled;
	bool LCDmode;
	bool Width384mode;
	bool DisplayModeChanging;

	void InitScreenSize(TDisplayMode reqDispMode, bool reqWidth384);
	void PrepareVideoMode();
	void ReleaseVideoMode();

	void PrepareStatusBar(bool clear = false);
	void RedrawStatusBar();

	DWORD Palette[256];
	void InitPalette();
};
//-----------------------------------------------------------------------------
#endif
