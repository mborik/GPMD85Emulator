// Emscripten main loop helper, adapted from Dear ImGui examples
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
#else
#define EMSCRIPTEN_MAINLOOP_BEGIN
#define EMSCRIPTEN_MAINLOOP_END
#endif
