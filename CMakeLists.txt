cmake_minimum_required(VERSION 3.20)

project(GPMD85emulator
    VERSION 1.0.2609
    DESCRIPTION "Open-source, multi-platform emulator of the Tesla PMD 85"
    HOMEPAGE_URL "https://github.com/mborik/GPMD85Emulator"
    LANGUAGES C CXX
)

include(GNUInstallDirs)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckCXXSourceCompiles)
include(TestBigEndian)

option(GPMD85EMU_ENABLE_DEBUG "Enable debug build flags" OFF)
option(GPMD85EMU_ENABLE_TRACE "Compile trace messages" ON)

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

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(WARNING "This project is primarily tested with Clang and GCC compilers.")
endif()

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "macOS deployment target" FORCE)
endif()

set(GPMD85EMU_COMMON_COMPILE_OPTIONS -Wno-deprecated)

if(GPMD85EMU_ENABLE_DEBUG)
    list(APPEND GPMD85EMU_COMMON_COMPILE_OPTIONS -g3 -O0)
    add_compile_definitions(DEBUG)
else()
    list(APPEND GPMD85EMU_COMMON_COMPILE_OPTIONS -Os)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_link_options(-s)
    endif()
endif()

if(NOT GPMD85EMU_ENABLE_TRACE AND NOT GPMD85EMU_ENABLE_DEBUG)
    add_compile_definitions(NOTRACEMSG)
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET IMPORTED_TARGET sdl2)
endif()

if(TARGET PkgConfig::SDL2)
    set(GPMD85EMU_SDL_TARGET PkgConfig::SDL2)
else()
    find_package(SDL2 REQUIRED)
    if(TARGET SDL2::SDL2)
        set(GPMD85EMU_SDL_TARGET SDL2::SDL2)
    else()
        add_library(GPMD85emu_sdl2 INTERFACE)
        target_include_directories(GPMD85emu_sdl2 INTERFACE ${SDL2_INCLUDE_DIRS})
        target_compile_options(GPMD85emu_sdl2 INTERFACE ${SDL2_CFLAGS_OTHER})
        target_link_libraries(GPMD85emu_sdl2 INTERFACE ${SDL2_LIBRARIES})
        set(GPMD85EMU_SDL_TARGET GPMD85emu_sdl2)
    endif()
endif()

set(GPMD85EMU_OPENGL_LIBS "")
if(APPLE)
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

set(CMAKE_REQUIRED_INCLUDES)
check_include_file("stdio.h" HAVE_STDIO_H)
check_include_file("stdarg.h" HAVE_STDARG_H)
check_include_file("stddef.h" HAVE_STDDEF_H)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("stdlib.h" HAVE_STDLIB_H)
check_include_file("unistd.h" HAVE_UNISTD_H)
check_include_file("inttypes.h" HAVE_INTTYPES_H)
check_include_file("memory.h" HAVE_MEMORY_H)
check_include_file("string.h" HAVE_STRING_H)
check_include_file("strings.h" HAVE_STRINGS_H)
check_include_file("sys/syslimits.h" HAVE_SYS_SYSLIMITS_H)
check_include_file("sys/types.h" HAVE_SYS_TYPES_H)
check_include_file("sys/stat.h" HAVE_SYS_STAT_H)
check_include_file("GL/gl.h" HAVE_GL_GL_H)
check_include_file("dirent.h" HAVE_DIRENT_H)
check_include_file("sys/ndir.h" HAVE_SYS_NDIR_H)
check_include_file("sys/dir.h" HAVE_SYS_DIR_H)
check_include_file("ndir.h" HAVE_NDIR_H)

if(HAVE_DIRENT_H)
    check_struct_has_member("struct dirent" d_type "dirent.h" HAVE_STRUCT_DIRENT_D_TYPE)
endif()

check_include_file("stdbool.h" HAVE_STDBOOL_H)
if(HAVE_STDBOOL_H)
    set(HAVE__BOOL 1)
endif()

if(HAVE_STDIO_H AND HAVE_STDARG_H AND HAVE_STDDEF_H AND HAVE_STDLIB_H AND HAVE_STRING_H)
    set(STDC_HEADERS 1)
endif()

check_function_exists(memset HAVE_MEMSET)
check_function_exists(mkdir HAVE_MKDIR)
check_function_exists(rmdir HAVE_RMDIR)
check_function_exists(stat HAVE_STAT)
check_function_exists(strstr HAVE_STRSTR)
check_function_exists(strtol HAVE_STRTOL)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strrchr HAVE_STRRCHR)
check_function_exists(error_at_line HAVE_ERROR_AT_LINE)

if(NOT HAVE_ERROR_AT_LINE)
    message(WARNING "error_at_line not found; continuing without it")
endif()

set(CMAKE_REQUIRED_FLAGS "-std=c++17")
unset(CMAKE_REQUIRED_LIBRARIES)
check_cxx_source_compiles(
    "#include <filesystem>\nint main(){std::filesystem::path p(\"/tmp\");return 0;}"
    HAVE_STD_FILESYSTEM
)

set(GPMD85EMU_FILESYSTEM_LIB "")
if(NOT HAVE_STD_FILESYSTEM AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_REQUIRED_LIBRARIES stdc++fs)
    check_cxx_source_compiles(
        "#include <filesystem>\nint main(){std::filesystem::path p(\"/tmp\");return 0;}"
        HAVE_STD_FILESYSTEM_WITH_STDCXXFS
    )
    unset(CMAKE_REQUIRED_LIBRARIES)

    if(HAVE_STD_FILESYSTEM_WITH_STDCXXFS)
        set(HAVE_STD_FILESYSTEM 1)
        set(GPMD85EMU_FILESYSTEM_LIB stdc++fs)
    endif()
endif()

if(NOT HAVE_STD_FILESYSTEM)
    check_cxx_source_compiles(
        "#include <experimental/filesystem>\nint main(){std::experimental::filesystem::path p(\"/tmp\");return 0;}"
        HAVE_STD_EXPERIMENTAL_FILESYSTEM
    )
endif()
unset(CMAKE_REQUIRED_FLAGS)

if(NOT HAVE_STD_FILESYSTEM AND NOT HAVE_STD_EXPERIMENTAL_FILESYSTEM)
    message(FATAL_ERROR "C++17 std::filesystem is required but not found.")
endif()

check_include_file("dlfcn.h" HAVE_DLFCN_H)
check_function_exists(dlopen HAVE_DLOPEN)
set(GPMD85EMU_DL_LIB "")
if(NOT HAVE_DLOPEN)
    find_library(DL_LIBRARY dl)
    if(NOT DL_LIBRARY)
        message(FATAL_ERROR "Dynamic linking support (dlopen) is required but not found.")
    endif()
    set(GPMD85EMU_DL_LIB ${DL_LIBRARY})
endif()

test_big_endian(WORDS_BIGENDIAN)

set(DIR_RESOURCES "${CMAKE_INSTALL_FULL_DATADIR}/${PACKAGE_TARNAME}")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gui/imgui/imgui.cpp")
    message(FATAL_ERROR "Missing gui/imgui submodule. Initialize submodules before configuring.")
endif()

set(CONFIG_H_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${CONFIG_H_OUTPUT_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in"
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
target_compile_definitions(gpmd85emu_gui PUBLIC IMGUI_USER_CONFIG='"imgui-mods/imconfig_mod.h"')
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

install(TARGETS GPMD85emu RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

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

install(FILES res/GPMD85emu.desktop DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES res/icon.png DESTINATION "${CMAKE_INSTALL_DATADIR}/pixmaps" RENAME GPMD85emu.png)
