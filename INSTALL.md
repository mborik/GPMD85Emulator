# Installation guide

## Prerequisites:
- C/C++ build tools (`gcc` or `clang`, `make`)
- autotools package (`autoconf`, `automake`, `autoheader`, `autom4te`...)
- system for managing library compiler/linker flags (`pkg-config`)
- SDL - Simple DirectMedia Library (`libSDL: "2.0.x"`)

### Platform specific instructions:
- Linux distro based on **Ubuntu** (or another **Debian**-like)
  - `sudo apt-get install build-essential autoconf autotools-dev pkg-config libsdl2-dev`

- Linux distro based on **Fedora** (or another **RedHat**-like)
  - `sudo dnf install gcc make autoconf automake pkgconfig SDL2-devel`
    _(in older distros there was `yum` package manager instead of `dnf`)_

- **MacOS** (tested on 10.12+)
  - first, "Xcode Command Line Tools" is required (for `clang` & `make`)
  - in addition to that you will need install tools & libs via MacPorts:
    `sudo port install autoconf automake pkgconfig libsdl2`,
    or with Brew: `brew install autoconf automake pkg-config sdl2`
  - tested on Apple Silicon M1 and latest MacOS 11.0+

## Building:
- generate configuration scripts with `autoreconf -vfi`
- run script `./configure`
  _(for all available options/switches type `./configure --help`)_
- then `make` to build
- _(optional)_ install to the system directories with `sudo make install`
