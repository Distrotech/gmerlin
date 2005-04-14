AC_INIT
AC_CONFIG_SRCDIR([include/gmerlin_encoders.h])

AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(gmerlin_encoders, 0.1.0)

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

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
else
  have_opt_gmerlin="false"
  GMERLIN_DEP_CFLAGS=""
  GMERLIN_DEP_LDPATH=""
  GMERLIN_DEP_LIBS=""
fi

dnl
dnl Required libraries
dnl

GMERLIN_REQUIRED="0.3.2pre2"
PKG_CHECK_MODULES(GMERLIN, gmerlin >= $GMERLIN_REQUIRED, , AC_MSG_ERROR("gmerlin not found"))
have_gmerlin="false"

GMERLIN_PLUGIN_DIR=`pkg-config --variable=prefix gmerlin`"/lib/gmerlin/plugins"
AC_SUBST(GMERLIN_PLUGIN_DIR)
AC_SUBST(GMERLIN_REQUIRED)

dnl
dnl Optional libraries
dnl

dnl
dnl Vorbis
dnl

VORBIS_REQUIRED="1.0"

have_vorbis=false
AH_TEMPLATE([HAVE_VORBIS], [Vorbis libraries are there])

XIPH_PATH_VORBIS(have_vorbis=true)
AM_CONDITIONAL(HAVE_VORBIS, test x$have_vorbis = xtrue)

if test x$have_vorbis = xtrue; then
AC_DEFINE(HAVE_VORBIS)
fi

AC_SUBST(VORBIS_REQUIRED)

dnl
dnl FLAC
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="$GMERLIN_DEP_LIBS -L/usr/local/lib -lFLAC -lm"
CFLAGS="$GMERLIN_DEP_CFLAGS -I/usr/local/include"

FLAC_REQUIRED="1.1.0"

AH_TEMPLATE([HAVE_FLAC], [Enable FLAC])
AC_MSG_CHECKING(for flac)
have_flac="false"

  AC_TRY_RUN([
    #include <FLAC/stream_decoder.h>
    #include <stdio.h>
    main()
    {
    int version_major;
    int version_minor;
    int version_patchlevel;
    fprintf(stderr, "FLAC__VERSION_STRING: %s\n", FLAC__VERSION_STRING);
    if(sscanf(FLAC__VERSION_STRING, "%d.%d.%d", &version_major,
              &version_minor, &version_patchlevel) < 3)
      return -1;
    if((version_major != 1) || (version_minor < 1))
      return 1;
    return 0;
    }
  ],
  [
    # program could be run
    have_flac="true"
    AC_MSG_RESULT(yes)
    FLAC_CFLAGS=$CFLAGS
    FLAC_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(FLAC_CFLAGS)
AC_SUBST(FLAC_LIBS)
AC_SUBST(FLAC_REQUIRED)

AM_CONDITIONAL(HAVE_FLAC, test x$have_flac = xtrue)

if test x$have_flac = xtrue; then
AC_DEFINE(HAVE_FLAC)
fi

dnl
dnl lame
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

if test x$have_vorbis = xtrue; then
LIBS="$GMERLIN_DEP_LIBS -lmp3lame -lvorbis -lm"
else
LIBS="$GMERLIN_DEP_LIBS -lmp3lame -lm"
fi

CFLAGS="$GMERLIN_DEP_CFLAGS"

LAME_REQUIRED="3.93"

AH_TEMPLATE([HAVE_LAME], [Enable lame])
AC_MSG_CHECKING(for lame)
have_lame="false"

  AC_TRY_RUN([
    #include <lame/lame.h>
    #include <stdio.h>
    main()
    {
    int version_major;
    int version_minor;
    const char * version;
    version = get_lame_version();
    fprintf(stderr, "lame version: %s\n", version);
    if(sscanf(version, "%d.%d", &version_major,
              &version_minor) < 2)
      return -1;
    if((version_major != 3) || (version_minor < 93))
      return 1;
    return 0;
    }
  ],
  [
    # program could be run
    have_lame="true"
    AC_MSG_RESULT(yes)
    LAME_CFLAGS=$CFLAGS
    LAME_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(LAME_CFLAGS)
AC_SUBST(LAME_LIBS)
AC_SUBST(LAME_REQUIRED)

AM_CONDITIONAL(HAVE_LAME, test x$have_lame = xtrue)

if test x$have_lame = xtrue; then
AC_DEFINE(HAVE_LAME)
fi

dnl
dnl faac
dnl

have_faac="true"
FAAC_REQUIRED="1.24"

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

AH_TEMPLATE([HAVE_FAAC], [Enable faac])

AC_CHECK_LIB(faac,faacEncOpen, , have_faac="false", -lm)

FAAC_CFLAGS="$GMERLIN_DEP_CFLAGS"
FAAC_LIBS="$GMERLIN_DEP_LIBS $LIBS -lm"

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(FAAC_CFLAGS)
AC_SUBST(FAAC_LIBS)
AC_SUBST(FAAC_REQUIRED)

AM_CONDITIONAL(HAVE_FAAC, test x$have_faac = xtrue)

if test x$have_faac = xtrue; then
AC_DEFINE(HAVE_FAAC)
fi

dnl
dnl Common CFLAGS
dnl

CFLAGS="-Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 $GMERLIN_CFLAGS"

dnl
dnl Build optimization flags
dnl

OPT_CFLAGS=""
TRY_CFLAGS=""

CPU_PENTIUM=false

if test x"$GCC" = x"yes"; then
  CFLAGS="-Wall $CFLAGS"
  TRY_CFLAGS="-O3"
  AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])
  TRY_CFLAGS="-finline-functions"
  AC_TRY_CFLAGS([$TRY_CFLAGS],[OPT_CFLAGS=$TRY_CFLAGS])
  dnl arch-specific flags
  case "$host_cpu" in
    i386)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=i386";;
    i486)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=i486";;
    i586)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=pentium"; CPU_PENTIUM=true ;;
    i686)           TRY_CFLAGS="$OPT_CFLAGS -mcpu=pentiumpro"; CPU_PENTIUM=true
;;
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
  true) CFLAGS="-g $CFLAGS"; OPT_CFLAGS="";;
esac

AC_SUBST(OPT_CFLAGS)

LIBS="$GMERLIN_LIBS"

if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix="${prefix}"
fi

AC_CONFIG_FILES([Makefile \
gmerlin_encoders.spec \
include/Makefile \
lib/Makefile \
plugins/Makefile \
plugins/flac/Makefile \
plugins/vorbis/Makefile \
plugins/lame/Makefile \
plugins/faac/Makefile ])

AC_OUTPUT

echo ""
echo "=== Libraries: ================================="

echo -n "flac:   "
if test "x$have_flac" = "xtrue"; then
echo "Found (CFLAGS=$FLAC_CFLAGS LIBS=$FLAC_LIBS)"
else
echo "Missing"
fi

echo -n "vorbis: "
if test "x$have_vorbis" = "xtrue"; then
echo "Found (CFLAGS=$VORBIS_CFLAGS LIBS=$VORBIS_LIBS)"
else
echo "Missing"
fi

echo -n "lame:   "
if test "x$have_lame" = "xtrue"; then
echo "Found (CFLAGS=$LAME_CFLAGS LIBS=$LAME_LIBS)"
else
echo "Missing"
fi

echo -n "faac:   "
if test "x$have_faac" = "xtrue"; then
echo "Found (CFLAGS=$FAAC_CFLAGS LIBS=$FAAC_LIBS)"
else
echo "Missing"
fi

echo
echo "If you installed a library but it was not detected, check the file INSTALL"
echo "for troubleshooting tips"

