cmake_minimum_required(VERSION 3.20)

project(GPMD85emulator
	VERSION 1.0.2610
	DESCRIPTION "Open-source, multi-platform emulator of the Tesla PMD 85"
	HOMEPAGE_URL "https://github.com/mborik/GPMD85Emulator"
	LANGUAGES C CXX
)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
	message(STATUS "Detecting default macOS deployment target from compiler...")
	file(WRITE "${CMAKE_BINARY_DIR}/target_test.c" "int main() { return 0; }\n")
	execute_process(
		COMMAND ${CMAKE_C_COMPILER} "${CMAKE_BINARY_DIR}/target_test.c" -o "${CMAKE_BINARY_DIR}/target_test_bin"
		OUTPUT_QUIET ERROR_QUIET
	)
	if(EXISTS "${CMAKE_BINARY_DIR}/target_test_bin")
		execute_process(
			COMMAND otool -l "${CMAKE_BINARY_DIR}/target_test_bin"
			COMMAND grep -A 2 minos
			COMMAND awk "/minos/ {print $2}"
			OUTPUT_VARIABLE DETECTED_TARGET
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		if(NOT DETECTED_TARGET)
			execute_process(
				COMMAND otool -l "${CMAKE_BINARY_DIR}/target_test_bin"
				COMMAND grep -A 3 LC_VERSION_MIN_MACOSX
				COMMAND awk "/version/ {print $2}"
				OUTPUT_VARIABLE DETECTED_TARGET
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)
		endif()
	endif()
	file(REMOVE "${CMAKE_BINARY_DIR}/target_test.c" "${CMAKE_BINARY_DIR}/target_test_bin")

	if(DETECTED_TARGET)
		set(CMAKE_OSX_DEPLOYMENT_TARGET "${DETECTED_TARGET}" CACHE STRING "macOS deployment target" FORCE)
	else()
		set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS deployment target" FORCE)
	endif()
endif()


include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckStructHasMember)
include(CheckCXXSourceCompiles)
include(TestBigEndian)
include(FindPkgConfig)

option(DEBUG "Enable debug build flags" OFF)
option(TRACE "Compile trace messages" ON)
option(INSTALLED_RESOURCES "Use install datadir for resource lookup at runtime" OFF)

set(PACKAGE "gpmd85emu")
set(PACKAGE_NAME "GPMD85emulator")
set(PACKAGE_TARNAME "gpmd85emu")
set(PACKAGE_URL "${PROJECT_HOMEPAGE_URL}")
set(PACKAGE_VERSION "${PROJECT_VERSION}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "martin@borik.net")
string(TIMESTAMP PACKAGE_YEAR "%Y")
set(VERSION "${PROJECT_VERSION}")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU|MSVC")
	message(WARNING "This project is primarily tested with Clang and GCC compilers.")
endif()

set(GPMD85EMU_COMMON_COMPILE_OPTIONS -Wno-deprecated)

if(DEBUG)
	if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
	endif()
	add_compile_definitions(DEBUG)
else()
	if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
	endif()
	add_link_options($<$<AND:$<CXX_COMPILER_ID:GNU>,$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>>:-s>)
endif()

if(NOT TRACE AND NOT DEBUG)
	add_compile_definitions(NOTRACEMSG)
endif()

find_package(PkgConfig QUIET)
set(GPMD85EMU_SDL_TARGET "")
if(EMSCRIPTEN)
	# SDL2 is provided by Emscripten ports, WebGL 2 (OpenGL ES 3.0) is used for rendering
	add_library(GPMD85emu_sdl2 INTERFACE)
	target_compile_options(GPMD85emu_sdl2 INTERFACE -sUSE_SDL=2)
	target_link_options(GPMD85emu_sdl2 INTERFACE -sUSE_SDL=2)
	set(GPMD85EMU_SDL_TARGET GPMD85emu_sdl2)
	add_compile_definitions(IMGUI_IMPL_OPENGL_ES3)
elseif(PkgConfig_FOUND)
	pkg_check_modules(SDL2_PKG QUIET IMPORTED_TARGET sdl2)
	if(NOT TARGET PkgConfig::SDL2_PKG)
		pkg_check_modules(SDL2_PKG QUIET IMPORTED_TARGET SDL2)
	endif()
	if(TARGET PkgConfig::SDL2_PKG)
		set(GPMD85EMU_SDL_TARGET PkgConfig::SDL2_PKG)
	endif()
endif()

if(NOT GPMD85EMU_SDL_TARGET)
	find_package(SDL2 REQUIRED)
	if(TARGET SDL2::SDL2)
		set(GPMD85EMU_SDL_TARGET SDL2::SDL2)
	else()
		add_library(GPMD85emu_sdl2 INTERFACE)
		target_include_directories(GPMD85emu_sdl2 INTERFACE ${SDL2_INCLUDE_DIRS})
		target_compile_options(GPMD85emu_sdl2 INTERFACE ${SDL2_CFLAGS_OTHER})
		target_link_options(GPMD85emu_sdl2 INTERFACE ${SDL2_LDFLAGS_OTHER})
		target_link_libraries(GPMD85emu_sdl2 INTERFACE ${SDL2_LIBRARIES})
		set(GPMD85EMU_SDL_TARGET GPMD85emu_sdl2)
	endif()
endif()

set(GPMD85EMU_OPENGL_LIBS "")
if(EMSCRIPTEN)
	# OpenGL is provided by the Emscripten runtime
elseif(APPLE)
	find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
	list(APPEND GPMD85EMU_OPENGL_LIBS ${OPENGL_FRAMEWORK})
else()
	find_package(OpenGL REQUIRED)
	if(TARGET OpenGL::GL)
		list(APPEND GPMD85EMU_OPENGL_LIBS OpenGL::GL)
	else()
		list(APPEND GPMD85EMU_OPENGL_LIBS ${OPENGL_gl_LIBRARY})
	endif()
endif()

check_include_file("sys/syslimits.h" HAVE_SYS_SYSLIMITS_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("sys/ndir.h" HAVE_SYS_NDIR_H)
check_include_file("sys/dir.h" HAVE_SYS_DIR_H)
check_include_file("ndir.h" HAVE_NDIR_H)

if(HAVE_DIRENT_H)
	check_struct_has_member("struct dirent" d_type "dirent.h" HAVE_STRUCT_DIRENT_D_TYPE)
endif()
set(FILESYSTEM_TEST_SOURCE "#include <filesystem>\nint main(){std::filesystem::path p(\"/tmp\");return p.empty();}\n")
set(GPMD85EMU_PROBE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake-probes")
file(MAKE_DIRECTORY "${GPMD85EMU_PROBE_DIR}")
function(gpmd85emu_try_link_filesystem out_var)
	set(test_src "${GPMD85EMU_PROBE_DIR}/${out_var}.cpp")
	file(WRITE "${test_src}" "${FILESYSTEM_TEST_SOURCE}")
	if(ARGC GREATER 1)
		try_compile(${out_var}
			"${CMAKE_CURRENT_BINARY_DIR}"
			"${test_src}"
			CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
			LINK_LIBRARIES ${ARGV1}
		)
	else()
		try_compile(${out_var}
			"${CMAKE_CURRENT_BINARY_DIR}"
			"${test_src}"
			CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
		)
	endif()
	set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()

gpmd85emu_try_link_filesystem(HAVE_STD_FILESYSTEM)

set(GPMD85EMU_FILESYSTEM_LIB "")
if(NOT HAVE_STD_FILESYSTEM)
	gpmd85emu_try_link_filesystem(HAVE_STD_FILESYSTEM_WITH_STDCXXFS stdc++fs)

	if(HAVE_STD_FILESYSTEM_WITH_STDCXXFS)
		set(HAVE_STD_FILESYSTEM 1)
		set(GPMD85EMU_FILESYSTEM_LIB stdc++fs)
	endif()
endif()

if(NOT HAVE_STD_FILESYSTEM)
	set(EXPERIMENTAL_FILESYSTEM_TEST_SOURCE "#include <experimental/filesystem>\nint main(){std::experimental::filesystem::path p(\"/tmp\");return p.empty();}\n")
	set(experimental_test_src "${GPMD85EMU_PROBE_DIR}/HAVE_STD_EXPERIMENTAL_FILESYSTEM.cpp")
	file(WRITE "${experimental_test_src}" "${EXPERIMENTAL_FILESYSTEM_TEST_SOURCE}")
	try_compile(HAVE_STD_EXPERIMENTAL_FILESYSTEM
		"${CMAKE_CURRENT_BINARY_DIR}"
		"${experimental_test_src}"
		CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
	)
	if(NOT HAVE_STD_EXPERIMENTAL_FILESYSTEM)
		try_compile(HAVE_STD_EXPERIMENTAL_FILESYSTEM_WITH_STDCXXFS
			"${CMAKE_CURRENT_BINARY_DIR}"
			"${experimental_test_src}"
			CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
			LINK_LIBRARIES stdc++fs
		)
		if(HAVE_STD_EXPERIMENTAL_FILESYSTEM_WITH_STDCXXFS)
			set(HAVE_STD_EXPERIMENTAL_FILESYSTEM 1)
			if(NOT GPMD85EMU_FILESYSTEM_LIB)
				set(GPMD85EMU_FILESYSTEM_LIB stdc++fs)
			endif()
		else()
			try_compile(HAVE_STD_EXPERIMENTAL_FILESYSTEM_WITH_CXXFS
				"${CMAKE_CURRENT_BINARY_DIR}"
				"${experimental_test_src}"
				CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
				LINK_LIBRARIES c++fs
			)
			if(HAVE_STD_EXPERIMENTAL_FILESYSTEM_WITH_CXXFS)
				set(HAVE_STD_EXPERIMENTAL_FILESYSTEM 1)
				if(NOT GPMD85EMU_FILESYSTEM_LIB)
					set(GPMD85EMU_FILESYSTEM_LIB c++fs)
				endif()
			endif()
		endif()
	endif()
endif()

if(NOT HAVE_STD_FILESYSTEM AND NOT HAVE_STD_EXPERIMENTAL_FILESYSTEM)
	message(FATAL_ERROR "C++17 filesystem support is required but not found (neither std::filesystem nor std::experimental::filesystem is available).")
endif()

set(GPMD85EMU_DL_LIB "")
if(NOT WIN32 AND NOT EMSCRIPTEN)
	check_include_file("dlfcn.h" HAVE_DLFCN_H)
	if(HAVE_DLFCN_H)
		if(CMAKE_DL_LIBS)
			list(APPEND GPMD85EMU_DL_LIB ${CMAKE_DL_LIBS})
		endif()
		set(GPMD85EMU_REQUIRED_LIBRARIES_SAVED "${CMAKE_REQUIRED_LIBRARIES}")
		set(CMAKE_REQUIRED_LIBRARIES ${GPMD85EMU_DL_LIB})
		check_c_source_compiles(
			"#include <dlfcn.h>\nint main(void){void* handle = dlopen(0, RTLD_LAZY); return handle == 0;}\n"
			HAVE_DLOPEN
		)
		set(CMAKE_REQUIRED_LIBRARIES "${GPMD85EMU_REQUIRED_LIBRARIES_SAVED}")
	endif()
	if(NOT HAVE_DLFCN_H OR NOT HAVE_DLOPEN)
		message(FATAL_ERROR "Dynamic linking support (dlopen) is required but not found.")
	endif()
endif()

test_big_endian(WORDS_BIGENDIAN)

if(EMSCRIPTEN)
	set(DIR_RESOURCES "/res")
elseif(INSTALLED_RESOURCES)
	set(DIR_RESOURCES "${CMAKE_INSTALL_FULL_DATADIR}/${PACKAGE_TARNAME}")
else()
	set(DIR_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/res")
endif()

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gui/imgui/imgui.cpp")
	message(FATAL_ERROR "Missing gui/imgui submodule. Initialize submodules before configuring.")
endif()

set(CONFIG_H_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${CONFIG_H_OUTPUT_DIR}")
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
	"${CONFIG_H_OUTPUT_DIR}/config.h"
)

add_library(gpmd85emu_core STATIC
	src/ArgvParser.cpp
	src/CommonUtils.cpp
	src/ChipCpu8080.cpp
	src/ChipCpu8080P.cpp
	src/ChipMemory.cpp
	src/ChipMemory12.cpp
	src/ChipMemory2A.cpp
	src/ChipMemory2AEx.cpp
	src/ChipMemory3.cpp
	src/ChipMemory3Ex.cpp
	src/ChipMemoryC2717.cpp
	src/ChipMemoryMato.cpp
	src/ChipPIO8255.cpp
	src/ChipPIT8253.cpp
	src/ChipUSART8251.cpp
	src/IifGPIO.cpp
	src/IifTape.cpp
	src/IifTapeMato.cpp
	src/IifTapePMD85.cpp
	src/IifTimer.cpp
	src/Joy4004482.cpp
	src/Mif85.cpp
	src/Mouse602.cpp
	src/Pmd32.cpp
	src/RomModule.cpp
	src/RomMegaModule.cpp
	src/Settings.cpp
	src/ScreenPMD85.cpp
	src/SoundDriver.cpp
	src/SystemPIO.cpp
	src/TapeBrowser.cpp
	src/Emulator.cpp
	src/Debugger.cpp
)

target_include_directories(gpmd85emu_core
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CONFIG_H_OUTPUT_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_SOURCE_DIR}/gui"
		"${CMAKE_CURRENT_SOURCE_DIR}/saa"
)
target_compile_options(gpmd85emu_core PRIVATE ${GPMD85EMU_COMMON_COMPILE_OPTIONS})
target_link_libraries(gpmd85emu_core PUBLIC ${GPMD85EMU_SDL_TARGET})

add_library(saasound STATIC
	saa/SAAAmp.cpp
	saa/SAAEnv.cpp
	saa/SAAFreq.cpp
	saa/SAANoise.cpp
	saa/SAAImpl.cpp
)
target_include_directories(saasound PUBLIC
	"${CONFIG_H_OUTPUT_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/saa"
)
target_compile_options(saasound PRIVATE ${GPMD85EMU_COMMON_COMPILE_OPTIONS})

add_library(gpmd85emu_gui STATIC
	gui/imgui/backends/imgui_impl_sdl2.cpp
	gui/imgui/backends/imgui_impl_opengl3.cpp
	gui/imgui/imgui.cpp
	gui/imgui/imgui_draw.cpp
	gui/imgui/imgui_tables.cpp
	gui/imgui/imgui_widgets.cpp
	gui/imgui/misc/cpp/imgui_stdlib.cpp
	gui/AboutDialog.cpp
	gui/CommonDialog.cpp
	gui/DebugSuite.cpp
	gui/Menu.cpp
	gui/MemDumpDialog.cpp
	gui/FileSelector.cpp
	gui/StatusBar.cpp
	gui/TapeDialog.cpp
	gui/UserInterface.cpp
)

target_include_directories(gpmd85emu_gui
	PUBLIC
		"${CONFIG_H_OUTPUT_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/saa"
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${CMAKE_CURRENT_SOURCE_DIR}/gui"
		"${CMAKE_CURRENT_SOURCE_DIR}/gui/imgui"
		"${CMAKE_CURRENT_SOURCE_DIR}/gui/imgui/backends"
		"${CMAKE_CURRENT_SOURCE_DIR}/gui/imgui-mods"
)
target_compile_definitions(gpmd85emu_gui PUBLIC IMGUI_USER_CONFIG=\"imgui-mods/imconfig_mod.h\")
target_compile_options(gpmd85emu_gui PRIVATE ${GPMD85EMU_COMMON_COMPILE_OPTIONS})
target_link_libraries(gpmd85emu_gui PUBLIC ${GPMD85EMU_SDL_TARGET})

add_executable(GPMD85emu
	main.cpp
)

target_include_directories(GPMD85emu PRIVATE
	"${CONFIG_H_OUTPUT_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/src"
	"${CMAKE_CURRENT_SOURCE_DIR}/gui"
	"${CMAKE_CURRENT_SOURCE_DIR}/saa"
)
target_compile_definitions(GPMD85emu PRIVATE DIR_RESOURCES="${DIR_RESOURCES}")
target_compile_options(GPMD85emu PRIVATE ${GPMD85EMU_COMMON_COMPILE_OPTIONS})

target_link_libraries(GPMD85emu PRIVATE
	gpmd85emu_core
	gpmd85emu_gui
	saasound
	${GPMD85EMU_SDL_TARGET}
	${GPMD85EMU_OPENGL_LIBS}
	${GPMD85EMU_FILESYSTEM_LIB}
	${GPMD85EMU_DL_LIB}
)

if(EMSCRIPTEN)
	set_target_properties(GPMD85emu PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
	target_link_options(GPMD85emu PRIVATE
		-sUSE_SDL=2
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sALLOW_MEMORY_GROWTH=1
		-sEXIT_RUNTIME=0
		-sFORCE_FILESYSTEM=1
		"--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/res@/res"
		"--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/rom@/rom"
		"--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/res/web/shell.html"
	)
	set_target_properties(GPMD85emu PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/web/shell.html")
	return()
endif()

install(TARGETS GPMD85emu
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	BUNDLE DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES
	rom/basic1.rmm
	rom/basic1tx.rmm
	rom/basic2.rmm
	rom/basic2A.rmm
	rom/basic3.rmm
	rom/booter2-pmd85-pmd32.rmm
	rom/c2717.rom
	rom/c2717-2.rom
	rom/demo0.rmm
	rom/demo0-c.rmm
	rom/didalfa.rom
	rom/didalfa2.rom
	rom/kli2.rmm
	rom/mato-gm.rom
	rom/mato-gm2-en.rom
	rom/mato-gm2-sk.rom
	rom/mato-gm3.rom
	rom/mato-mb.rom
	rom/mato-mb-ru.rom
	rom/monit1.rom
	rom/monit1cs.rom
	rom/monit1u.rom
	rom/monit2.rom
	rom/monit2A.rom
	rom/monit2E.rom
	rom/monit3.rom
	rom/mrs2.rmm
	rom/pascal1.rmm
	rom/pascal22.rmm
	rom/pmd85-pmd32.rmm
	rom/sach1.rmm
	rom/wurmi.rmm
	res/default.conf
	res/icon128.bmp
	res/icon32.bmp
	res/icon.png
	DESTINATION "${CMAKE_INSTALL_DATADIR}/${PACKAGE_TARNAME}"
)

install(FILES res/GPMD85emu.desktop DESTINATION "share/applications")
install(FILES res/icon.png DESTINATION "share/pixmaps" RENAME GPMD85emu.png)
