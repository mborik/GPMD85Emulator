/*	ScreenPMD85.h: Core of graphical output and screen generation
	Copyright (c) 2010-2026 Martin Borik <martin@borik.net>

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
//-----------------------------------------------------------------------------
#ifndef SCREENPMD85_H_
#define SCREENPMD85_H_
//-----------------------------------------------------------------------------
#include "globals.h"
#include "imgui/imgui.h"
#include "TapeBrowser.h"
//-----------------------------------------------------------------------------
#define BORDER_MULTIPLIER 12
#define DWORD_COLOR_ENTRY(R, G, B) SDL_FOURCC(R, G, B, 0xff)
//-----------------------------------------------------------------------------
typedef struct SCANLINER_DEF {
	DWORD x2[6 * 4];
	DWORD x3[6 * 9];
	DWORD x4[6 * 16];
	DWORD x5[6 * 25];
} SCANLINER_DEF;
//-----------------------------------------------------------------------------
#define scanlinerMethodPrototype(function) void function(DWORD *dst, int pitch, DWORD *scl, int w, int h)
typedef void (*scanlinerMethod)(DWORD *dst, int pitch, DWORD *scl, int w, int h);
//-----------------------------------------------------------------------------
scanlinerMethodPrototype(point2x);
scanlinerMethodPrototype(point3x);
scanlinerMethodPrototype(point4x);
scanlinerMethodPrototype(point5x);
//-----------------------------------------------------------------------------
class ScreenPMD85
{
public:
	ScreenPMD85(TDisplayMode dispMode, int border);
	virtual ~ScreenPMD85();

	inline TDisplayMode GetDisplayMode() { return dispMode; }
	void SetDisplayMode(TDisplayMode dispMode, int border);
	inline void SetDisplayMode(TDisplayMode dispMode)
		{ SetDisplayMode(dispMode, borderSize / BORDER_MULTIPLIER); }

	void SetWidth384(bool mode384);
	inline bool IsWidth384() { return width384mode; }

	void SetHalfPassMode(THalfPassMode halfPass);
	inline THalfPassMode GetHalfPassMode() { return halfPass; }

	void SetLcdMode(bool lcdMode);
	inline bool IsLcdMode() { return lcdMode; }

	inline void SetBlinkStatus(bool state) { blinkState = state; }
	inline void ToggleBlinkStatus() { blinkState = !blinkState; }
	inline bool GetBlinkStatus() { return blinkState; }

	inline int GetMultiplier() { return screenWidth / bufferWidth; }
	inline int GetScreenOffsetX() { return screenRect->x; }
	inline int GetScreenOffsetY() { return screenRect->y; }
	inline DWORD *GetPalette() { return palette; }

	void SetColorProfile(TColorProfile ColProf);
	inline TColorProfile GetColorProfile() { return colorProfile; }

	void SetColorAttr(int idx, TColor attr);
	TColor GetColorAttr(int idx);

	void RefreshDisplay();
	void FillBuffer(BYTE *videoRam, bool needRedraw = true);

	ImVec2 GetBorderOffset() { return ImVec2((float) borderSize, (float) borderSize); }
	ImVec2 GetWindowSize() { return ImVec2((float) windowWidth, (float) windowHeight); }
	ImVec2 GetScreenSize() { return ImVec2((float) screenWidth, (float) screenHeight); }
	ImTextureID GetScreenTexture() { return (ImTextureID) (intptr_t) screenTexture; }
	ImTextureID GetScalerTexture() { return (ImTextureID) (intptr_t) scanlinerTexture; }

private:
	BYTE *screenPixelBuffer;
	uint screenTexture;
	BYTE *scanlinerPixelBuffer;
	uint scanlinerTexture;

	SDL_Rect *screenRect;
	SDL_mutex *displayModeMutex;

	int borderSize;
	int bufferWidth;
	int bufferHeight;
	int windowWidth;
	int windowHeight;
	int screenWidth;
	int screenHeight;

	bool blinkState;
	bool blinkingEnabled;
	bool lcdMode;
	bool width384mode;

	TDisplayMode dispMode;
	TColorProfile colorProfile;
	THalfPassMode halfPass;
	BYTE cAttr[4];
	BYTE pAttr[8];
	DWORD palette[16];

	const SCANLINER_DEF *scanliner;
	int scanlinerMode;

	void InitVideoMode(TDisplayMode reqDispMode, bool reqWidth384);
	void ReleaseVideoMode();
	void PrepareScanliner();
	void InitScanliners();
	void InitPalette();
};
//-----------------------------------------------------------------------------
#endif
