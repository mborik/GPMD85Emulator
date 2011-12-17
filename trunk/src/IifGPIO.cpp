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
void IifGPIO::resetDevice(int ticks)
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
void IifGPIO::writeToDevice(BYTE port, BYTE value, int ticks)
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
BYTE IifGPIO::readFromDevice(BYTE port, int ticks)
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
