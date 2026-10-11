# Description of (E)PROM content files for PMD 85
-------------------------------------------------
- all files in this directory are binary contents of
  (E)PROM chips and sizes are in multiples of kilobytes.

## Operating system ##
- so called Monitor
- file extension: `.rom`

### `monit1.rom`
- OS for PMD 85-1
- size 4kB
- location #8000

### `monit1u.rom`
- modified OS for PMD 85-1
  + fixed system PIO initialization (#8004)
  + added 8253 timer initialization (#8276)
- size 4kB
- location #8000

### `monit1cs.rom`
- modified OS for PMD 85-1
  + fixed system PIO initialization (#8004)
  + added 8253 timer initialization (#8276)
  + extended charset with diacritic chars
- size 5kB
- location #8000

### `monit2.rom`
- OS for PMD 85-2
- size 4kB
- location #8000

### `monit2E.rom`
- OS for PMD 85-2 with BASIC program loader from ROM module
- some additional utilities
- size 8kB (6kB)
- location #8000

### `monit2A.rom`
- OS for PMD 85-2A
- size 4kB
- location #8000

### `monit3.rom`
- OS for PMD 85-3
- size 8kB
- location #E000

### `mato-mb.rom`, `mato-mb-ru.rom`
- OS for MATO microcomputer with BASIC-G V2.1
- including BASIC demo program
- RU version - russion charset and translations
- size 16kB
- location #8000

### `mato-gm.rom`
- games for MATO microcomputer
  * games: AUTO, PAMPUCH, TEHLY, ZABY, KLAVIR and KRESLIC
- size 16kB
- location #8000

### `mato-gm2-sk.rom`, `mato-gm2-en.rom`
- games for MATO microcomputer - slovak/english version
  * games: AUTO, PAMPUCH, TEHLY, SPACE INVADERS
- size 16kB
- location #8000

### `mato-gm3.rom`
- game STRIPPED
- size 16kB
- location #8000
- author: Miroslav Urda

### `c2717.rom`
- OS for Consul 2717 microcomputer
- BASIC-G and extension with disk and network services
- disk driver for 8" drives
- size 16kB
- location #8000

### `c2717-2.rom`
- OS for Consul 2717 microcomputer
- BASIC-G and extension with disk and network services
- disk driver for both 8" or 5,25" drives
- size 16kB
- location #8000

### `didalfa.rom`
- OS for Didaktik Alfa microcomputer including BASIC alfa
- size 13kB (4kB for OS, 9kB BASIC alfa)
- location #8000

### `didalfa2.rom`
- OS for Didaktik Alfa 2 microcomputer including BASIC alfa 2.1
- size 16kB
- location #8000


## ROM modules
- kind of extension cartridges
- file extension `.rmm`

### `basic1.rmm`
- BASIC-G V1.0 for PMD 85-1
- size 9kB
- source location #0000
- target location #0000

### `basic1tx.rmm`
- BASIC-G V1.0 for PMD 85-1 with extended editor
- charset extended with lowercase and diacritic characters
- size 1kB
- any source location after BASIC-G
- target location 748Ah

### `basic2.rmm`
- BASIC-G V2.0 for PMD 85-2
- size 9kB
- source location #0000
- target location #0000

### `basic2A.rmm`
- BASIC-G V2.A for PMD 85-2A
- size 9kB
- source location #0000
- target location #0000

### `basic3.rmm`
- BASIC G V3.0 for PMD 85-3
- size 10kB
- source location #0000
- target location #0000 (9kB) and #B800 (1kB)

### `pascal1.rmm`
- PASCAL V1.02(A) for PMD 85-2(A)
- size 20kB
- source location #0000
- target location #2DB0
- author: RNDr. Peter Tomcsanyi

### `pascal22.rmm`
- TOM Pascal V2.2(A) for PMD 85-2(A)
- size 32kB
- source location #0000
- target location #0000
- author: RNDr. Peter Tomcsanyi

### `kli2.rmm`
- function keys K0 to K11 will be filled with some BASIC commands
- work with BASIC for PMD 85-2(A)
- size 1kB

### `demo0.rmm, demo0-c.rmm`
- demo program from PMD 85-0 prototype
- size 1kB
- any source location after BASIC-G
- suffix `c` is adopted to color output

### `wurmi.rmm`
- original "housenka"
- size 3kB
- any source location after BASIC-G
- author: Frantisek Fuka, 1984

### `sach1.rmm`
- chess for PMD 85-1
- size 12kB
- source location #0000
- target location #0000
- run with `JOB 000030000000` command from OS

### `mrs2.rmm`
- Memory Resident System - assembler & monitor
- work with PMD 85-2 or 2A
- size 9kB
- source location #0000
- target location #6000
- standard system library will be loaded to #0040
  when STOP key was pressed while initialization

### `booter2-pmd85-pmd32.rmm`
- driver for PMD 32 device for PMD 85-2(A)
- size 2kB
- source location #0000
- will be copied to free space in VRAM space (buffer right to screen data  #C470)

### `pmd85-pmd32.rmm`
- former driver for PMD 32 device for PMD 85-2
- including DOS PMD 32
- size 2kB
- any source location after BASIC-G
- target location #7900
