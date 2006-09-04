AC_INIT(gmerlin-avdecoder, 0.1.5)
AC_CONFIG_SRCDIR([include/avdec.h])
AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(1.8.5)

AC_DISABLE_STATIC
AM_PROG_AS_MOD
AC_PROG_LIBTOOL

dnl
dnl General stuff
dnl

AC_CHECK_HEADERS(byteswap.h)

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
AC_CHECK_FUNCS(ftello vasprintf)


dnl
dnl Check for Dependencies package
dnl

GMERLIN_DEP_DIR=/opt/gmerlin

if test -d $GMERLIN_DEP_DIR; then
  have_opt_gmerlin="true"
  export PKG_CONFIG_PATH=$GMERLIN_DEP_DIR/lib/pkgconfig:$PKG_CONFIG_PATH
  GMERLIN_DEP_CFLAGS="-I$GMERLIN_DEP_DIR/include"
  GMERLIN_DEP_RPATH="-Wl,--rpath -Wl,$GMERLIN_DEP_DIR/lib"
  GMERLIN_DEP_LIBS="-L$GMERLIN_DEP_DIR/lib"
else
  have_opt_gmerlin="false"
  GMERLIN_DEP_CFLAGS=""
  GMERLIN_DEP_LDPATH=""
  GMERLIN_DEP_LIBS=""
fi

AC_SUBST(GMERLIN_DEP_LIBS)
LDFLAGS="$GMERLIN_DEP_RPATH"


dnl
dnl Check for Libraries
dnl

GAVL_REQUIRED="0.2.5"

PKG_CHECK_MODULES(GAVL, gavl = $GAVL_REQUIRED, , AC_MSG_ERROR("gavl not found"))
AC_SUBST(GAVL_REQUIRED)

ICONV_LIBS=
AC_CHECK_LIB(iconv, libiconv_close, ICONV_LIBS="-liconv")
AC_SUBST(ICONV_LIBS)

dnl
dnl Optional Libraries
dnl

dnl
dnl gmerlin
dnl 

have_gmerlin="false"

GMERLIN_REQUIRED="0.3.5"

AC_ARG_ENABLE(gmerlin,
[AC_HELP_STRING([--disable-gmerlin],[Disable gmerlin plugins (default: autodetect)])],
[case "${enableval}" in
   yes) test_gmerlin=true ;;
   no)  test_gmerlin=false ;;
esac],[test_gmerlin=true])

if test x$test_gmerlin = xtrue; then

PKG_CHECK_MODULES(GMERLIN, gmerlin >= $GMERLIN_REQUIRED, have_gmerlin="true",
have_gmerlin="false")

GMERLIN_PLUGIN_DIR=`pkg-config --variable=prefix gmerlin`"/lib/gmerlin/plugins"

fi

AC_SUBST(GMERLIN_PLUGIN_DIR)
AC_SUBST(GMERLIN_REQUIRED)

AM_CONDITIONAL(HAVE_GMERLIN, test x$have_gmerlin = xtrue)

dnl
dnl AVCodec
dnl

GMERLIN_CHECK_AVCODEC

dnl
dnl Check for theora
dnl

GMERLIN_CHECK_THEORA

dnl
dnl Check for speex
dnl

GMERLIN_CHECK_SPEEX

dnl
dnl Check for mjpegtools
dnl

GMERLIN_CHECK_MJPEGTOOLS

dnl
dnl Vorbis
dnl 

GMERLIN_CHECK_VORBIS

dnl
dnl libmpeg2
dnl 

GMERLIN_CHECK_LIBMPEG2

dnl
dnl libtiff
dnl

GMERLIN_CHECK_LIBTIFF

dnl
dnl libsmbclient
dnl

GMERLIN_CHECK_SAMBA

dnl
dnl PNG
dnl 

GMERLIN_CHECK_LIBPNG

dnl
dnl FAAD2
dnl

GMERLIN_CHECK_FAAD2

dnl
dnl DVDREAD
dnl

GMERLIN_CHECK_DVDREAD

dnl
dnl FLAC
dnl

GMERLIN_CHECK_FLAC

dnl
dnl Musepack
dnl

GMERLIN_CHECK_MUSEPACK

dnl
dnl MAD
dnl

GMERLIN_CHECK_MAD

dnl
dnl liba52
dnl

GMERLIN_CHECK_LIBA52

dnl
dnl CDrom support
dnl

GMERLIN_CHECK_CDIO

dnl
dnl Check wether to enable win32 DLLs
dnl We compile them, if we have an intel architecture and
dnl gcc. Other libs make a more complicated check here.
dnl

AH_TEMPLATE([HAVE_W32DLL], [ Win32 dll loader enabled ])

enable_w32dll="false"

if test x"$GCC" = x"yes"; then
 case "$host_cpu" in
    i?86 | k?-* | athlon | pentium) enable_w32dll="true"
 esac
fi

if test "x$enable_w32dll" = "xtrue"; then
AC_DEFINE([HAVE_W32DLL])
fi

AM_CONDITIONAL(HAVE_W32DLL, test x"$enable_w32dll" = "xtrue")

W32_NO_OPTIMIZE=""
AC_SUBST(W32_NO_OPTIMIZE)

dnl
dnl For win32 libraries location, needed by libw32dll.
dnl
                                                                                          
AC_ARG_WITH(w32-path,[  --with-w32-path=path    Location of WIN32 libraries],
            w32_path="$withval", w32_path="/usr/lib/win32")
AC_SUBST(w32_path)

AC_CHECK_HEADERS(sys/mman.h)

dnl
dnl Real DLLs loader
dnl

AH_TEMPLATE([HAVE_REALDLL], [ RealPlayer dll loader enabled ])

enable_realdll="false"

if test x"$GCC" = x"yes"; then
 case "$host_cpu" in
    i?86 | k?-* | athlon | pentium) enable_realdll="true"
 esac
fi

if test "x$enable_realdll" = "xtrue"; then
AC_DEFINE([HAVE_REALDLL])
REALDLL_LIBS="-ldl"
else
REALDLL_LIBS=""
fi

AM_CONDITIONAL(HAVE_REALDLL, test x"$enable_realdll" = "xtrue")
AC_SUBST(REALDLL_LIBS)

dnl
dnl Xanim DLLs loader
dnl

AH_TEMPLATE([HAVE_XADLL], [ Xanim dll loader enabled ])

enable_xadll="false"

if test x"$GCC" = x"yes"; then
 case "$host_cpu" in
    i?86 | k?-* | athlon | pentium) enable_xadll="true"
 esac
fi

if test "x$enable_xadll" = "xtrue"; then
AC_DEFINE([HAVE_XADLL])
XADLL_LIBS="-ldl"
else
XADLL_LIBS=""
fi

AC_SUBST(XADLL_LIBS)
AM_CONDITIONAL(HAVE_XADLL, test x"$enable_xadll" = "xtrue")


dnl
dnl Build optimization flags
dnl

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])
AC_SUBST(OPT_CFLAGS)

dnl
dnl Common CFLAGS
dnl

CFLAGS="-Wall -Wmissing-declarations -D_REENTRANT -D_FILE_OFFSET_BITS=64 $GAVL_CFLAGS"

dnl The following is necessary so the win32 dll loader also gets compiled
dnl with -g

if test x$LQT_DEBUG = xtrue; then
  CFLAGS="-g $CFLAGS"
fi

dnl
dnl Common libs
dnl 

LIBS="$LIBS $GAVL_LIBS"

dnl
dnl Output
dnl 

AC_CONFIG_FILES([Makefile \
gmerlin_avdec.spec \
gmerlin_avdec.pc \
include/Makefile \
lib/Makefile \
lib/libw32dll/Makefile \
lib/libw32dll/dmo/Makefile \
lib/libw32dll/qtx/Makefile \
lib/libw32dll/qtx/qtxsdk/Makefile \
lib/libw32dll/wine/Makefile \
lib/libw32dll/DirectShow/Makefile \
lib/GSM610/Makefile \
m4/Makefile \
plugins/Makefile \
tests/Makefile ])

AC_OUTPUT

echo ""
echo "=== Libraries: ================================="

echo -n "gmerlin:                "
if test "x$have_gmerlin" = "xtrue"; then
echo "Found (CFLAGS=$GMERLIN_CFLAGS LIBS=$GMERLIN_LIBS)"
elif test "x$test_gmerlin" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://gmerlin.sourceforge.net)"
fi

echo -n "libavcodec (ffmpeg):    "
if test "x$have_avcodec" = "xtrue"; then
echo "Found (CFLAGS=$AVCODEC_CFLAGS LIBS=$AVCODEC_LIBS)"
elif test "x$test_avcodec" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.ffmpeg.org/)"
fi

echo -n "faad2:                  "
if test "x$have_faad2" = "xtrue"; then
echo "Found (CFLAGS=$FAAD2_CFLAGS LIBS=$FAAD2_LIBS)"
elif test "x$test_faad2" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.audiocoding.com/)"
fi

echo -n "flac:                   "
if test "x$have_flac" = "xtrue"; then
echo "Found (CFLAGS=$FLAC_CFLAGS LIBS=$FLAC_LIBS)"
elif test "x$test_flac" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://flac.sourceforge.net/)"
fi

echo -n "vorbis:                 "
if test "x$have_vorbis" = "xtrue"; then
echo "Found (CFLAGS=$VORBIS_CFLAGS LIBS=$VORBIS_LIBS)"
elif test "x$test_vorbis" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.vorbis.com/)"
fi

echo -n "theora:                 "
if test "x$have_theora" = "xtrue"; then
echo "Found (CFLAGS=$THEORA_CFLAGS LIBS=$THEORA_LIBS)"
elif test "x$test_theora" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.theora.org/)"
fi

echo -n "speex:                  "
if test "x$have_speex" = "xtrue"; then
echo "Found (CFLAGS=$SPEEX_CFLAGS LIBS=$SPEEX_LIBS)"
elif test "x$test_speex" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.speex.org/)"
fi

echo -n "libpng:                 "
if test "x$have_libpng" = "xtrue"; then
echo "Found (CFLAGS=$PNG_CFLAGS LIBS=$PNG_LIBS)"
elif test "x$test_libpng" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.libpng.org/pub/png/libpng.html)"
fi

echo -n "libtiff:                "
if test "x$have_libtiff" = "xtrue"; then
echo "Found (CFLAGS=$TIFF_CFLAGS LIBS=$TIFF_LIBS)"
elif test "x$test_libtiff" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.remotesensing.org/libtiff/)"
fi

echo -n "libmpeg2 (mpeg2dec)     "
if test "x$have_libmpeg2" = "xtrue"; then
echo "Found (CFLAGS=$LIBMPEG2_CFLAGS LIBS=$LIBMPEG2_LIBS)"
elif test "x$test_libmpeg2" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://libmpeg2.sourceforge.net/)"
fi

echo -n "libmad 0.15.x:          "
if test "x$have_mad" = "xtrue"; then
echo "Found (CFLAGS=$MAD_CFLAGS LIBS=$MAD_LIBS)"
elif test "x$test_mad" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://mad.sourceforge.net/)"
fi

echo -n "liba52 (a52dec):        "
if test "x$have_liba52" = "xtrue"; then
echo "Found (CFLAGS=$LIBA52_CFLAGS LIBS=$LIBA52_LIBS)"
elif test "x$test_liba52" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://liba52.sourceforge.net/)"
fi

echo -n "libmpcdec:              "
if test "x$have_musepack" = "xtrue"; then
echo "Found (CFLAGS=$MUSEPACK_CFLAGS LIBS=$MUSEPACK_LIBS)"
elif test "x$test_musepack" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.musepack.net/)"
fi

echo -n "libcdio:                "
if test "x$have_cdio" = "xtrue"; then
echo "Found (CFLAGS=$CDIO_CFLAGS LIBS=$CDIO_LIBS)"
elif test "x$test_cdio" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.gnu.org/software/libcdio/)"
fi

echo -n "mjpegtools:             "
if test "x$have_mjpegtools" = "xtrue"; then
echo "Found (CFLAGS=$MJPEGTOOLS_CFLAGS LIBS=$MJPEGTOOLS_LIBS)"
elif test "x$test_mjpegtools" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://mjpeg.sourceforge.net/)"
fi

echo -n "samba:                  "
if test "x$have_libsmbclient" = "xtrue"; then
echo "Found (CFLAGS=$SAMBA_CFLAGS LIBS=$SAMBA_LIBS)"
elif test "x$test_libsmbclient" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.samba.org)"
fi

echo -n "libdvdread:             "
if test "x$have_dvdread" = "xtrue"; then
echo "Found (CFLAGS=$DVDREAD_CFLAGS LIBS=$DVDREAD_LIBS)"
elif test "x$test_dvdread" = "xfalse"; then
echo "Disabled"
else
echo "Missing (Go to http://www.dtek.chalmers.se/groups/dvd/downloads.shtml)"
fi

echo ""
echo "=== Other features: ============================"

echo -n "win32 DLL Support:      "
if test "x$enable_w32dll" = "xtrue"; then
echo "Enabled"
else
echo "Disabled (Unsupported Architecture)"
fi

echo -n "RealPLayer DLL Support: "
if test "x$enable_realdll" = "xtrue"; then
echo "Enabled"
else
echo "Disabled (Unsupported Architecture)"
fi

echo -n "Xanim DLL Support:      "
if test "x$enable_xadll" = "xtrue"; then
echo "Enabled"
else
echo "Disabled (Unsupported Architecture)"
fi

echo
echo "If you installed a library but it was not detected, check the file INSTALL"
echo "for troubleshooting tips. Also note that if the configure script reaches"
echo "this point, all missing packages are optional so compilation should succeed"
echo "anyway."
