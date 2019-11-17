# SAASound automake
# mborik, 11/2019

AUTOMAKE_OPTIONS = foreign
noinst_LIBRARIES = libSAASound.a

AM_CPPFLAGS = -I$(srcdir)

libSAASound_a_SOURCES = \
	SAAAmp.cpp \
	SAAEnv.cpp \
	SAAFreq.cpp \
	SAANoise.cpp \
	SAAImpl.cpp

noinst_HEADERS = \
	types.h \
	SAAAmp.h \
	SAAEnv.h \
	SAAFreq.h \
	SAANoise.h \
	SAAImpl.h \
	SAASound.h

EXTRA_DIST = \
	LICENSE \
	README.md
