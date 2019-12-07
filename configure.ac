# GPMD85emu autoconf
# mborik, 10/2011-04/2014, 09/2017, 11-12/2019

AC_PREREQ([2.59])
AC_INIT([GPMD85emulator], [0.11.1912], [mborik@users.sourceforge.net],
        [gpmd85emu], [https://github.com/mborik/GPMD85Emulator])
AC_CONFIG_SRCDIR([GPMD85emu.cpp])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
AC_DEFINE_UNQUOTED([PACKAGE_YEAR], "`date +%Y`",
                   [Define to the build year of this package.])

# Checks for programs:
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
	CXXFLAGS="-g3 -O0 -Wall -DDEBUG"
	CFLAGS="-g3 -O0 -Wall -DDEBUG"
	LDFLAGS=""
	DEBUG=1
else
	CXXFLAGS="-Os -Wall"
	CFLAGS="-Os -Wall"
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

# Check if we have software renderer:
AC_MSG_CHECKING([gpmd85emu: Software renderer])
AC_ARG_WITH([soft-render],
	AS_HELP_STRING([--with-soft-render], [init software renderer, default: no]),
	[case "${withval}" in
		yes) softrender=yes ;;
		no)  softrender=no  ;;
		*)   AC_MSG_ERROR([bad value ${withval} for --with-soft-render]) ;;
	esac],
	[softrender=no]
)
if test "x$softrender" = "xyes"; then
	CXXFLAGS+=" -DSOFTRENDER"
	CFLAGS+=" -DSOFTRENDER"
fi
AC_MSG_RESULT([${softrender}])

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

# NOTICE: In case of compile error with missing malloc() / realloc() functions,
#         you can try to set enviroment variables 'ac_cv_func_malloc_0_nonnull'
#         and 'ac_cv_func_realloc_0_nonnull' to 'yes' and reconfigure.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_ERROR_AT_LINE

# Checks for standard library functions:
AC_CHECK_FUNCS([memset mkdir strstr strtol strcasecmp strrchr])

AC_CONFIG_FILES([\
	Makefile \
	saa/Makefile \
	src/Makefile \
	gui/Makefile \
	rom/Makefile \
	res/Makefile \
])
AC_OUTPUT
