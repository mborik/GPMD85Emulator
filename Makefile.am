# GPMD85emu automake
# mborik, 10/2011, 02/2012, 01/2018

AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = GPMD85emu

SUBDIRS = \
	src \
	gui \
	rom \
	res

DIST_SUBDIRS = $(SUBDIRS)

AM_CPPFLAGS = \
	-I$(srcdir)/src \
	-I$(srcdir)/gui \
	@libSDL_CFLAGS@ \
	@resDir_CFLAGS@

GPMD85emu_LDADD = \
	@libSDL_LIBS@ \
	src/libGPMD85emu_core.a \
	gui/libGPMD85emu_gui.a

GPMD85emu_DEPENDENCIES = \
	src/libGPMD85emu_core.a \
	gui/libGPMD85emu_gui.a

GPMD85emu_SOURCES = \
	GPMD85emu.cpp

noinst_HEADERS = \
	sigslot.h \
	globals.h \
	config.h

EXTRA_DIST = \
	AUTHORS \
	INSTALL.md \
	README.md \
	ChangeLog
