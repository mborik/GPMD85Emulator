# GPMD85emu autoconf
# mborik, 10/2011-04/2014, 09/2017

AC_PREREQ([2.59])
AC_INIT([GPMD85emulator], [0.9.1709], [mborik@users.sourceforge.net],
        [gpmd85emu], [http://gpmd85emulator.sourceforge.net/])
AC_CONFIG_SRCDIR([GPMD85emu.cpp])

AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
AC_DEFINE_UNQUOTED([PACKAGE_YEAR], "`date +%Y`",
                   [Define to the build year of this package.])

# Checks for programs:
AC_PROG_RANLIB
AC_PROG_CXX
AC_PROG_CC

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

# Check if we have enabled trace statements (but not full debug mode):
AC_MSG_CHECKING([gpmd85emu: Trace statements enabled])
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

# Check if we have to display screen with OpenGL:
AC_MSG_CHECKING([gpmd85emu: OpenGL])
AC_ARG_WITH([opengl],
	AS_HELP_STRING([--with-opengl], [OpenGL support, default: no]),
	[case "${withval}" in
		yes) opengl=yes ;;
		no)  opengl=no  ;;
		*)   AC_MSG_ERROR([bad value ${withval} for --with-opengl]) ;;
	esac],
	[opengl=no]
)
AC_MSG_RESULT([${opengl}])

resDir_CFLAGS=" -DDIR_RESOURCES=\"\\\""$\{pkgdatadir}"\\\"\""
AC_SUBST(resDir_CFLAGS)

# Checks for libraries (with classic lib/header solution if pkg-config fail):
PKG_CHECK_MODULES([libSDL], [sdl],, [
	AC_CHECK_LIB([SDL], [SDL_Quit], [
		AC_CHECK_HEADER([SDL.h], [
			libSDL_LIBS="-lSDL -lpthread"
			libSDL_CFLAGS="-I${includedir}/SDL -D_GNU_SOURCE=1 -D_REENTRANT"
			AC_SUBST(libSDL_LIBS)
			AC_SUBST(libSDL_CFLAGS)
		], AC_MSG_ERROR([SDL header not found]))
	], AC_MSG_ERROR([SDL library not found]))
])
if test "x$opengl" = "xyes"; then
	PKG_CHECK_MODULES([libGL], [gl], [
		CXXFLAGS+=" -DOPENGL"
		CFLAGS+=" -DOPENGL"
	], [
		AC_CHECK_HEADER([SDL/SDL_opengl.h],
			AC_CHECK_LIB([GL], [main], [
				libGL_LIBS="-lGL"
				AC_SUBST(libGL_LIBS)
				CXXFLAGS+=" -DOPENGL"
				CFLAGS+=" -DOPENGL"
			], AC_CHECK_LIB([opengl32], [main], [
				libGL_LIBS="-lopengl32"
				AC_SUBST(libGL_LIBS)
				CXXFLAGS+=" -DOPENGL"
				CFLAGS+=" -DOPENGL"
			], AC_MSG_WARN([OpenGL library not found]))
		),
		AC_MSG_WARN([OpenGL header not found]))
	])
fi

AC_SUBST(CXXFLAGS)
AC_SUBST(CFLAGS)
AC_SUBST(DEBUG)

# Checks for header files:
AC_CHECK_HEADERS([\
	inttypes.h memory.h string.h strings.h \
	stddef.h stdint.h stdlib.h unistd.h \
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
	src/Makefile \
	gui/Makefile \
	rom/Makefile \
	res/Makefile \
])
AC_OUTPUT
