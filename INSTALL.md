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

- **macOS** (tested on 10.12+)
  - first, "Xcode Command Line Tools" is required (for `clang`, `make`, and core developer toolchain utilities)
  - in addition to that you will need install tools & libs via MacPorts:
    `sudo port install cmake pkgconfig libsdl2`,
    or with Brew: `brew install cmake pkg-config sdl2`
  - tested on Apple Silicon M1 and latest macOS 11.0+

## Building:
- configure build directory with `cmake -S . -B build`
  _(for all available options/switches type `cmake -L -N build` after configuration)_
- build with `cmake --build build`
- _(optional)_ install to the system directories with `sudo cmake --install build`
