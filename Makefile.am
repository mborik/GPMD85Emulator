# GPMD85emu automake by mborik

AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = GPMD85emu

SUBDIRS = \
	saa \
	src \
	gui \
	rom \
	res

DIST_SUBDIRS = $(SUBDIRS)

AM_CPPFLAGS = \
	-I$(srcdir)/saa \
	-I$(srcdir)/src \
	-I$(srcdir)/gui \
	-I$(srcdir)/gui/imgui \
	-I$(srcdir)/gui/imgui/backends \
	-I$(srcdir)/gui/imgui-mods \
	@libSDL_CFLAGS@ \
	@libGL_CFLAGS@ \
	@resDir_CFLAGS@

GPMD85emu_LDADD = \
	@libSDL_LIBS@ \
	@libGL_LIBS@ \
	src/libGPMD85emu_core.a \
	gui/libGPMD85emu_gui.a \
	saa/libSAASound.a

GPMD85emu_DEPENDENCIES = \
	src/libGPMD85emu_core.a \
	gui/libGPMD85emu_gui.a \
	saa/libSAASound.a

GPMD85emu_SOURCES = \
	main.cpp \
	sigslot.hpp

noinst_HEADERS = \
	globals.h \
	config.h

EXTRA_DIST = \
	AUTHORS.md \
	INSTALL.md \
	LICENSE.md \
	README.md
