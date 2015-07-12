AC_PREREQ([2.69])

AC_INIT([libsombrero],[m4_esyscmd(./git-version-gen .tarball-version)],[lgirdwood (at) gmail (dot) com])

AC_CONFIG_MACRO_DIR([m4])

# libtool versioning
SOMBRERO_CURRENT_VERSION=0
SOMBRERO_RELEASE_VERSION=0
SOMBRERO_AGE_VERSION=0
LT_VERSION=$SOMBRERO_CURRENT_VERSION:$SOMBRERO_RELEASE_VERSION:$SOMBRERO_AGE_VERSION
AC_SUBST(LT_VERSION)

SOMBRERO_VERSION=0.10
AC_SUBST(SOMBRERO_VERSION)

VERSION=$SOMBRERO_VERSION
PACKAGE=libsombrero

AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax])

# Specify a configuration file
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
AC_CHECK_FUNCS([bzero memset qsort posix_memalign], , [AC_MSG_ERROR([Need bzero memset qsort posix_memalign])])
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

# SSE4_2 support
AC_ARG_ENABLE(sse42, [AS_HELP_STRING([--enable-sse42],[enable SSE42 optimizations])], have_sse42=$enableval, have_sse42=yes)
if test "$have_sse42" = "yes"; then
        AC_DEFINE(HAVE_SSE42,1,[Define to enable SSE42 optimizations.])
fi
AM_CONDITIONAL(HAVE_SSE42, test "$have_sse42" = "yes")

if test "$have_sse42" = "yes" -a "x$SSE42_CFLAGS" = x; then
	AX_CHECK_COMPILE_FLAG(-msse4.2, [SSE42_CFLAGS="-DOPS_SSE42 -msse4.2 -ffast-math -ftree-vectorizer-verbose=0"],
		[AC_MSG_ERROR([Need a version of gcc with -msse42])])
fi

AC_SUBST(SSE42_CFLAGS)

# AVX support
AC_ARG_ENABLE(avx, [AS_HELP_STRING([--enable-avx],[enable AVX optimizations])], have_avx=$enableval, have_avx=yes)
if test "$have_avx" = "yes"; then
        AC_DEFINE(HAVE_AVX,1,[Define to enable AVX optimizations.])
fi
AM_CONDITIONAL(HAVE_AVX, test "$have_avx" = "yes")

if test "$have_avx" = "yes" -a "x$AVX_CFLAGS" = x; then
	AX_CHECK_COMPILE_FLAG(-mavx, [AVX_CFLAGS="-DOPS_AVX -mavx -ffast-math -ftree-vectorizer-verbose=0"],
		[AC_MSG_ERROR([Need a version of gcc with -mavx])])
fi

AC_SUBST(AVX_CFLAGS)

# AVX2 support
AC_ARG_ENABLE(avx2, [AS_HELP_STRING([--enable-avx2],[enable AVX2 optimizations])], have_avx2=$enableval, have_avx2=yes)
if test "$have_avx2" = "yes"; then
        AC_DEFINE(HAVE_AVX2,1,[Define to enable AVX2 optimizations.])
fi
AM_CONDITIONAL(HAVE_AVX2, test "$have_avx2" = "yes")

if test "$have_avx2" = "yes" -a "x$AVX2_CFLAGS" = x; then
	AX_CHECK_COMPILE_FLAG(-mavx2, [AVX2_CFLAGS="-DOPS_AVX2 -mavx2 -ffast-math -ftree-vectorizer-verbose=0"],
		[AC_MSG_ERROR([Need a version of gcc with -mavx2])])
fi

AC_SUBST(AVX2_CFLAGS)

# FMA support
AC_ARG_ENABLE(fma, [AS_HELP_STRING([--enable-fma],[enable FMA optimizations])], have_fma=$enableval, have_fma=yes)
if test "$have_fma" = "yes"; then
        AC_DEFINE(HAVE_FMA,1,[Define to enable FMA optimizations.])
fi
AM_CONDITIONAL(HAVE_FMA, test "$have_fma" = "yes")

if test "$have_fma" = "yes" -a "x$FMA_CFLAGS" = x; then
	AX_CHECK_COMPILE_FLAG(-mfma, [FMA_CFLAGS="-DOPS_FMA -mfma -ffast-math -ftree-vectorizer-verbose=0"],
		[AC_MSG_ERROR([Need a version of gcc with -mfma])])
fi

AC_SUBST(FMA_CFLAGS)

# OpenMP support
AC_ARG_ENABLE(openmp, [AS_HELP_STRING([--enable-openmp],[enable OpenMP])], have_openmp=$enableval, have_openmp=yes)
if test "$have_openmp" = "yes"; then
        AC_DEFINE(HAVE_OPENMP,1,[Define to enable OpenMP])
fi
AM_CONDITIONAL(HAVE_OPENMP, test "$have_openmp" = "yes")

if test "$have_openmp" = "yes" -a "x$OPENMP_CFLAGS" = x; then
	AX_CHECK_COMPILE_FLAG(-fopenmp, [OPENMP_CFLAGS="-fopenmp"],
		[AC_MSG_ERROR([Need a version of gcc with -fopenmp])])
fi

AC_SUBST(OPENMP_CFLAGS)

AC_CONFIG_FILES([
Makefile
src/Makefile
include/Makefile
doc/Makefile
examples/Makefile
m4/Makefile
libsombrero.pc
])

AC_OUTPUT

AS_IF([test "$have_sse42" = "yes"], ENABLE_SSE42=yes, ENABLE_SSE42=no)
AS_IF([test "$have_avx" = "yes"], ENABLE_AVX=yes, ENABLE_AVX=no)
AS_IF([test "$have_avx2" = "yes"], ENABLE_AVX2=yes, ENABLE_AVX2=no)
AS_IF([test "$have_fma" = "yes"], ENABLE_FMA=yes, ENABLE_FMA=no)
AS_IF([test "$have_openmp" = "yes"], ENABLE_OPENMP=yes, ENABLE_OPENMP=no)


echo "
---{ $PACKAGE_NAME $VERSION }---

prefix:                        ${prefix}
Compiler:                      ${CC}
CFLAGS:                        ${CFLAGS}
LIBS:                          ${LIBS}

Enable SSE42:                  ${ENABLE_SSE42}
Enable AVX:                    ${ENABLE_AVX}
Enable AVX2:                   ${ENABLE_AVX2}
Enable FMA:                    ${ENABLE_FMA}
Enable OpenMP:    	       ${ENABLE_OPENMP}
"

