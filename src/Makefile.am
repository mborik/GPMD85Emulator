# GPMD85emu automake
# mborik, 10/2011, 02/2012, 01/2018

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../gui \
	-I$(srcdir)/../saa \
	@libSDL_CFLAGS@

noinst_LIBRARIES = libGPMD85emu_core.a

libGPMD85emu_core_a_SOURCES = \
	ArgvParser.cpp \
	CommonUtils.cpp \
	ChipCpu8080.cpp \
	ChipCpu8080P.cpp \
	ChipMemory.cpp \
	ChipMemory12.cpp \
	ChipMemory2A.cpp \
	ChipMemory2AEx.cpp \
	ChipMemory3.cpp \
	ChipMemory3Ex.cpp \
	ChipMemoryC2717.cpp \
	ChipPIO8255.cpp \
	ChipPIT8253.cpp \
	ChipUSART8251.cpp \
	IifGPIO.cpp \
	IifTape.cpp \
	IifTimer.cpp \
	Mif85.cpp \
	Pmd32.cpp \
	RomModule.cpp \
	Settings.cpp \
	ScreenPMD85.cpp \
	SoundDriver.cpp \
	SystemPIO.cpp \
	TapeBrowser.cpp \
	Emulator.cpp \
	Debugger.cpp

noinst_HEADERS = \
	macros8080.h \
	ArgvParser.h \
	CommonUtils.h \
	ChipCpu8080.h \
	ChipCpu8080P.h \
	ChipMemory.h \
	ChipMemory12.h \
	ChipMemory2A.h \
	ChipMemory2AEx.h \
	ChipMemory3.h \
	ChipMemory3Ex.h \
	ChipMemoryC2717.h \
	ChipPIO8255.h \
	ChipPIT8253.h \
	ChipUSART8251.h \
	IifGPIO.h \
	IifTape.h \
	IifTimer.h \
	InterruptController.h \
	Mif85.h \
	PeripheralDevice.h \
	Pmd32.h \
	RomModule.h \
	Settings.h \
	ScreenPMD85.h \
	SoundDriver.h \
	SystemPIO.h \
	TapeBrowser.h \
	Emulator.h \
	Debugger.h
