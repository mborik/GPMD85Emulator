-------------------------------------------------------------------------------
* copytape.ptp *
-------------------------------------------------------------------------------
- copying programs

-------------------------------------------------------------------------------
| Name     | Start     | Machine  | Author                                    |
-------------------------------------------------------------------------------
| COPY     | JUMP 0000 | 1        | IFO software - UAK                        |
| COPYPLUS | JUMP 0100 | 1        | IFO software                              |
| COPYCOPY | JUMP 0000 | 1        | IFO software                              |
| COPPY    | JUMP 0000 | 1        | ODPM PRAHA 2                              |
| WELL+2   | AutoRun   | 2,2A,3   | Karel Šuhajda, Petr Tůma                  |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* dam-0000.ptp *
-------------------------------------------------------------------------------
- Karel Šuhajda
- DAM - Disassembler / Assembler / Monitor - for PMD 85-1
- System for creating and debugging programs in assembly language
- Source files

+ DAM-0000 is relocatable and can be loaded at any address from 0000H up to
  5800H. Loading is done with the command 'MGLD 00 XX', where XX is the high
  byte of the address to which DAM will be loaded. If it should be at address
  0000H, XX does not have to be entered.
-------------------------------------------------------------------------------
| Name     | Type | Start     | Machine | Note                                |
-------------------------------------------------------------------------------
| DAM-0000 |  ?   | AutoRun   | 1       | the DAM program                     |
| DAM0-SRC |  %   |           |         | source code of DAM-0000 itself      |
| JOYDEMO  |  %   |           |         | source - 4004/482 joystick test     |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* dam-0000-src.ptp *
-------------------------------------------------------------------------------
- Karel Šuhajda
+ Source file of DAM-0000 in the format for DAM-0000 itself (type `%`) and in
  the format for the newer DAM+2 (type `A`).
-------------------------------------------------------------------------------
| Name     | Type | Note                                                      |
-------------------------------------------------------------------------------
| DAM0-SRC |  %   | DAM-0000 source code                                      |
| DAM0-SRC |  A   | DAM-0000 source code for DAM+2                            |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* dam+2.ptp *
-------------------------------------------------------------------------------
- Karel Šuhajda
- DAM - Disassembler / Assembler / Monitor - original cassette + bonus content
- System for creating and debugging programs in assembly language

+ The standard version of DAM+2 is relocatable and can be loaded at any address
  from 0000H up to 5800H. Loading is done with the command 'MGLD 00 XX', where
  XX is the high byte of the address to which DAM will be loaded. If it should
  be at address 0000H, XX does not have to be entered.
+ DAM+9000 is a version of DAM that runs in the extended memory of PMD 85-2A
  or PMD 85-3 computers, so the entire lower part of the memory remains free
  for the source text. On the PMD 85-3 it is of course necessary to switch to
  compatibility mode first by entering the command JUMP FFF0. On the PMD 85-2A
  it is necessary to switch to AllRAM mode first by running the short program
  "ALLRAM2A".
  In both cases, the MONITOR (8000H to 8FFFH) is already in RAM and may
  therefore be accidentally overwritten!
+ The RESRC2 utility is relocatable and can be loaded at any address from 0000H
  up to 7800H. Loading is done with the command 'MGLD 00 XX', where XX is the
  high byte of the address to which it will be loaded. If it should be at
  address 0000H, XX does not have to be entered.

- The author of the OPTIMAX utility is not known to me (probably Petr Tůma).
- The author of the RESRC2 utility is Petr Tůma.
- The authors of the BASIC program JOYSTICK are M. Černý and V. Hollmann
-------------------------------------------------------------------------------
| Name     | Type | Start     | Machine  | Note                               |
-------------------------------------------------------------------------------
| DAM+2    |  ?   | AutoRun   | 2,2A,3   | standard relocatable DAM           |
| DAM+2.1  |  X   |           |          | \                                  |
| DAM+2.2  |  X   |           |          |  \ DAM+2 manual written in         |
| DAM+2.3  |  X   |           |          |  / the KASWORD text editor         |
| DAM+2.3  |  X   |           |          | /                                  |
| TRANSFER |  ?   | JUMP 0000 | 2,2A,3   | src. text converter from DAM-0000  |
| TRANSFER |  X   |           |          | manual for the TRANSFER utility    |
| OPTIMAX  |  ?   | JUMP 0000 | 2,2A,3   | source text optimizer              |
| RESRC2   |  ?   | AutoRun   | 2,2A,3   | disassembly into source text       |
| TEXTER   |  A   |           |          | source - convert data to src. text |
| EXTENDCH |  A   |           |          | source - show chars with diacrit.  |
| JOYDEMO  |  A   |           |          | source - 4004/482 joystick test    |
| JOYSTICK |  >   | RUN       | 1,2,2A,3 | BASIC - joystick wiring diagram    |
| ALLRAM2A |  ?   | JUMP 7000 | 2A       | switch to AllRAM on PMD 85-2A      |
| DAM+9000 |  ?   | JUMP 9000 | 2A,3     | DAM from address 9000H             |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* devutils.ptp *
-------------------------------------------------------------------------------
- various development utilities

+ VMON9000 is a version that runs in the extended memory of the PMD 85-2A or
  PMD 85-3 computers, so the entire lower part of the memory remains free for
  the debugged program. On the PMD 85-3 it is of course necessary to switch to
  compatibility mode first by entering the command JUMP FFF0. On the PMD 85-2A
  it is necessary to switch to AllRAM mode first by running the short program
  "ALLRAM2A".
  In both cases, the MONITOR (8000H to 8FFFH) is already in RAM and may
  therefore be accidentally overwritten!
-------------------------------------------------------------------------------
| Name     | Start     | Machine  | Author / Note                             |
-------------------------------------------------------------------------------
| DDT 6000 | JUMP 6000 | 1,2,2A,3 | ? / monitor, debugger                     |
| VMON 2.1 | JUMP E900 | 1,2,2A,3 | Petr Tůma / screen monitor, debugger      |
| ALLRAM2A | JUMP 7000 | 2A       | switch to AllRAM on PMD 85-2A             |
| VMON9000 | JUMP 9000 | 2A,3     | Petr Tůma / monitor, debugger (ext. mem.) |
| DISASS   | RUN       | 1,2,2A,3 | Pavol Kiko / disassembler in BASIC        |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* karel.ptp *
-------------------------------------------------------------------------------
- robot Karel by various authors

-------------------------------------------------------------------------------
| Name     | Start     | Machine  | Author / Note                             |
-------------------------------------------------------------------------------
| KAREL    | AutoRun   | 1,2,2A,3 | Karel Šuhajda / 2D Karel                  |
| KAREL2D  | RUN       | 1,2,2A   | Tomáš Bartovský / 2D Karel (Basic)        |
| KAREL-2R | JUMP 0000 | 1        | Tomáš Bartovský / 2D Karel                |
| KAREL3D0 | JUMP 0000 | 1,2,2A,3 | Marián Vittek / 3D Karel v0.0             |
| KAREL3.2 | JUMP 0000 | 1,2,2A,3 | Marián Vittek / 3D Karel v2.0             |
| KAREL3D2 | JUMP 0000 | 1,2,2A,3 | Marián Vittek / 3D Karel v3.2             |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* karel2D3D.ptp *
-------------------------------------------------------------------------------
- Robot Karel by various authors, with AutoRun

-------------------------------------------------------------------------------
| Name     | Start     | Machine  | Author / Note                             |
-------------------------------------------------------------------------------
| KAREL    | AutoRun   | 1,2,2A,3 | Karel Šuhajda / 2D Karel                  |
| KAREL3D0 | AutoRun   | 1,2,2A,3 | Marián Vittek / 3D Karel v0.0             |
| KAREL3.2 | AutoRun   | 1,2,2A,3 | Marián Vittek / 3D Karel v2.0             |
| KAREL3D2 | AutoRun   | 1,2,2A,3 | Marián Vittek / 3D Karel v3.2             |
-------------------------------------------------------------------------------


-------------------------------------------------------------------------------
* mrs.ptp *
-------------------------------------------------------------------------------
- UAK Bratislava
- Memory Resident System - System for creating and debugging programs
  in assembly language
-------------------------------------------------------------------------------
| Name     | Start     | Machine  | Note                                      |
-------------------------------------------------------------------------------
| MRS      | JUMP 6000 | 1        |                                           |
| MRS-NOVE | JUMP 6000 | 1        | modified version                          |
| MRS2     | JUMP 6000 | 2,2A,3   | version for PMD 85-2, 2A and 3            |
| LIB      |           |          | library                                   |
| BZZZZT   |           |          | source - buzzing from the game Hlípa      |
-------------------------------------------------------------------------------
