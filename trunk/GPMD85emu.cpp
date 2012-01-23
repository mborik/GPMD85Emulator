/*	GPMD85emu.cpp: Initialization and main program loop.
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
//-----------------------------------------------------------------------------
#include "CommonUtils.h"
#include "GPMD85main.h"
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	printf("\n= %s v%s ~ Copyright (c) 2011 RM-TEAM =", PACKAGE_NAME, VERSION);
	printf("\n- This program comes with ABSOLUTELY NO WARRANTY. This is free software,");
	printf("\n- and you are welcome to redistribute it under certain conditions.\n\n");

	PathUserHome = SDL_getenv("HOME");
	PathApplication = get_current_dir_name();
	PathResources = new char[(strlen(DIR_RESOURCES) + 1)];
	PathAppConfig = new char[(strlen(PathUserHome) + 16)];
	strcpy(PathResources, DIR_RESOURCES);
	sprintf(PathAppConfig, "%s%c.%s", PathUserHome, DIR_DELIMITER, PACKAGE_TARNAME);

	debug("",   "Resource path: %s", PathResources);
	debug(NULL, "Application path: %s", PathApplication);
	debug(NULL, "Application config path: %s", PathAppConfig);

	struct stat filestat;
	if (stat(PathAppConfig, &filestat) != 0)
		mkdir(PathAppConfig, 0755);

	// initialization of SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE) < 0)
		error("", "Couldn't initialize SDL: %s\n", SDL_GetError());

	const SDL_VideoInfo *vi = SDL_GetVideoInfo();
	SDL_Rect** modes = SDL_ListModes(NULL, SDL_FULLSCREEN);

	gvi.w = gvi.h = 0;
	gvi.hw = vi->hw_available;
	gvi.wm = vi->wm_available;
	gvi.depth = vi->vfmt->BitsPerPixel;

	if (modes > (SDL_Rect **) 0) {
		debug(NULL, "Actual framebuffer resolution: %d x %d",
			vi->current_w, vi->current_h);

		for (int i = 0; modes[i]; i++) {
			if (modes[i]->w == vi->current_w || modes[i]->h == vi->current_h) {
				gvi.w = modes[i]->w;
				gvi.h = modes[i]->h;

				if (gvi.w == vi->current_w && gvi.h == vi->current_h)
					break;
			}
		}
	}

	if (!(gvi.w + gvi.h)) {
		gvi.w = vi->current_w;
		gvi.h = vi->current_h;
	}

	if (SDL_VideoModeOK(gvi.w, gvi.h, gvi.depth, SDL_FULLSCREEN) == 0) {
		warning("", "Full-screen disabled, no suitable resolution!");
		gvi.w = gvi.h = 0;
	}
	else
		debug(NULL, "Full-screen resolution: %d x %d", gvi.w, gvi.h);

	SDL_EnableKeyRepeat(0, 0);

	if (gvi.wm) {
		SDL_WM_SetCaption(PACKAGE_NAME, NULL);
		SDL_Surface *icon = SDL_LoadBMP(LocateResource("icon.bmp", false));
		if (icon) {
			SDL_SetColorKey(icon, SDL_SRCCOLORKEY, SDL_MapRGB(icon->format, 255, 0, 255));
			SDL_WM_SetIcon(icon, NULL);
			SDL_FreeSurface(icon);
		}
		else
			warning("", "Can't load icon resource file");
	}

	debug(NULL, "Initialization process started...");

	Emulator = new TEmulator();
	Emulator->ProcessSettings(-1);

	debug("", "Starting %d FPS refresh timer", GPU_FRAMES_PER_SEC);
	Emulator->BaseTimer = SDL_AddTimer(GPU_TIMER_INTERVAL, FormMain_BaseTimerCallback, Emulator);
	Emulator->ActionPlayPause(true);

	DWORD nextTick;
	int i = 0, j, k = 0;
	BYTE *kb = NULL;
	SDL_Event event;
	bool waitForRelease = false;

	debug("", "Starting main CPU %dHz loop", CPU_FRAMES_PER_SEC);
	while (Emulator->isActive) {
		nextTick = SDL_GetTicks() + CPU_TIMER_INTERVAL;

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
				case SDL_KEYUP:
				case SDL_KEYDOWN:
					Emulator->keyBuffer = kb = SDL_GetKeyState(&i);
					kb[SDLK_NUMLOCK] = kb[SDLK_CAPSLOCK] = kb[SDLK_SCROLLOCK] = 0;
					if (event.type == SDL_QUIT)
						kb[SDLK_POWER] = 1;

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

		// have a break, have a tick-tock...
		while (SDL_GetTicks() < nextTick)
			SDL_Delay(1);
	}

	debug("", "Main CPU loop terminated");
	SDL_RemoveTimer(Emulator->BaseTimer);
	SDL_Delay(WEAK_REFRESH_TIME);

	delete Emulator;
	SDL_Quit();

	delete [] PathResources;
	delete [] PathAppConfig;
	free(PathApplication);

	return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------
