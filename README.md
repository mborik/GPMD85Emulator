# GPMD85Emulator
- Open-source, multi-platform emulator of the Tesla PMD 85,
  an 8-bit personal microcomputer produced in 1980s in former Czechoslovakia

> Notice: GPMD85Emulator is not intended to be built and run on Microsoft Windows,
> because there is a much better and more feature-rich **PMD 85 emulator**
> specifically for this platform (https://pmd85.borik.net/wiki/Emulator)

## AUTHORS:
- **mborik** _[Martin Bórik]_
  - lead programmer of this *nix port
- **rombor** _[Roman Bórik]_
  - original Windows code and emulation core
- **ub880d**
  - support developer, maintainer, tester
- **mikezt/zeroteam**
  - quick-search in the file-selector, bugfixing and testing
- **a8jan**
  - implementantion of Mega ROM Module, 5x5 scaler, support
- **Staon**
  - fixed a compatibility bug in the file selector
- **Cizo/SinDiKat**
  - bugfixing, testing and support
- **ikon/SinDiKat**
  - testing and support

## REQUIRED LIBRARIES:
- **SDL2 - Simple DirectMedia Library** _(v2.0.x)_
- **OpenGL - Open Graphics Library** _(v2.1 or higher)_

## INCORPORATED LIBRARIES:
- **SAASound** by Dave Hooper & Simon Owen, [see license](saa/LICENCE)
- **Dear ImGui** by Omar Cornut, [see license](gui/imgui/LICENSE.txt)
- **sigslot** by Pierre-Antoine Lacaze, [see source](sigslot.hpp)

## HOTKEYS:
- function `[f]` keys are any of Alt, Win, Mac or Meta keys
- e.g. for start/stop of tape emulator, use `[f]+P` hotkey

## INSTALLATION:
- check [installation guide](INSTALL.md) for prerequisites
- clone with all submodules with `git clone --recurse-submodules [url]`
- generate configuration scripts with `autoreconf -vfi`
  _(you should have the autotools package installed)_
- run script `./configure`
- then run `make` to build
- _(optional)_ install into system dirs with `sudo make install`

## CONFIGURATION PARAMETERS:
- to enable debug mode, use `./configure --enable-debug`
- to disable all trace messages, use `./configure --disable-trace` (size optimization)

## COMMAND-LINE ARGUMENTS:
- `-h`, `--help`
  --- print this help
- `-v`, `--version`
  --- print version number
- `-c`, `--over-cfg`
  --- override user's configuration
- `-m`, `--machine` `{X}`
  --- select machine (`1`, `2`, `2A`, `3`, `C2717`, `Alfa`, `Alfa2`, `Mato`)
- `-r`, `--rmm`
  --- connect ROM module
- `-mrm`, `--megarom`
  --- Mega ROM module image
- `-sc`, `--scaler` `{1..5}`
  --- screen size multiplier
- `-bd`, `--border` `{0..9}`
  --- screen border width
- `-hp`, `--halfpass` `{0..5}`
  --- scanliner (`0`=NONE, `1`-`4`=HALFPASS, `5`=LCD)
- `-cp`, `--profile` `{0..3}`
  --- color profile (`0`=MONO, `1`=STD, `2`=RGB, `3`=ColorACE)
- `-vol`, `--volume` `{0..127}`
  --- sound volume (`0`=MUTE)
- `-mif`, `--mif85`
  --- connect MIF 85 music interface
- `-p`, `--pmd32`
  --- connect PMD 32 disk interface
- `-drA`, `--drive-a` `"filename.p32"`
  --- drive A disk image
- `-dwA`, `--drive-a-write`
  --- drive A write enabled
- `-drB`, `--drive-b` `"filename.p32"`
  --- drive B disk image
- `-dwB`, `--drive-b-write`
  --- drive B write enabled
- `-drC`, `--drive-c` `"filename.p32"`
  --- drive C disk image
- `-dwC`, `--drive-c-write`
  --- drive C write enabled
- `-drD`, `--drive-d` `"filename.p32"`
  --- drive D disk image
- `-dwD`, `--drive-d-write`
  --- drive D write enabled
- `-t`, `--tape` `"filename.ptp"`
  --- tape image
- `-trs`, `--tape-real`
  --- real tape speed
- `-s`, `--snap` `"filename.psn"`
  --- load snapshot
- `-b`, `--memblock` `"filename.bin"`
  --- load memory block
- `-ptr`, `--memblock-address` `{WORD}`
  --- load memory block at given address
