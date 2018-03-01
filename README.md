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
- **Cize/SinDiKat**
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
- to enable trace informations use `./configure --enable-trace`.
