// Emscripten helpers (main loop, file download), adapted from Dear ImGui examples
// (https://github.com/ocornut/imgui/blob/master/examples/libs/emscripten/emscripten_mainloop_stub.h)
//
// Usage:
//   EMSCRIPTEN_MAINLOOP_BEGIN
//   { ...loop body... }
//   EMSCRIPTEN_MAINLOOP_END
// Under Emscripten the body becomes a callback driven by the browser
// (requestAnimationFrame); the main loop never returns, so ASYNCIFY is not needed.
#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <functional>

static std::function<void()> MainLoopForEmscriptenP;
static void MainLoopForEmscripten() { MainLoopForEmscriptenP(); }

#define EMSCRIPTEN_MAINLOOP_BEGIN  MainLoopForEmscriptenP = [&]()
#define EMSCRIPTEN_MAINLOOP_END    ; emscripten_set_main_loop(MainLoopForEmscripten, 0, true)

// Offers a file from the virtual filesystem to the user as a browser download
// and removes it from the filesystem afterwards.
static inline void InvokeFileDownload(const char *fileName, const char *mimeType)
{
	EM_ASM({
		var path = UTF8ToString($0);
		var mime = UTF8ToString($1);
		var name = path.split('/').pop();
		var blob = new Blob([FS.readFile(path)], { type: mime });
		var a = document.createElement('a');
		a.href = URL.createObjectURL(blob);
		a.download = name;
		document.body.appendChild(a);
		a.click();
		document.body.removeChild(a);
		setTimeout(function () { URL.revokeObjectURL(a.href); FS.unlink(path); }, 1000);
	}, fileName, mimeType);
}
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif
