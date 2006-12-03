dnl Process this file with autoconf to produce a configure script.

AC_INIT(include/gavl/gavl.h)

AM_CONFIG_HEADER(include/gavl/config.h)

AM_INIT_AUTOMAKE(gavl, 0.2.6)

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

AC_CHECK_HEADERS(sys/times.h)

dnl
dnl Check for libpng (only needed for some tests)
dnl

GMERLIN_CHECK_LIBPNG

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

CFLAGS="-Wall -Wmissing-declarations $OPT_CFLAGS"

dnl
dnl Stuff from libsamplerate
dnl

AC_ARG_ENABLE(cpu-clip,
    AC_HELP_STRING([--disable-cpu-clip], [disable tricky cpu specific clipper]),
    ac_arg_cpu_clip="N", ac_arg_cpu_clip="Y")

if test x$ac_arg_cpu_clip = "xY" ; then
        AC_C_CLIP_MODE
else
        echo "checking processor clipping capabilities... disabled"
        ac_cv_c_clip_positive=0
        ac_cv_c_clip_negative=0
        fi

AC_DEFINE_UNQUOTED(CPU_CLIPS_POSITIVE, ${ac_cv_c_clip_positive},
        [Target processor clips on positive float to int conversion.])
AC_DEFINE_UNQUOTED(CPU_CLIPS_NEGATIVE, ${ac_cv_c_clip_negative},
        [Target processor clips on negative float to int conversion.])

AC_CHECK_SIZEOF(int,0)
AC_CHECK_SIZEOF(long,0)
AC_CHECK_SIZEOF(float,4)
AC_CHECK_SIZEOF(double,8)

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
gavl/libsamplerate/Makefile \
gavl/c/Makefile \
gavl/hq/Makefile \
gavl/mmx/Makefile \
gavl/mmxext/Makefile)

