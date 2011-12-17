//---------------------------------------------------------------------------
#ifndef IifGPIOH
#define IifGPIOH
//---------------------------------------------------------------------------
#include "globals.h"
#include "PeripheralDevice.h"
#include "ChipPIO8255.h"
//---------------------------------------------------------------------------
#define IIF_GPIO_MASK       0xFC
#define IIF_GPIO_ADR        0x4C

#define IIF_GPIO_REG_MASK   0xFF
#define IIF_GPIO_REG_A      0x4C
#define IIF_GPIO_REG_B      0x4D
#define IIF_GPIO_REG_C      0x4E
#define IIF_GPIO_REG_CWR    0x4F
//---------------------------------------------------------------------------
class IifGPIO: public PeripheralDevice, public ChipPIO8255
{
public:
	IifGPIO();

	virtual void resetDevice(int ticks);
	virtual void writeToDevice(BYTE port, BYTE value, int ticks);
	virtual BYTE readFromDevice(BYTE port, int ticks);

	sigslot::signal0<> OnBeforeResetA;
	sigslot::signal0<> OnBeforeResetB;
	sigslot::signal0<> OnAfterResetA;
	sigslot::signal0<> OnAfterResetB;

	void WriteByte(TPIOPort dest, BYTE val);
	BYTE ReadByte(TPIOPort src);
	void ChangeBit(TPIOPort dest, TPIOPortBit bit, bool state);
	bool ReadBit(TPIOPort src, TPIOPortBit bit);

	inline int GetCurrentTicks() { return currentTicks; }

private:
	int currentTicks;
};
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------

