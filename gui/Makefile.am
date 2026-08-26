# GPMD85emu automake by mborik

AUTOMAKE_OPTIONS = foreign subdir-objects
EXTRA_DIST = update-submodules.sh

AM_CPPFLAGS = \
	-DIMGUI_USER_CONFIG='"$(srcdir)/custom_imconfig.h"' \
	-I$(srcdir)/.. \
	-I$(srcdir)/../saa \
	-I$(srcdir)/../src \
	-I$(srcdir)/imgui \
	@libSDL_CFLAGS@

noinst_LIBRARIES = libGPMD85emu_gui.a

libGPMD85emu_gui_a_SOURCES = \
	custom_imconfig.h \
	imgui/backends/imgui_impl_sdl2.cpp \
	imgui/backends/imgui_impl_sdl2.h \
	imgui/backends/imgui_impl_opengl3.cpp \
	imgui/backends/imgui_impl_opengl3.h \
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
	imgui/misc/cpp/imgui_stdlib.h \
	imgui_memory_editor.h \
	portable-file-dialogs.h \
	CommonDialog.cpp \
	DebugWindow.cpp \
	EditDialog.cpp \
	Menu.cpp \
	FileSelector.cpp \
	StatusBar.cpp \
	TapeDialog.cpp \
	UserInterface.cpp \
	UserInterfaceData.cpp

noinst_HEADERS = \
	UserInterface.h \
	UserInterfaceData.h
