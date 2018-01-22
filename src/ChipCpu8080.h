/*	ChipCpu8080.h: Class for emulation of i8080 microprocessor
	Copyright (c) 2006-2015 Roman Borik <pmd85emu@gmail.com>

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
// instruction duration definition according to the processor-cycle lengths
#define TR4            4
#define TR4R3          7
#define TR4R3R3       10
#define TR4R3R3R3     13
#define TR4R3R3R3R3   16
#define TR4W3          7
#define TR4R3W3       10
#define TR4R3R3W3     13
#define TR4R3R3W3W3   16
#define TR4R3R3W3W5   18
#define TR5            5
#define TR5R3R3       11
#define TR5R3R3W3W3   17
#define TR5W3W3       11
#define TR4N3N3       10
#define TR4N1          5
//---------------------------------------------------------------------------
class ChipCpu8080 : public sigslot::has_slots<> {
public:
	ChipCpu8080(ChipMemory *mem);
	virtual ~ChipCpu8080();

	// some of CPU signals
	inline void DoReset() { resetPending = true; } // RESET signal
	int         DoInstruction();                   // instruction execution
	bool        DoInterrupt();                     // INT signal

	// I/O devices
	void AddDevice(BYTE portAddr, BYTE portMask,
	               PeripheralDevice* device, bool needReset);
	void RemoveDevice(BYTE portAddr);

	// "Fi2TTL" listeners
	sigslot::signal2<int, int> TCyclesListeners;

	// interupt controller
	void SetInterruptController(InterruptController *intCtrl);

	// share the memory object
	inline ChipMemory *GetMemory() { return memory; }

	// INTE output signal
	inline bool IsInterruptEnabled() { return iff; }

	// processor registers
	inline void SetAF(WORD rAF) { AF = rAF; }
	inline void SetBC(WORD rBC) { BC = rBC; }
	inline void SetDE(WORD rDE) { DE = rDE; }
	inline void SetHL(WORD rHL) { HL = rHL; }
	inline void SetSP(WORD rSP) { SP = rSP; }
	inline void SetPC(WORD rPC) { PC = rPC; }
	inline void SetIff(bool rIff) { iff = rIff; ei1 = ei2 = false; }

	inline WORD GetAF() { return AF; }
	inline WORD GetBC() { return BC; }
	inline WORD GetDE() { return DE; }
	inline WORD GetHL() { return HL; }
	inline WORD GetSP() { return SP; }
	inline WORD GetPC() { return PC; }

	void SetChipState(BYTE *buffer);
	int  GetChipState(BYTE *buffer);

	// processor T-Cycles
	inline int  GetTCycles() { return countTCycles; }
	inline void SetTCycles(int count) { countTCycles = count; }
	inline void IncTCycles() { countTCycles++; lastInstrTCycles++; totalTCycles++; }
	inline int  GetTCyclesTotal() { return totalTCycles; }
	inline void ClearTCyclesTotal() { totalTCycles = 0; }
	inline int  GetIntTCyclesMin() { return intCyclesMin; }
	inline int  GetIntTCyclesMax() { return intCyclesMax; }
	inline int  GetIntTCyclesAvg() { return intCyclesAvg; }
	inline int  GetFetchCount() { return fetchCounter; }
	inline void ClearFetchCount() { fetchCounter = 0; }
	inline bool IsSlowCpu() { return slowCpu; }
	inline void SetSlowCpu(bool state) { slowCpu = state; }
	inline int  GetLength(BYTE opcode) { return length[opcode]; }

	void InitCountIntCycles();

private:
	// I/O devices
	PeripheralDevice* FindDevice(BYTE port);

	// prepare of table of flags
	void PrepareFlagsTables();
	void CountIntCycles();
	void Reset();

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
	bool resetPending;       // request for reset is pending

	int totalTCycles;        // T-Cycles counter (total)
	int lastInstrTCycles;    // T-Cycles from last instruction
	int fetchCounter;        // count of executed instructions

	int intSP;               // stack pointer before interrupt was executed
	int intCyclesCur;        // current T-Cycles counter before interrupt
	int intCyclesMin;        // min duration of interrupt routine
	int intCyclesMax;        // max duration of interrupt routine
	int intCyclesAvg;        // avg duration of interrupt routine

	static int length[256];

	// tables for flags examining
	static BYTE auxcarryAddTable[8];
	static BYTE auxcarrySubTable[8];
	static BYTE sz53p1Table[0x100];
	static WORD daaTable[0x400];

	ChipMemory *memory;             // memory object pointer
	InterruptController *intCtrl;   // interupt controller object pointer

	PORT_HANDLER *ports;            // port list pointer
	PORT_HANDLER *lastPort;         // last port in list

protected:
// members to share with children implementations...
	bool slowCpu;                   // slowed-down CPU state
	int  countTCycles;              // T-Cycles counter

	static int duration[256];       // instruction durations
	virtual int GetDuration(BYTE opcode, bool jmp);
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

