# GPMD85emu automake
# mborik, 02/2012, 08/2026

AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = update-submodules.sh

AM_CPPFLAGS = \
	-DIMGUI_USER_CONFIG='"$(srcdir)/custom_imconfig.h"' \
	-I$(srcdir)/imgui \
	@libSDL_CFLAGS@

noinst_LIBRARIES = libimgui.a

libimgui_a_SOURCES = \
	custom_imconfig.h \
	imgui/backends/imgui_impl_sdl2.cpp \
	imgui/backends/imgui_impl_sdl2.h \
	imgui/backends/imgui_impl_sdlrenderer2.cpp \
	imgui/backends/imgui_impl_sdlrenderer2.h \
	imgui/imconfig.h \
	imgui/imgui.cpp \
	imgui/imgui.h \
	imgui/imgui_draw.cpp \
	imgui/imgui_internal.h \
	imgui/imgui_tables.cpp \
	imgui/imgui_widgets.cpp \
	imgui/imstb_rectpack.h \
	imgui/imstb_textedit.h \
	imgui/imstb_truetype.h \
	imgui/misc/cpp/imgui_raii.h \
	imgui/misc/cpp/imgui_stdlib.cpp \
	imgui/misc/cpp/imgui_stdlib.h
