/*	GPMD85emu.cpp: Initialization and main program loop.
	Copyright (c) 2011-2026 Martin Borik <martin@borik.net>

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
#define GL_GLEXT_PROTOTYPES
#include "imconfig_mod.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
//-----------------------------------------------------------------------------
#include "ArgvParser.h"
#include "CommonUtils.h"
#include "Emulator.h"
//-----------------------------------------------------------------------------
#ifdef IMGUI_IMPL_OPENGL_ES2
#  include "SDL_opengles2.h"
#else
#  include "SDL_opengl.h"
#endif
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	if (!ParseOptions(&argc, &argv))
		return EXIT_FAILURE;
	else if (argv_config.version) {
		printf("v%s\n", VERSION);
		return EXIT_SUCCESS;
	}

	IntroMessage();

	PathUserHome = SDL_getenv("HOME");
	PathApplication = getcwd(NULL, PATH_MAX);
	PathResources = new char[strlen(DIR_RESOURCES) + 1];
	PathAppConfig = new char[strlen(PathUserHome) + 16];
	PathGuiConfig = new char[strlen(PathUserHome) + 32];
	strcpy(PathResources, DIR_RESOURCES);
	sprintf(PathAppConfig, "%s%c.%s", PathUserHome, DIR_DELIMITER, PACKAGE_TARNAME);
	sprintf(PathGuiConfig, "%s%c.%s/imgui.conf", PathUserHome, DIR_DELIMITER, PACKAGE_TARNAME);

	debug("",   "Resource path: %s", PathResources);
	debug(NULL, "Application path: %s", PathApplication);
	debug(NULL, "Application config path: %s", PathAppConfig);
	debug(NULL, "ImGui config path: %s", PathGuiConfig);

	if (stat(PathAppConfig, &filestat) != 0)
		mkdir(PathAppConfig, 0755);

	// initialization of SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0)
		error("", "Couldn't initialize SDL:\n\t%s", SDL_GetError());

	// Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
	// GL ES 2.0 + GLSL 100 (WebGL 1.0)
	const char* glsl_version = "#version 100";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(IMGUI_IMPL_OPENGL_ES3)
	// GL ES 3.0 + GLSL 300 es (WebGL 2.0)
	const char* glsl_version = "#version 300 es";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
	// GL 3.2 Core + GLSL 150
	const char* glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

	// From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
	SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

	SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_JOY_CONS, "1");
	SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");

	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_WindowFlags window_flags = (SDL_WindowFlags) (
		SDL_WINDOW_OPENGL |
		SDL_WINDOW_RESIZABLE |
		SDL_WINDOW_HIDDEN
	);

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
			window_flags);

	if (!gdc.window)
		error("", "Couldn't initialize window:\n\t%s", SDL_GetError());

	gdc.windowID = SDL_GetWindowID(gdc.window);
	gdc.context = SDL_GL_CreateContext(gdc.window);
	if (!gdc.context)
		error("", "Couldn't initialize OpenGL context:\n\t%s", SDL_GetError());

	SDL_GL_MakeCurrent(gdc.window, gdc.context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.IniFilename = PathGuiConfig;

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(1.0f);
	style.FontScaleDpi = 1.0f;
	style.FontSizeBase = 13.0f;
	style.FrameRounding = 0.0f;
	ImVec4 background = style.Colors[ImGuiCol_WindowBg];

	io.Fonts->AddFontDefaultBitmap();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	style.Colors[ImGuiCol_TitleBgActive] = style.Colors[ImGuiCol_MenuBarBg];
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForOpenGL(gdc.window, gdc.context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	SDL_Surface *icon = SDL_LoadBMP(LocateResource("icon.bmp"));
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
	Emulator->ProcessArgvOptions(true);

	if (Settings->GUI->position.x >= 0 || Settings->GUI->position.y >= 0)
		SDL_SetWindowPosition(gdc.window, Settings->GUI->position.x, Settings->GUI->position.y);
	if (Settings->GUI->windowSize.x >= 0 || Settings->GUI->windowSize.y >= 0)
		SDL_SetWindowSize(gdc.window, Settings->GUI->windowSize.x, Settings->GUI->windowSize.y);
	SDL_ShowWindow(gdc.window);

	Emulator->ActionPlayPause(true);

	Uint64 lastTime = SDL_GetPerformanceCounter(), currentTime;
	double cpuAccumulator = 0.0, baseAccumulator = 0.0, deltaTime;

	SDL_Event event;
	int i = 0, j, k = 0;
	BYTE *kb = Emulator->keyBuffer;
	bool waitForRelease = false;

	debug("", "Starting main CPU %dHz loop and %d FPS refresh timer",
		CPU_FRAMES_PER_SEC, GPU_FRAMES_PER_SEC);

	while (Emulator->isActive) {
		currentTime = SDL_GetPerformanceCounter();
		deltaTime = (double)((currentTime - lastTime) * 1000) / (double) SDL_GetPerformanceFrequency();
		lastTime = currentTime;

		if (deltaTime > 100.0)
			deltaTime = 100.0;

		cpuAccumulator += deltaTime;
		baseAccumulator += deltaTime;

		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);

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

				case SDL_CONTROLLERDEVICEADDED:
				case SDL_CONTROLLERDEVICEREMOVED:
					Emulator->ActionJoyControllers(NULL, true);
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

		while (cpuAccumulator >= CPU_TIMER_INTERVAL) {
			Emulator->CpuTimerCallback();
			cpuAccumulator -= CPU_TIMER_INTERVAL;
		}

		while (baseAccumulator >= GPU_TIMER_INTERVAL) {
			Emulator->BaseTimerCallback(
				io.WantCaptureKeyboard && GUI->InAnyWindowExceptEmulator()
			);

			Emulator->RefreshDisplay();
			baseAccumulator -= GPU_TIMER_INTERVAL;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		GUI->DrawMenu();
		GUI->DrawTapeDialog();
		GUI->DrawAboutDialog();
		GUI->DrawQueryDialog();
		GUI->DrawFileSelector();
		GUI->DrawDiskImagesDialog();
		GUI->DrawEmulatorWindow();

		Emulator->actionCallback();
		ImGui::Render();

		glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
		glClearColor(background.x * background.w, background.y * background.w, background.z * background.w, background.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(gdc.window);
	}

	SDL_GetWindowPosition(gdc.window,
			&Settings->GUI->position.x, &Settings->GUI->position.y);
	SDL_GetWindowSize(gdc.window,
			&Settings->GUI->windowSize.x, &Settings->GUI->windowSize.y);

	SDL_HideWindow(gdc.window);
	debug("", "Main loop terminated");

	delete Emulator;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gdc.context);
	SDL_DestroyWindow(gdc.window);
	SDL_Quit();

	delete [] PathGuiConfig;
	delete [] PathResources;
	delete [] PathAppConfig;
	free(PathApplication);

	return EXIT_SUCCESS;
}
//-----------------------------------------------------------------------------
