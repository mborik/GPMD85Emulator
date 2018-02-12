/*	RaomModule.h: Class for emulation of RAOM modules
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

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
#ifndef RaomModuleH
#define RaomModuleH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIO8255.h"
//---------------------------------------------------------------------------
#define RAOM_MODULE_MASK          0x8C
#define RAOM_MODULE_ADR           0x88

#define RAOM_REG_MASK             0xFF

#define RAOM_REG_DATA_CHTF        0x88
#define RAOM_REG_LOW_CHTF         0x98
#define RAOM_REG_MID_CHTF         0x99
#define RAOM_REG_HIGH_CHTF        0x9A
#define RAOM_REG_CWR_CHTF         0x9B

#define RAOM_REG_DATA_KUVI        0xE8
#define RAOM_REG_HIGH_KUVI        0xF8
#define RAOM_REG_ROMD_KUVI        0xF8
#define RAOM_REG_LOW_KUVI         0xF9
#define RAOM_REG_MID_KUVI         0xFA
#define RAOM_REG_CWR_KUVI         0xFB

#define CHTF_ROM_SIZE_KB          64
#define CHTF_ROM_SIZE             (CHTF_ROM_SIZE_KB * KB)
#define CHTF_RAM_SIZE_KB          192
#define CHTF_RAM_SIZE             (CHTF_RAM_SIZE_KB * KB)

#define KUVI_ROM_SIZE_KB          256
#define KUVI_ROM_SIZE             (KUVI_ROM_SIZE_KB * KB)
#define KUVI_RAM_SIZE_KB          256
#define KUVI_RAM_SIZE             (KUVI_RAM_SIZE_KB * KB)

#define RAM_BLOCK_SIZE_KB         16
#define RAM_BLOCK_SIZE            (RAM_BLOCK_SIZE_KB * KB)
//---------------------------------------------------------------------------
class RaomModule : public PeripheralDevice, public ChipPIO8255 {
	public:
		RaomModule(TRaomType type);
		virtual ~RaomModule();

		virtual void ResetDevice(int ticks);
		virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
		virtual BYTE ReadFromDevice(BYTE port, int ticks);

		void InsertDisk(char *file);
		void RemoveDisk();

		inline int GetRomSize() { return romSize; }
		bool InsertRom(BYTE addressKB, BYTE sizeKB, BYTE *src);
		void RemoveRom(BYTE addressKB, BYTE sizeKB);
		void RemoveRomPack();

		inline int IsWriteProtect() { return writeProtect; }
		inline void SetWriteProtect(bool writeProtect) { this->writeProtect = writeProtect; }

	private:
		TRaomType type;
		int addr;
		BYTE cwr;

		BYTE *RomPack;
		int romSize;

		FILE *hFile;
		BYTE *RamBlock;
		int ramAddr;
		int ramSize;
		bool writeRamBlock;
		bool writeProtect;
		DWORD dwRW;

		void ComposeAdddress();
		void ReadBlock();
		void WriteBlock();
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
