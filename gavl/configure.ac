dnl Process this file with autoconf to produce a configure script.

AC_INIT(include/gavl/gavl.h)

AM_CONFIG_HEADER(include/gavl/config.h)

AM_INIT_AUTOMAKE(gavl, 0.2.4pre1)

AC_CANONICAL_HOST

dnl AC_DISABLE_STATIC

AC_PROG_LIBTOOL

AC_PROG_MAKE_SET

dnl
dnl Check for compiler flags
dnl  

GAVL_ENDIANESS="GAVL_PROCESSOR_LITTLE_ENDIAN"

AC_C_ATTRIBUTE_ALIGNED

AC_C_BIGENDIAN(GAVL_ENDIANESS="GAVL_PROCESSOR_BIG_ENDIAN",,AC_MSG_ERROR("Cannot detect endianess"))
AC_SUBST(GAVL_ENDIANESS)

dnl
dnl Check for Dependencies package
dnl

GMERLIN_DEP_DIR=/opt/gmerlin

if test -d $GMERLIN_DEP_DIR; then
  have_opt_gmerlin="true"
  export PKG_CONFIG_PATH=$GMERLIN_DEP_DIR/lib/pkgconfig:$PKG_CONFIG_PATH
  GMERLIN_DEP_CFLAGS="-I$GMERLIN_DEP_DIR/include"
  GMERLIN_DEP_LDPATH="$GMERLIN_DEP_DIR/lib"
  GMERLIN_DEP_LIBS="-L$GMERLIN_DEP_DIR/lib"
  GMERLIN_DEP_RPATH="-Wl,--rpath -Wl,$GMERLIN_DEP_DIR/lib"
else
  have_opt_gmerlin="false"
  GMERLIN_DEP_CFLAGS=""
  GMERLIN_DEP_LDPATH=""
  GMERLIN_DEP_LIBS=""
  GMERLIN_DEP_RPATH=""
fi

LDFLAGS="$GMERLIN_DEP_RPATH"
AC_SUBST(GMERLIN_DEP_LIBS)

dnl
dnl Check for library functions
dnl 

dnl Checks for library functions.
AC_CHECK_FUNCS([memalign])
AC_C99_FUNC_LRINT
AC_C99_FUNC_LRINTF

SAMPLERATE_REQUIRED="0.1.0"

PKG_CHECK_MODULES(SECRETRABBIT, samplerate >= $SAMPLERATE_REQUIRED, ,
                  AC_MSG_ERROR("libsamplerate not found! Go to http://www.mega-nerd.com/SRC/"))

AC_SUBST(SAMPLERATE_REQUIRED)

dnl
dnl Build optimization flags
dnl

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])

dnl
dnl Check for SIMD
dnl

AH_TEMPLATE([ARCH_X86],    [Intel Architecture (32/64)])
AH_TEMPLATE([ARCH_X86_64], [Intel Architecture (64)])
AH_TEMPLATE([ARCH_PPC],    [PowerPC Architecture])

AH_TEMPLATE([HAVE_MMX],    [MMX Supported])
AH_TEMPLATE([HAVE_3DNOW],  [3Dnow Supported])
AH_TEMPLATE([HAVE_SSE],    [SSE Supported])
AH_TEMPLATE([HAVE_SSE2],   [SSE2 Supported])


GAVL_CHECK_SIMD($host_cpu, "$OPT_CFLAGS")

if test x"$HAVE_MMX" = "xtrue"; then
AC_DEFINE(HAVE_MMX)
fi
AM_CONDITIONAL(HAVE_MMX, test "x$HAVE_MMX" = "xtrue")

if test x"$HAVE_3DNOW" = "xtrue"; then
AC_DEFINE(HAVE_3DNOW)
fi
AM_CONDITIONAL(HAVE_3DNOW, test "x$HAVE_3DNOW" = "xtrue")

if test x"$HAVE_SSE" = "xtrue"; then
AC_DEFINE(HAVE_SSE)
fi
AM_CONDITIONAL(HAVE_SSE, test "x$HAVE_SSE" = "xtrue")

if test x"$HAVE_SSE2" = "xtrue"; then
AC_DEFINE(HAVE_SSE2)
fi
AM_CONDITIONAL(HAVE_SSE2, test "x$HAVE_SSE2" = "xtrue")

if test x"$ARCH_X86" = "xtrue"; then
AC_DEFINE(ARCH_X86)
fi

if test x"$ARCH_X86_64" = "xtrue"; then
AC_DEFINE(ARCH_X86_64)
fi

if test x"$ARCH_PPC" = "xtrue"; then
AC_DEFINE(ARCH_PPC)
fi

dnl
dnl Now, build the final CFLAGS.
dnl 

CFLAGS="-Wall $SECRETRABBIT_CFLAGS $OPT_CFLAGS"

dnl
dnl Output variables
dnl

INCLUDES='-I$(top_builddir)/include'
AC_SUBST(INCLUDES)

AC_OUTPUT(Makefile \
gavl.spec \
gavl.pc \
src/Makefile \
m4/Makefile \
include/Makefile \
include/gavlconfig.h \
include/gavl/Makefile \
gavl/Makefile \
gavl/libgdither/Makefile \
gavl/c/Makefile \
gavl/mmx/Makefile \
gavl/mmxext/Makefile)

