/*	ChipCpu8080.h: Class for emulation of i8080 microprocessor
	Copyright (c) 2006-2010 Roman Borik <pmd85emu@gmail.com>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
//---------------------------------------------------------------------------
#ifndef ChipCpu8080H
#define ChipCpu8080H
//---------------------------------------------------------------------------
#include "globals.h"
#include "macros8080.h"
#include "ChipMemory.h"
#include "PeripheralDevice.h"
#include "InterruptController.h"
//---------------------------------------------------------------------------
class ChipCpu8080 : public sigslot::has_slots<> {
public:
	ChipCpu8080(ChipMemory *mem);
	virtual ~ChipCpu8080();

	// some of CPU signals
	void Reset();             // RESET signal
	int  DoInstruction();     // one instruction execution
	bool DoInterrupt();       // INT signal

	// INTE output signal
	inline bool IsInterruptEnabled() { return iff; }

	// I/O devices
	void AddDevice(BYTE portAddr, BYTE portMask, PeripheralDevice* device, bool needReset);
	void RemoveDevice(BYTE portAddr);

	// "Fi2TTL" listeners
	sigslot::signal2<int, int> TCyclesListeners;

	// interupt controller
	void SetInterruptController(InterruptController *intContr);

	// processor registers
	inline void SetAF(WORD rAF) { AF = rAF; }
	inline void SetBC(WORD rBC) { BC = rBC; }
	inline void SetDE(WORD rDE) { DE = rDE; }
	inline void SetHL(WORD rHL) { HL = rHL; }
	inline void SetSP(WORD rSP) { SP = rSP; }
	inline void SetPC(WORD rPC) { PC = rPC; }
	void SetIff(bool rIff);

	inline WORD GetAF() { return AF; }
	inline WORD GetBC() { return BC; }
	inline WORD GetDE() { return DE; }
	inline WORD GetHL() { return HL; }
	inline WORD GetSP() { return SP; }
	inline WORD GetPC() { return PC; }

	void SetChipState(BYTE *buffer);
	int GetChipState(BYTE *buffer);

	// processor T-Cycles
	inline int GetTCycles() { return TCycles; }
	inline void SetTCycles(int TCycles) { this->TCycles = TCycles; }
	inline void IncTCycles() { TCycles++; lastInstrTCycles++; TCyclesTotal++; }
	inline int GetTCyclesTotal() { return TCyclesTotal; }
	inline void ClearTCyclesTotal() { TCyclesTotal = 0; }

	inline int GetLength(BYTE opcode) { return length[opcode]; }
	const char* GetMnemonics(BYTE opcode, bool asZ80);

private:
	// I/O devices
	PeripheralDevice* FindDevice(BYTE port);

	// prepare of table of flags
	void PrepareFlagsTables();

	// struct for list of connected I/O devices
	typedef struct PortHandler {
		BYTE portAddr;             // port address
		BYTE portMask;             // port mask
		PeripheralDevice *device;  // pointer to device object
		bool needReset;            // device need reset
		PortHandler *next;         // pointer to next list item
	} PORT_HANDLER;

	// register pair type
	typedef union {
		struct {
			BYTE l;
			BYTE h;
		} b;
		WORD w;
	} RegPair;

	// processor registers
	RegPair af;              // S Z 0 AC 0 PE 1 CY
	RegPair bc;
	RegPair de;
	RegPair hl;
	RegPair sp;
	RegPair pc;

	// flip-flop
	bool iff;                // enabled/disabled interupt
	bool ei1;                // has been executed EI instruction
	bool ei2;                // is necessary to enable interupt?
	bool halt;               // processor halted
	bool inta;               // has been accepted interupt

	// semafory
	bool isexe;              // instruction is executing?
	bool isint;              // instruction is executing after interupt?

	int TCycles;             // T-Cycles counter
	int TCyclesTotal;        // T-Cycles counter (total)
	int lastInstrTCycles;    // T-Cycles from last instruction

	int duration[256];
	int length[256];

	char instr8080[256][16]; // 8080 mnemonic
	char instrZ80[256][16];  // Z80 mnemonic

	// tables for flags examining
	BYTE auxcarryAddTable[8];
	BYTE auxcarrySubTable[8];
	BYTE sz53p1Table[0x100];
	WORD daaTable[0x400];

	ChipMemory *memory;             // memory object pointer
	InterruptController *intContr;  // interupt controller object pointer

	PORT_HANDLER *Ports;            // port list pointer
	PORT_HANDLER *LastPort;         // last port in list
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

