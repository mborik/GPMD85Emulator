/*	IifTapePMD85.h: Derived class for emulation of PMD 85 tape interface
	Copyright (c) 2006-2026 Roman Borik <pmd85emu@gmail.com>

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
#ifndef IifTapePMD85H
#define IifTapePMD85H
//---------------------------------------------------------------------------
#include "globals.h"
#include "IifTape.h"
#include "PeripheralDevice.h"
#include "ChipUSART8251.h"
#include "ChipPIT8253.h"
//---------------------------------------------------------------------------
class IifTapePMD85 : public PeripheralDevice, public IifTape, public ChipUSART8251 {
	public:
		IifTapePMD85(TComputerModel model, TTapeIfType ifType);

		virtual void ResetDevice(int ticks);
		virtual void WriteToDevice(BYTE port, BYTE value, int ticks);
		virtual BYTE ReadFromDevice(BYTE port, int ticks);

		void TapeClockService123(int ticks, int dur);
		void TapeClockService23(TPITCounter counter, bool outState);

	private:
		void FnOnRtsSet();
		void FnOnTxRChange();
		void InitTapeTx();
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
