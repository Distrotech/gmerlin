AC_INIT
AC_CONFIG_SRCDIR([include/gmerlin_encoders.h])

AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(gmerlin-encoders, 0.2.2)

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
  GMERLIN_DEP_LIBS="-L$GMERLIN_DEP_DIR/lib"
  GMERLIN_DEP_RPATH="-Wl,--rpath -Wl,$GMERLIN_DEP_DIR/lib"
else
  have_opt_gmerlin="false"
  GMERLIN_DEP_CFLAGS=""
  GMERLIN_DEP_LDPATH=""
  GMERLIN_DEP_LIBS=""
  GMERLIN_DEP_RPATH=""
fi

LDFLAGS="-Wl,--rpath -Wl,$GMERLIN_DEP_DIR/lib"
AC_SUBST(GMERLIN_DEP_LIBS)

dnl
dnl Required libraries
dnl

GMERLIN_REQUIRED="0.3.4"
PKG_CHECK_MODULES(GMERLIN, gmerlin >= $GMERLIN_REQUIRED, , AC_MSG_ERROR("gmerlin not found"))
have_gmerlin="false"

GMERLIN_PLUGIN_DIR=`pkg-config --variable=prefix gmerlin`"/lib/gmerlin/plugins"
AC_SUBST(GMERLIN_PLUGIN_DIR)
AC_SUBST(GMERLIN_REQUIRED)

dnl
dnl Optional libraries
dnl

dnl
dnl Ogg
dnl

OGG_REQUIRED="1.1"

have_ogg=false
AH_TEMPLATE([HAVE_OGG], [Ogg libraries are there])

XIPH_PATH_OGG(have_ogg=true)
AM_CONDITIONAL(HAVE_OGG, test x$have_ogg = xtrue)

if test x$have_ogg = xtrue; then
AC_DEFINE(HAVE_OGG)
fi

AC_SUBST(OGG_REQUIRED)

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
dnl Theora
dnl

AH_TEMPLATE([HAVE_THEORA],
            [Do we have theora installed?])

have_theora="false"

THEORA_REQUIRED="1.0alpha5"

PKG_CHECK_MODULES(THEORA, theora >= $THEORA_REQUIRED, have_theora="true", have_theora="false")
AC_SUBST(THEORA_REQUIRED)
AC_SUBST(THEORA_LIBS)
AC_SUBST(THEORA_CFLAGS)

AM_CONDITIONAL(HAVE_THEORA, test x$have_theora = xtrue)

if test "x$have_theora" = "xtrue"; then
AC_DEFINE([HAVE_THEORA])
fi

dnl
dnl Speex
dnl

AH_TEMPLATE([HAVE_SPEEX],
            [Do we have speex installed?])

have_speex="false"

SPEEX_REQUIRED="1.0.4"

PKG_CHECK_MODULES(SPEEX, speex >= $SPEEX_REQUIRED, have_speex="true", have_speex="false")
AC_SUBST(SPEEX_REQUIRED)
AC_SUBST(SPEEX_LIBS)
AC_SUBST(SPEEX_CFLAGS)

AM_CONDITIONAL(HAVE_SPEEX, test x$have_speex = xtrue)

if test "x$have_speex" = "xtrue"; then
AC_DEFINE([HAVE_SPEEX])
fi

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

have_faac="false"
FAAC_REQUIRED="1.24"

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

AH_TEMPLATE([HAVE_FAAC], [Enable faac])

LIBS="$GMERLIN_DEP_LIBS -lfaac -lm"
CFLAGS="$GMERLIN_DEP_CFLAGS"

AC_MSG_CHECKING(for faac)
AC_TRY_RUN([
    #include <inttypes.h>
    #include <faac.h>
    main()
    {
    int samplerate = 44100, num_channels = 2;
    unsigned long input_samples, output_bytes;
    faacEncHandle enc;
    enc = faacEncOpen(samplerate,
                      num_channels,
                      &input_samples,
                      &output_bytes);

    return 0;
    }
  ],
  [
    # program could be run
    have_faac="true"
    AC_MSG_RESULT(yes)
    FAAC_CFLAGS=$CFLAGS
    FAAC_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

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
dnl mjpegtools
dnl

dnl
dnl Check for mjpegtools
dnl

AH_TEMPLATE([HAVE_MJPEGTOOLS],
            [Do we have mjpegtools installed?])

have_mjpegtools="false"

MJPEGTOOLS_REQUIRED="1.9.0"

PKG_CHECK_MODULES(MJPEGTOOLS, mjpegtools >= $MJPEGTOOLS_REQUIRED, have_mjpegtools="true", have_mjpegtools="false")
AC_SUBST(MJPEGTOOLS_REQUIRED)
AC_SUBST(MJPEGTOOLS_LIBS)
AC_SUBST(MJPEGTOOLS_CFLAGS)

AM_CONDITIONAL(HAVE_MJPEGTOOLS, test x$have_mjpegtools = xtrue)

if test "x$have_mjpegtools" = "xtrue"; then
AC_DEFINE([HAVE_MJPEGTOOLS])
fi

dnl
dnl CFLAGS
dnl

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])
CFLAGS="-Wall -Wmissing-declarations -D_REENTRANT -D_FILE_OFFSET_BITS=64 $OPT_CFLAGS $GMERLIN_CFLAGS"

dnl
dnl LIBS
dnl

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
m4/Makefile \
plugins/Makefile \
plugins/flac/Makefile \
plugins/ogg/Makefile \
plugins/lame/Makefile \
plugins/faac/Makefile \
plugins/yuv4mpeg/Makefile \
])

AC_OUTPUT

echo ""
echo "=== Libraries: ================================="

echo -n "flac:       "
if test "x$have_flac" = "xtrue"; then
echo "Found (CFLAGS=$FLAC_CFLAGS LIBS=$FLAC_LIBS)"
else
echo "Missing (Go to: http://flac.sourceforge.net/)"
fi

echo -n "ogg:        "
if test "x$have_ogg" = "xtrue"; then
echo "Found (CFLAGS=$OGG_CFLAGS LIBS=$OGG_LIBS)"
else
echo "Missing (Go to: http://www.xiph.org/)"
fi

echo -n "vorbis:     "
if test "x$have_vorbis" = "xtrue"; then
echo "Found (CFLAGS=$VORBIS_CFLAGS LIBS=$VORBIS_LIBS)"
else
echo "Missing (Go to: http://www.vorbis.com/)"
fi

echo -n "theora:     "
if test "x$have_theora" = "xtrue"; then
echo "Found (CFLAGS=$THEORA_CFLAGS LIBS=$THEORA_LIBS)"
else
echo "Missing (Go to: http://www.theora.org/)"
fi

echo -n "speex:      "
if test "x$have_speex" = "xtrue"; then
echo "Found (CFLAGS=$SPEEX_CFLAGS LIBS=$SPEEX_LIBS)"
else
echo "Missing (Go to: http://www.speex.org/)"
fi


echo -n "lame:       "
if test "x$have_lame" = "xtrue"; then
echo "Found (CFLAGS=$LAME_CFLAGS LIBS=$LAME_LIBS)"
else
echo "Missing (Go to: http://www.mp3dev.org/)"
fi

echo -n "faac:       "
if test "x$have_faac" = "xtrue"; then
echo "Found (CFLAGS=$FAAC_CFLAGS LIBS=$FAAC_LIBS)"
else
echo "Missing (Go to: http://www.audiocoding.com/)"
fi

echo -n "mjpegtools: "
if test "x$have_mjpegtools" = "xtrue"; then
echo "Found (CFLAGS=$MJPEGTOOLS_CFLAGS LIBS=$MJPEGTOOLS_LIBS)"
else
echo "Missing (Go to: http://mjpeg.sourceforge.net/)"
fi

echo
echo "If you installed a library but it was not detected, check the file INSTALL"
echo "for troubleshooting tips. Also note that if the configure script reaches"
echo "this point, all missing packages are optional so compilation should succeed"
echo "anyway."
