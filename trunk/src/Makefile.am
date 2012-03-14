# GPMD85emu automake
# mborik, 10/2011, 02/2012

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../gui \
	@libSDL_CFLAGS@

noinst_LIBRARIES = libGPMD85emu_core.a

libGPMD85emu_core_a_SOURCES = \
	CommonUtils.cpp \
	GPMD85main.cpp \
	ChipCpu8080.cpp \
	ChipMemory.cpp \
	ChipPIO8255.cpp \
	ChipPIT8253.cpp \
	ChipUSART8251.cpp \
	IifGPIO.cpp \
	IifTape.cpp \
	IifTimer.cpp \
	Pmd32.cpp \
	RaomModule.cpp \
	RomModule.cpp \
	Settings.cpp \
	ScreenPMD85.cpp \
	SoundDriver.cpp \
	SystemPIO.cpp \
	TapeBrowser.cpp \
	Debugger.cpp

noinst_HEADERS = \
	macros8080.h \
	CommonUtils.h \
	GPMD85main.h \
	ChipCpu8080.h \
	ChipMemory.h \
	ChipPIO8255.h \
	ChipPIT8253.h \
	ChipUSART8251.h \
	IifGPIO.h \
	IifTape.h \
	IifTimer.h \
	InterruptController.h \
	PeripheralDevice.h \
	Pmd32.h \
	RaomModule.h \
	RomModule.h \
	Settings.h \
	ScreenPMD85.h \
	SoundDriver.h \
	SystemPIO.h \
	TapeBrowser.h \
	Debugger.h
