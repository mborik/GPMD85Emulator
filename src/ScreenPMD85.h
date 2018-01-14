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
#define WEAK_REFRESH_TIME 200
//-----------------------------------------------------------------------------
typedef struct SCANLINER_DEF {
	DWORD x2[6 * 4];
	DWORD x3[6 * 9];
	DWORD x4[6 * 16];
} SCANLINER_DEF;
//-----------------------------------------------------------------------------
#define scanlinerMethodPrototype(function) void function(DWORD *dst, int pitch, DWORD *scl, int w, int h)
typedef void (*scanlinerMethod)(DWORD *dst, int pitch, DWORD *scl, int w, int h);
//-----------------------------------------------------------------------------
scanlinerMethodPrototype(point2x);
scanlinerMethodPrototype(point3x);
scanlinerMethodPrototype(point4x);
//-----------------------------------------------------------------------------
class ScreenPMD85
{
public:
	ScreenPMD85(TDisplayMode dispMode, int border);
	virtual ~ScreenPMD85();

	inline TDisplayMode GetDisplayMode() { return dispMode; }
	void SetDisplayMode(TDisplayMode dispMode, int border);
	inline void SetDisplayMode(TDisplayMode dispMode) {
		SetDisplayMode(dispMode, borderSize / BORDER_MULTIPLIER);
	}

	void SetWidth384(bool mode384);
	inline bool IsWidth384() { return width384mode; }

	void SetHalfPassMode(THalfPassMode halfPass);
	inline THalfPassMode GetHalfPassMode() { return halfPass; }

	void SetLcdMode(bool lcdMode);
	inline bool IsLcdMode() { return lcdMode; }

	inline void SetBlinkStatus(bool dotsDisplayed) { blinkState = dotsDisplayed; }
	inline void ToggleBlinkStatus() { blinkState = !blinkState;}
	inline bool GetBlinkStatus() { return blinkState; }

	inline int GetMultiplier() { return screenHeight / bufferHeight; }

	void SetColorProfile(TColorProfile ColProf);
	inline TColorProfile GetColorProfile() { return colorProfile; }

	void SetColorAttr(int idx, TColor attr);
	TColor GetColorAttr(int idx);

	void RefreshDisplay();
	void FillBuffer(BYTE *videoRam);

private:
	SDL_Texture *scanlinerTexture;
	SDL_Texture *screenTexture;
	SDL_Rect *screenRect;

	int borderSize;
	int bufferWidth;
	int bufferHeight;
	int screenWidth;
	int screenHeight;

	bool blinkState;
	bool blinkingEnabled;
	bool lcdMode;
	bool width384mode;
	bool displayModeChanging;

	TDisplayMode dispMode;
	TColorProfile colorProfile;
	THalfPassMode halfPass;
	TColor cAttr[4];
	TColor pAttr[8];
	DWORD *palette;

	const SCANLINER_DEF *scanliner;
	int scanlinerMode;

	void InitScreenSize(TDisplayMode reqDispMode, bool reqWidth384);
	void PrepareVideoMode();
	void ReleaseVideoMode();
	void PrepareScreen(bool clear = false);
	void PrepareScanliner();
	void InitScanliners();
	void InitPalette();
};
//-----------------------------------------------------------------------------
#endif
