/*	IifGPIO.cpp: Class for emulation of base GPIO interface
	Copyright (c) 2006 Roman Borik <pmd85emu@gmail.com>

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom
	the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
	THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
	OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
	ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
	OR OTHER DEALINGS IN THE SOFTWARE.
*/
//---------------------------------------------------------------------------
#include "IifGPIO.h"
#include "CommonUtils.h"
//---------------------------------------------------------------------------
IifGPIO::IifGPIO() : ChipPIO8255(true)
{
	OnBeforeResetA.disconnect_all();
	OnBeforeResetB.disconnect_all();
	OnAfterResetA.disconnect_all();
	OnAfterResetB.disconnect_all();
}
//---------------------------------------------------------------------------
// metody zdedene z triedy PeripheralDevice
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri jeho resete.
 */
void IifGPIO::ResetDevice(int ticks)
{
	currentTicks = ticks;

	OnBeforeResetA();
	OnBeforeResetB();

	ChipReset(false);

	OnAfterResetA();
	OnAfterResetB();
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri zapise na porty GPIO.
 */
void IifGPIO::WriteToDevice(BYTE port, BYTE value, int ticks)
{
	currentTicks = ticks;

	switch (port & IIF_GPIO_REG_MASK) {
		case IIF_GPIO_REG_A:
			CpuWrite(PP_PortA, value);
			break;

		case IIF_GPIO_REG_B:
			CpuWrite(PP_PortB, value);
			break;

		case IIF_GPIO_REG_C:
			CpuWrite(PP_PortC, value);
			break;

		case IIF_GPIO_REG_CWR:
			CpuWrite(PP_CWR, value);
			break;
	}
}
//---------------------------------------------------------------------------
/**
 * Metoda je volana procesorom pri citani z portov GPIO.
 */
BYTE IifGPIO::ReadFromDevice(BYTE port, int ticks)
{
	BYTE retval;

	currentTicks = ticks;

	switch (port & IIF_GPIO_REG_MASK) {
		case IIF_GPIO_REG_A:
			retval = CpuRead(PP_PortA);
			break;

		case IIF_GPIO_REG_B:
			retval = CpuRead(PP_PortB);
			break;

		case IIF_GPIO_REG_C:
			retval = CpuRead(PP_PortC);
			break;

		case IIF_GPIO_REG_CWR:
			retval = CpuRead(PP_CWR);
			break;

		default:
			retval = 0xFF;
			break;
	}

	return retval;
}
//---------------------------------------------------------------------------
void IifGPIO::WriteByte(TPIOPort dest, BYTE val)
{
	PeripheralWriteByte(dest, val);
}
//---------------------------------------------------------------------------
BYTE IifGPIO::ReadByte(TPIOPort src)
{
	return PeripheralReadByte(src);
}
//---------------------------------------------------------------------------
void IifGPIO::ChangeBit(TPIOPort dest, TPIOPortBit bit, bool state)
{
	PeripheralChangeBit(dest, bit, state);
}
//---------------------------------------------------------------------------
bool IifGPIO::ReadBit(TPIOPort src, TPIOPortBit bit)
{
	return PeripheralReadBit(src, bit);
}
//---------------------------------------------------------------------------
