# Installation guide

## Prerequisites:
- C/C++ build tools (`gcc` or `clang`, `make`)
- CMake (`cmake` 3.20+)
- system for managing library compiler/linker flags (`pkg-config`, optional)
- SDL - Simple DirectMedia Library (`libSDL: "2.0.x"`)

### Platform specific instructions:
- Linux distro based on **Ubuntu** (or another **Debian**-like)
  - `sudo apt-get install build-essential cmake pkg-config libsdl2-dev libgl1-mesa-dev`

- Linux distro based on **Fedora** (or another **RedHat**-like)
  - `sudo dnf install gcc make cmake pkgconfig SDL2-devel mesa-libGL-devel`
    _(in older distros there was `yum` package manager instead of `dnf`)_

- **macOS** (tested since 10.12+)
  - first, "Xcode Command Line Tools" is required
    (for `clang`, `make`, and core developer toolchain utilities)
  - in addition to that you will need install tools & libs using Brew:
    `brew install cmake pkg-config sdl2`,
    or with MacPorts: `sudo port install cmake pkgconfig libsdl2`
  - tested on Apple Silicon and latest macOSes

## Building:
- configure build directory with `cmake -S . -B build`
  _(for all available options/switches type `cmake -L -N build` after configuration)_
- build with `cmake --build build`
- _(optional)_ install to the system directories with `sudo cmake --install build`

## Building for the web (WebAssembly via Emscripten):
The emulator can be compiled with [Emscripten](https://emscripten.org) into a single page
with the whole emulator GUI (menu, dialogs, emulator window) running in the browser canvas
(SDL2 is provided by Emscripten ports, rendering uses WebGL 2, the main loop is driven
by the browser via `emscripten_set_main_loop`).

### Prerequisites:
- Python 3, `git`, CMake (3.20+) and `make` (or `ninja`)
- Emscripten SDK (`emsdk`), installed the same way on every platform:
  ```
  git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
  cd ~/emsdk && ./emsdk install latest && ./emsdk activate latest
  source ~/emsdk/emsdk_env.sh
  ```
- **Ubuntu/Debian**: `sudo apt-get install build-essential cmake python3 git`
- **Fedora**: `sudo dnf install gcc make cmake python3 git`
- **macOS**: `xcode-select --install` and `brew install cmake python git`
  (Brew also offers `brew install emscripten` as an alternative to `emsdk`)

### Building:
- `emcmake cmake -S . -B build-web`
- `cmake --build build-web`
- result is `build-web/index.html` + `index.js`, `index.wasm`, `index.data`
  (ROMs and resources are preloaded into the virtual filesystem)
- test locally with `python3 -m http.server -d build-web 8000` and open <http://localhost:8000>
  _(opening the file directly via `file://` doesn't work)_

### Hosting on GitHub Pages:
- the page template is `res/web/shell.html` (fullscreen canvas only)
- workflow `.github/workflows/pages.yml` builds the project with Emscripten and publishes
  `index.*` files; enable it in repository **Settings → Pages → Source: GitHub Actions**
- the page is then served at `https://<user>.github.io/GPMD85Emulator/`
