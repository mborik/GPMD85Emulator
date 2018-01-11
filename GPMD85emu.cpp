/*	GPMD85emu.cpp: Initialization and main program loop.
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
//-----------------------------------------------------------------------------
#include "CommonUtils.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	printf("\n= %s v%s ~ Copyright (c) %s ~ %s =", PACKAGE_NAME, VERSION, PACKAGE_YEAR, PACKAGE_URL);
	printf("\n- This program comes with ABSOLUTELY NO WARRANTY. This is free software,");
	printf("\n- and you are welcome to redistribute it under certain conditions.\n\n");

	PathUserHome = SDL_getenv("HOME");
	PathApplication = getcwd(NULL, PATH_MAX);
	PathResources = (char *) malloc(strlen(DIR_RESOURCES) + 1);
	PathAppConfig = (char *) malloc(strlen(PathUserHome) + 16);
	strcpy(PathResources, DIR_RESOURCES);
	sprintf(PathAppConfig, "%s%c.%s", PathUserHome, DIR_DELIMITER, PACKAGE_TARNAME);

	debug("",   "Resource path: %s", PathResources);
	debug(NULL, "Application path: %s", PathApplication);
	debug(NULL, "Application config path: %s", PathAppConfig);

	if (stat(PathAppConfig, &CommonUtils::filestat) != 0)
		mkdir(PathAppConfig, 0755);

	// initialization of SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
		error("", "Couldn't initialize SDL:\n\t%s", SDL_GetError());

	SDL_DisplayMode desktop;
	if (SDL_GetDesktopDisplayMode(0, &desktop) != 0)
		error("", "Couldn't get desktop display mode:\n\t%s", SDL_GetError());

	debug(NULL, "Actual framebuffer resolution: %d x %d (%d Hz, %s)",
			desktop.w, desktop.h, desktop.refresh_rate,
			SDL_GetPixelFormatName(desktop.format) + 16);

	SDL_zero(gdc);
	gdc.w = desktop.w;
	gdc.h = desktop.h;
	gdc.freq = desktop.refresh_rate;
	gdc.format = desktop.format;
	gdc.window = SDL_CreateWindow(PACKAGE_NAME,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256, 256,
			SDL_WINDOW_HIDDEN);

	if (!gdc.window)
		error("", "Couldn't initialize window:\n\t%s", SDL_GetError());

	gdc.windowID = SDL_GetWindowID(gdc.window);
	gdc.renderer = SDL_CreateRenderer(gdc.window, -1, SDL_RENDERER_ACCELERATED);

	if (!gdc.renderer)
		error("", "Couldn't initialize renderer:\n\t%s", SDL_GetError());

	SDL_RenderSetScale(gdc.renderer, 1, 1);
	SDL_RenderSetIntegerScale(gdc.renderer, SDL_TRUE);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);

	SDL_Surface *icon = SDL_LoadBMP(LocateResource("icon.bmp", false));
	if (icon) {
		SDL_SetColorKey(icon, SDL_TRUE, SDL_MapRGB(icon->format, 255, 0, 255));
		SDL_SetWindowIcon(gdc.window, icon);
		SDL_FreeSurface(icon);
	}
	else
		warning("", "Can't load icon resource file");

//---------------------------------------------------------------------------------------
	debug(NULL, "Initialization process started...");

	Emulator = new TEmulator();
	Emulator->ProcessSettings(-1);
	SDL_ShowWindow(gdc.window);

	debug("", "Starting %d FPS refresh timer", GPU_FRAMES_PER_SEC);
	Emulator->BaseTimer = SDL_AddTimer(GPU_TIMER_INTERVAL, FormMain_BaseTimerCallback, Emulator);
	Emulator->ActionPlayPause(true);

	DWORD nextTick;
	SDL_Event event;
	int i = 0, j, k = 0;
	BYTE *kb = Emulator->keyBuffer;
	bool waitForRelease = false;

	debug("", "Starting main CPU %dHz loop", CPU_FRAMES_PER_SEC);

	while (Emulator->isActive) {
		nextTick = SDL_GetTicks() + CPU_TIMER_INTERVAL;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					if (event.key.repeat != 0)
						break;
					/* no break */
				case SDL_QUIT:
					memcpy(kb, SDL_GetKeyboardState(NULL), SDL_NUM_SCANCODES);
					kb[SDL_SCANCODE_NUMLOCKCLEAR] = kb[SDL_SCANCODE_CAPSLOCK] = kb[SDL_SCANCODE_SCROLLLOCK] = 0;
					if (event.type == SDL_QUIT)
						kb[SDL_SCANCODE_POWER] = 1;

					if (waitForRelease) {
						for (j = --i; j > 0; j--) {
							if (kb[j]) {
								k++;
								break;
							}
						}
					}
					else if ((waitForRelease = Emulator->TestHotkeys()) == true)
						k = 4;
					break;

				case SDL_WINDOWEVENT:
					if (event.window.windowID == gdc.windowID) {
						if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
							Emulator->RefreshDisplay();
						}
						else if (Settings->pauseOnFocusLost && (
								event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED ||
								event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)) {

							Emulator->ActionPlayPause((event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED));
						}
					}
					break;

				default:
					break;
			}
		}

		if (k > 0 && i > 0) {
			k--;
			memset(kb, 0, sizeof(BYTE) * i);
		}
		else
			waitForRelease = false;

		Emulator->CpuTimerCallback();

		while (SDL_GetTicks() < nextTick)
			SDL_Delay(1);
	}

	SDL_HideWindow(gdc.window);
	debug("", "Main CPU loop terminated");

	SDL_RemoveTimer(Emulator->BaseTimer);
	SDL_Delay(WEAK_REFRESH_TIME);

	delete Emulator;

	SDL_DestroyRenderer(gdc.renderer);
	SDL_DestroyWindow(gdc.window);
	SDL_Quit();

	free(PathResources);
	free(PathAppConfig);
	free(PathApplication);

	return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------
