AC_PREREQ([2.69])

AC_INIT([libsombrero],[m4_esyscmd(./git-version-gen .tarball-version)],[lgirdwood (at) gmail (dot) com])

AC_CONFIG_MACRO_DIR([m4])

# libtool versioning
SOMBRERO_MAJOR_VERSION=0
SOMBRERO_MINOR_VERSION=90
SOMBRERO_MICRO_VERSION=0
SOMBRERO_VERSION=$SOMBRERO_MAJOR_VERSION.$SOMBRERO_MINOR_VERSION.$SOMBRERO_MICRO_VERSION
AC_SUBST(SOMBRERO_MAJOR_VERSION)
AC_SUBST(SOMBRERO_MINOR_VERSION)
AC_SUBST(SOMBRERO_MICRO_VERSION)
AC_SUBST(SOMBRERO_VERSION)

LT_VERSION=$SOMBRERO_MICRO_VERSION
LT_RELEASE=$SOMBRERO_MAJOR_VERSION.$SOMBRERO_MINOR_VERSION
AC_SUBST(LT_VERSION)
AC_SUBST(LT_RELEASE)

VERSION=$SOMBRERO_VERSION
PACKAGE=libsombrero

AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax])

# Specify a configuration file
#AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

dnl Initialize libtool
LT_PREREQ(2.2)
LT_INIT

dnl Initialize maintainer mode
AM_MAINTAINER_MODE([enable])

CFLAGS="-Wall"

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([bzero memset qsort], , [AC_MSG_ERROR([Need bzero memset qsort])])
AC_CHECK_LIB([m], [sqrtf], , [AC_MSG_ERROR([Need sqrtf])])

# Debug symbol support
AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug],[enable debug symbols])],
	have_debug=$enableval, have_debug=no)
if test "$have_debug" = "yes"; then
        AC_DEFINE(HAVE_DEBUG,1,[Define to enable debug symbols.])
        CFLAGS+=" -g"
else
	CFLAGS+=" -O3"
fi

# AVX support
AC_ARG_ENABLE(avx, [AS_HELP_STRING([--enable-avx],[enable AVX optimizations])], have_avx=$enableval, have_avx=no)
if test "$have_avx" = "yes"; then
        AC_DEFINE(HAVE_AVX,1,[Define to enable AVX optimizations.])
fi
AM_CONDITIONAL(HAVE_AVX, test "$have_avx" = "yes")

if test "$have_avx" = "yes" -a "x$AVX_CFLAGS" = x; then
	AX_CHECK_COMPILE_FLAG(-mavx, [AVX_CFLAGS="-mavx"],
		[AC_MSG_ERROR([Need a version of gcc with -mavx])])
fi

AC_SUBST(AVX_CFLAGS)

AC_CONFIG_FILES([
Makefile
src/Makefile
include/Makefile
doc/Makefile
examples/Makefile
m4/Makefile
])

AC_OUTPUT
