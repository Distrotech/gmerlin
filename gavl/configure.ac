dnl Process this file with autoconf to produce a configure script.

AC_INIT(include/gavl/gavl.h)

AM_CONFIG_HEADER(include/gavl/config.h)

AM_INIT_AUTOMAKE(gavl, 0.2.2)

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

dnl
dnl Check for library functions
dnl 

dnl Checks for library functions.
AC_CHECK_FUNCS([memalign])
AC_C99_FUNC_LRINT
AC_C99_FUNC_LRINTF

SAMPLERATE_REQUIRED="0.1.0"

PKG_CHECK_MODULES(SECRETRABBIT, samplerate >= $SAMPLERATE_REQUIRED, ,
                  AC_MSG_ERROR("libsamplerate not found! Get it from http://www.mega-nerd.com/SRC/"))

AC_SUBST(SAMPLERATE_REQUIRED)

dnl
dnl CFLAGS
dnl

CFLAGS="-Wall"

dnl
dnl Build optimization flags
dnl


OPT_CFLAGS=""
TRY_CFLAGS=""

CPU_PENTIUM=false

if test x"$GCC" = x"yes"; then
  CFLAGS="-Wall -finline-functions $CFLAGS"
  TRY_CFLAGS="-O3"
  AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])

  dnl -fomit-frame-pointer
  TRY_CFLAGS="$OPT_CFLAGS -fomit-frame-pointer -funroll-all-loops -falign-functions=2 -falign-loops=2 -falign-jumps=2"
  AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])

  dnl arch-specific flags
  case "$host_cpu" in
    i386)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=i386";;
    i486)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=i486";;
    i586)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=pentium"; CPU_PENTIUM=true ;;
    i686)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=pentiumpro"; CPU_PENTIUM=true ;;
    k6)             TRY_CFLAGS="$OPT_CFLAGS -mcpu=k6"; CPU_PENTIUM=true ;;
    sparc)          TRY_CFLAGS="$OPT_CFLAGS -mcpu=ultrasparc -mvis";;
  esac
AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])
fi

dnl
dnl Now, build the final CFLAGS.
dnl 

have_debug=false

AC_ARG_ENABLE(debug,
AC_HELP_STRING([--enable-debug],[Enable debugging (disable optimization)]),
[case "${enableval}" in
   yes) have_debug=true ;;
   no)  have_debug=false ;;
esac],have_debug=false)

case $have_debug in
  true) CFLAGS="-g $CFLAGS";;
  false) CFLAGS="$CFLAGS $OPT_CFLAGS";;
esac

dnl
dnl Output variables
dnl
dnl CPU_PENTIUM="false"

case $CPU_PENTIUM in
true) AC_DEFINE_UNQUOTED(ARCH_X86, , [X86 Architecture])
esac
AM_CONDITIONAL(ARCH_X86, test x$CPU_PENTIUM = xtrue)

AM_CONDITIONAL(HAVE_STANDALONE, test x$have_standalone = xtrue)

INCLUDES='-I$(top_builddir)/include'
AC_SUBST(INCLUDES)

AC_OUTPUT(Makefile \
gavl.spec \
gavl.pc \
src/Makefile \
include/Makefile \
include/gavlconfig.h \
include/gavl/Makefile \
gavl/Makefile \
gavl/libgdither/Makefile \
gavl/c/Makefile \
gavl/mmx/Makefile \
gavl/mmxext/Makefile)

