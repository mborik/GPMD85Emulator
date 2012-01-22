# GPMD85emu autoconf
# mborik, 10/2011

AC_PREREQ([2.59])
AC_INIT([GPMD85emulator], [0.9.1112], [mborik@users.sourceforge.net], [gpmd85emu], [http://gpmd85emulator.sourceforge.net/])
AM_INIT_AUTOMAKE([GPMD85emulator], [0.9.1112])
AC_CONFIG_SRCDIR([GPMD85emu.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs:
AC_PROG_RANLIB
AC_PROG_CXX
AC_PROG_CC

# Check if we have enable debugging:
AC_MSG_CHECKING([debugging enabled])
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
	DEBUG=1;
else
	CXXFLAGS="-O3 -Wall"
	CFLAGS="-O3 -Wall"
	LDFLAGS="-s"
	DEBUG=;
fi
AC_MSG_RESULT([${debug}])

# Check if we have enable only tracing statements (not full debug mode):
AC_MSG_CHECKING([tracing statements enabled])
AC_ARG_ENABLE([trace],
	AS_HELP_STRING([--enable-trace], [enable tracing statements, default: no]),
	[case "${enableval}" in
		yes) trace=yes ;;
		no)  trace=no  ;;
		*)   AC_MSG_ERROR([bad value ${enableval} for --enable-trace]) ;;
	esac],
	[trace=no]
)
if test "x$trace" = "xyes"; then
	CXXFLAGS+=" -DDEBUG"
	CFLAGS+=" -DDEBUG"
fi
AC_MSG_RESULT([${trace}])

AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(DEBUG)

old_CFLAGS="${CFLAGS}"
old_CPPFLAGS="${CPPFLAGS}"

# Checks for libraries (with classic lib/header solution if pkg-config fail):
PKG_CHECK_MODULES([libSDL], [sdl],, [
	AC_CHECK_LIB([SDL], [SDL_Quit], [
		CFLAGS="${CFLAGS} -I${includedir}/SDL -I${oldincludedir}/SDL"
		CPPFLAGS="${CPPFLAGS} -I${includedir}/SDL -I${oldincludedir}/SDL"
		AC_CHECK_HEADER([SDL.h], [
			libSDL_LIBS="-lSDL -lpthread"
			libSDL_CFLAGS="-I${includedir}/SDL -D_GNU_SOURCE=1 -D_REENTRANT"
		], AC_MSG_ERROR([SDL header not found]))
	], AC_MSG_ERROR([SDL library not found]))
])

CFLAGS="${old_CFLAGS}"
CPPFLAGS="${old_CPPFLAGS}"

# Checks for header files:
AC_CHECK_HEADERS([inttypes.h memory.h stddef.h stdint.h stdlib.h string.h strings.h unistd.h])

AC_HEADER_STAT
AC_HEADER_DIRENT
AC_HEADER_STDBOOL
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
	src/Makefile \
	rom/Makefile \
	res/Makefile \
])
AC_OUTPUT
