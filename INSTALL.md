# Installation guide

## Prerequisites:
- C/C++ build tools (`gcc` or `clang`, `make`)
- autotools package (`autoconf`, `automake`, `autoheader`, `autom4te`...)
- system for managing library compiler/linker flags (`pkg-config`)
- SDL - Simple DirectMedia Library (`libSDL: "1.2.x"`)

### Platform specific instructions:
- Linux distro based on **Ubuntu** (or another **Debian**-like)
  - `sudo apt-get install build-essential autoconf autotools-dev pkg-config libsdl1.2-dev`

- Linux distro based on **Fedora** (or another **RedHat**-like)
  - `sudo dnf install gcc make autoconf automake pkgconfig SDL-devel`
    _(in older distros there was `yum` package manager instead of `dnf`)_

- **MacOS** (tested on Sierra)
  - first, "Xcode Command Line Tools" is required (for `clang` & `make`)
  - in addition to that you will need install tools & libs via MacPorts:
    `sudo port install autoconf automake pkgconfig libsdl`

### OpenGL support additional requirements:
> **VOLUNTEERS NEEDED:**
> Everyone can include here some steps or guide for his own distro/platform.

- **Fedora/RedHat** or **Ubuntu/Debian**:
  - preinstalled with SDL as requirement of `SDL_opengl.h`


## Building:
- generate configuration scripts with `autoreconf -vfi`
- run script `./configure`
  _(for all available options/switches type `./configure --help`)_
- then `make` to build
- _(optional)_ install to the system directories with `sudo make install`
