# GPMD85emu automake
# mborik, 10/2011

AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = GPMD85emu

SUBDIRS = \
	src \
	rom \
	res

DIST_SUBDIRS = $(SUBDIRS)

AM_CPPFLAGS = \
	-I$(srcdir)/src \
	@libSDL_CFLAGS@ \
	@libXML_CFLAGS@ \
	-DDIR_RESOURCES="\"${pkgdatadir}\""

GPMD85emu_LDADD = \
	@libSDL_LIBS@ \
	@libXML_LIBS@ \
	src/libGPMD85emu.a

GPMD85emu_DEPENDENCIES = \
	src/libGPMD85emu.a

GPMD85emu_SOURCES = \
	GPMD85emu.cpp

noinst_HEADERS = \
	sigslot.h \
	globals.h \
	config.h

EXTRA_DIST = \
	README \
	AUTHORS \
	ChangeLog
