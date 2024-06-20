# GPMD85Emulator
- multiplatform GNU/GPL emulator of the Tesla PMD 85, an 8-bit personal
  micro-computer produced in 80s of 20th century in former Czechoslovakia

> Notice that GPMD85Emulator is not aimed to build and run on Windows,
> because there is our much better and feature-rich PMD 85 Emulator
> specifically for this platform (https://pmd85.borik.net/wiki/Emulator)

## AUTHORS:
- **mborik** _[Martin Bórik]_
  - leader programmer of this *nix port based on SDL
- **rombor** _[Roman Bórik]_
  - original Windows code and emulation of CPU and chips
- **ub880d**
  - support developer, maintainer, tester

### SPECIAL THANKS FOR THE SUPPORT:
- **ikon/SinDiKat**
  - translation of comments sk>en, testing and support
- **mikezt/zeroteam**
  - quick-search in file-selector, bugfixing and testing
- **Cizo/SinDiKat**
  - testing and support
- **Staon**
  - fixing compatibility bug in file-selector's ScanDir

## REQUIRED LIBRARIES:
- **SDL - Simple DirectMedia Library** _(v2.0.x)_

## HOTKEYS:
- function `[f]` keys are any of Alt, Win, Mac or Meta keys
- main menu appear with `[f]+F1` or with the "menu key"
- for start/stop of tape emulator use `[f]+P` hotkey

## INSTALLATION:
- check [installation guide](INSTALL.md) for prerequisites.
- generate configuration scripts with `autoreconf -vfi`
  _(you will need autotools package)_
- run script `./configure`
- then `make` to build
- _(optional)_ install to the system dirs with `sudo make install`

## CONFIGURATION PARAMETERS:
- to enable debug mode use `./configure --enable-debug`
- omit all trace messages with `./configure --disable-trace` (size optimization)
- to use software 2D rendering context instead of accelerated use `./configure --with-soft-render`

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
- `-soft`, `--soft-render`
  --- use software renderer instead of accelerated
