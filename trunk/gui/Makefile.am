# GPMD85emu automake
# mborik, 02/2012

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = \
	-I$(srcdir)/.. \
	-I$(srcdir)/../src \
	@libSDL_CFLAGS@

noinst_LIBRARIES = libGPMD85emu_gui.a

libGPMD85emu_gui_a_SOURCES = \
	CommonDialog.cpp \
	EditDialog.cpp \
	Menu.cpp \
	FileSelector.cpp \
	TapeDialog.cpp \
	UserInterface.cpp \
	UserInterfaceData.cpp

noinst_HEADERS = \
	UserInterface.h \
	UserInterfaceData.h
