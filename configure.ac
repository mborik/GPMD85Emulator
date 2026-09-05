# GPMD85emu autoconf by mborik

AC_PREREQ([2.59])
AC_INIT([GPMD85emulator], [1.0.2609], [martin@borik.net],
		[gpmd85emu], [https://github.com/mborik/GPMD85Emulator])
AC_CONFIG_SRCDIR([main.cpp])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
AC_DEFINE_UNQUOTED([PACKAGE_YEAR], "`date +%Y`",
				   [Define to the build year of this package.])

# Checks for programs:
AC_CANONICAL_HOST
AC_PROG_RANLIB
AC_PROG_CXX
AC_PROG_CC
AC_REQUIRE_CPP

# Add an additional flag if using CLANG
AC_MSG_CHECKING([if compiling with clang])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[
	#ifndef __clang__
		not clang
	#endif
]])], [clang=yes], [clang=no])
AC_MSG_RESULT([$clang])

# Check if we have enabled debug mode:
AC_MSG_CHECKING([gpmd85emu: Debug mode])
AC_ARG_ENABLE([debug],
	AS_HELP_STRING([--enable-debug], [enable debugging, default: no]),
	[case "${enableval}" in
		yes) debug=yes ;;
		no)  debug=no  ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	esac],
	[debug=no]
)
if test "x$debug" = "xyes"; then
	CXXFLAGS="-g3 -O0 -Wno-deprecated -DDEBUG"
	CFLAGS="-g3 -O0 -Wno-deprecated -DDEBUG"
	LDFLAGS=""
	DEBUG=1
else
	CXXFLAGS="-Os -Wno-deprecated"
	CFLAGS="-Os -Wno-deprecated"
	if test "x$clang" = "xno"; then
		LDFLAGS="-s"
	fi
	DEBUG=
fi
AC_MSG_RESULT([${debug}])

# Check if we want compile all trace statements (not available in debug mode):
AC_MSG_CHECKING([gpmd85emu: Check if compile trace statements])
AC_ARG_ENABLE([trace],
	AS_HELP_STRING([--disable-trace], [not compile trace statements, default: no]),
	[case "${enableval}" in
		no) trace=no  ;;
		*)  trace=yes ;;
	esac],
	[trace=yes]
)
if test "x$trace" = "xno" -a "x$debug" = "xno"; then
	CXXFLAGS+=" -DNOTRACEMSG"
	CFLAGS+=" -DNOTRACEMSG"
fi
AC_MSG_RESULT([${trace}])

resDir_CFLAGS=" -DDIR_RESOURCES=\"\\\""$\{pkgdatadir}"\\\"\""
AC_SUBST(resDir_CFLAGS)

# Checks for libraries (with classic lib/header solution if pkg-config fail):
PKG_CHECK_MODULES([libSDL], [sdl2],, [
	AC_CHECK_LIB([SDL2], [SDL_Init], [
		AC_CHECK_HEADER([SDL2/SDL.h], [
			libSDL_LIBS="-lSDL2"
			libSDL_CFLAGS="-I${includedir}/SDL2 -D_THREAD_SAFE"
			AC_SUBST(libSDL_LIBS)
			AC_SUBST(libSDL_CFLAGS)
		], AC_MSG_ERROR([SDL2 header not found]))
	], AC_MSG_ERROR([SDL2 library not found]))
])

libGL_LIBS=""
libGL_CFLAGS=""

case "${host_os}" in
	darwin*)
		# Dynamic detection of default macOS deployment target from compiler
		AC_MSG_CHECKING([default macOS deployment target from compiler])
		AC_LANG_PUSH([C++])

		# Compile and link a minimal conftest binary
		AC_LINK_IFELSE([AC_LANG_PROGRAM([], [])], [
			# Extract minos/sdk value from the generated binary using otool and awk
			DETECTED_TARGET=$(otool -l conftest$EXEEXT | grep -A 2 "minos" | awk '/minos/ {print $2}')

			# Fallback for older otool tools (older Xcode versions used LC_VERSION_MIN_MACOSX)
			if test -z "$DETECTED_TARGET"; then
				DETECTED_TARGET=$(otool -l conftest$EXEEXT | grep -A 3 "LC_VERSION_MIN_MACOSX" | awk '/version/ {print $2}')
			fi
		], [
			DETECTED_TARGET=""
		])
		AC_LANG_POP([C++])

		# macOS: fallback to safe 10.15 if otool fails
		if test -n "$DETECTED_TARGET"; then
			AC_MSG_RESULT([$DETECTED_TARGET])
			DEFAULT_MACOS_TARGET="$DETECTED_TARGET"
		else
			AC_MSG_RESULT([unknown, using fallback 10.15])
			DEFAULT_MACOS_TARGET="10.15"
		fi

		# Set MACOSX_DEPLOYMENT_TARGET for the linker
		if test -z "$MACOSX_DEPLOYMENT_TARGET"; then
			export MACOSX_DEPLOYMENT_TARGET="$DEFAULT_MACOS_TARGET"
		fi

		# ... and finally write to CXXFLAGS for the compiler
		if [[ "$CXXFLAGS" != *"-mmacosx-version-min"* ]]; then
			CXXFLAGS="$CXXFLAGS -mmacosx-version-min=$MACOSX_DEPLOYMENT_TARGET"
		fi

		# macOS: Use native Apple Framework for OpenGL
		AC_MSG_CHECKING([for OpenGL framework])
		libGL_LIBS="-Wl,-framework,OpenGL"
		libGL_CFLAGS=""
		AC_MSG_RESULT([yes])
		;;
	*)
		AC_CHECK_LIB([GL], [glClear], [
			libGL_LIBS="-lGL"
			AC_CHECK_HEADERS([GL/gl.h], [
				libGL_CFLAGS="-I${includedir}/GL"
			], [AC_MSG_ERROR([OpenGL headers not found])])
		], [
			AC_MSG_ERROR([OpenGL library not found.])
		])
		;;
esac
AC_SUBST(libGL_LIBS)
AC_SUBST(libGL_CFLAGS)

# Checks for dynamic linking library (safe fallback for linux)
AC_SEARCH_LIBS([dlopen], [dl], [], [
	AC_MSG_ERROR([Dynamic linking support (dlopen) is required but not found.])
])

AC_LANG_PUSH([C++])

# Force C++17 standard for Clang and GCC
AC_MSG_CHECKING([whether $CXX supports C++17 standard])
CXXFLAGS="$CXXFLAGS -std=c++17"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
	#if __cplusplus < 201703L
	#error "Not C++17"
	#endif
]], [[]])], [
	AC_MSG_RESULT([yes])
], [
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([C++17 compiler is required to build this project.])
])

# Check for std::filesystem support (C++17)
AC_CACHE_CHECK([for std::filesystem], [ax_cv_std_filesystem], [
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[
		#include <filesystem>
	]], [[
		std::filesystem::path p("/tmp");
	]])], [ax_cv_std_filesystem=yes], [ax_cv_std_filesystem=no])
])

# If std::filesystem is not available, check for std::experimental::filesystem
if test "x$ax_cv_std_filesystem" = "xno" && test "x$clang" = "xno"; then
	LIBS_save="$LIBS"
	LIBS="$LIBS -lstdc++fs"
	AC_CACHE_CHECK([for std::filesystem with -lstdc++fs], [ax_cv_std_filesystem_lib], [
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
			#include <filesystem>
		]], [[
			std::filesystem::path p("/tmp");
		]])], [ax_cv_std_filesystem_lib=yes], [ax_cv_std_filesystem_lib=no])
	])
	if test "x$ax_cv_std_filesystem_lib" = "xyes"; then
		ax_cv_std_filesystem=yes
	else
		LIBS="$LIBS_save"
	fi
fi

# Define checks into config.h
if test "x$ax_cv_std_filesystem" = "xyes"; then
	AC_DEFINE([HAVE_STD_FILESYSTEM], [1], [Define to 1 if you have std::filesystem])
else
	AC_CACHE_CHECK([for std::experimental::filesystem], [ax_cv_std_experimental_filesystem], [
		AC_LINK_IFELSE([AC_LANG_PROGRAM([[
			#include <experimental/filesystem>
		]], [[
			std::experimental::filesystem::path p("/tmp");
		]])], [ax_cv_std_experimental_filesystem=yes], [ax_cv_std_experimental_filesystem=no])
	])

	if test "x$ax_cv_std_experimental_filesystem" = "xyes"; then
		AC_DEFINE([HAVE_STD_EXPERIMENTAL_FILESYSTEM], [1], [Define to 1 if you have std::experimental::filesystem])
	else
		AC_MSG_ERROR([C++17 std::filesystem is required but not found. On macOS, ensure your deployment target is set correctly.])
	fi
fi

AC_LANG_POP([C++])

AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(DEBUG)

# Checks for header files:
AC_CHECK_HEADERS([\
	stdio.h stdarg.h stddef.h stdint.h stdlib.h unistd.h \
	inttypes.h memory.h string.h strings.h \
	sys/syslimits.h \
])

AC_HEADER_STAT
AC_HEADER_DIRENT
AC_STRUCT_DIRENT_D_TYPE
AC_HEADER_STDBOOL
AC_C_BIGENDIAN
AC_C_INLINE
AC_FUNC_ERROR_AT_LINE

# Checks for standard library functions:
AC_CHECK_FUNCS([memset mkdir rmdir stat strstr strtol strcasecmp strrchr])

AC_CONFIG_FILES([\
	Makefile \
	saa/Makefile \
	src/Makefile \
	gui/Makefile \
	rom/Makefile \
	res/Makefile \
])
AC_OUTPUT
