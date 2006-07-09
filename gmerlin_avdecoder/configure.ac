AC_INIT(gmerlin-avdecoder, 0.1.4)
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

GAVL_REQUIRED="0.2.4"

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

GMERLIN_REQUIRED="0.3.4"

PKG_CHECK_MODULES(GMERLIN, gmerlin >= $GMERLIN_REQUIRED, have_gmerlin="true",
have_gmerlin="false")

GMERLIN_PLUGIN_DIR=`pkg-config --variable=prefix gmerlin`"/lib/gmerlin/plugins"
AC_SUBST(GMERLIN_PLUGIN_DIR)
AC_SUBST(GMERLIN_REQUIRED)

AM_CONDITIONAL(HAVE_GMERLIN, test x$have_gmerlin = xtrue)

dnl
dnl AVCodec
dnl

AH_TEMPLATE([HAVE_LIBAVCODEC],
            [Do we have libavcodec installed?])

have_avcodec=false

AVCODEC_BUILD="3342336"

ACL_PATH_AVCODEC($AVCODEC_BUILD , have_avcodec="true", have_avcodec="false")
AVCODEC_REQUIRED=$AVCODEC_VERSION

AM_CONDITIONAL(HAVE_LIBAVCODEC, test x$have_avcodec = xtrue)

AC_SUBST(AVCODEC_REQUIRED)
AC_SUBST(AVCODEC_LIBS)
AC_SUBST(AVCODEC_CFLAGS)

if test "x$have_avcodec" = "xtrue"; then
AC_DEFINE([HAVE_LIBAVCODEC])
fi

dnl
dnl Check for theora
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
dnl Check for speex
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
dnl libmpeg2
dnl 

LIBMPEG2_REQUIRED="0.4.0"

have_libmpeg2=false
AH_TEMPLATE([HAVE_LIBMPEG2], [libmpeg2 found])

PKG_CHECK_MODULES(LIBMPEG2, libmpeg2 >= $LIBMPEG2_REQUIRED, have_libmpeg2=true, have_libmpeg2=false)

AM_CONDITIONAL(HAVE_LIBMPEG2, test x$have_libmpeg2 = xtrue)

if test x$have_libmpeg2 = xtrue; then
AC_DEFINE(HAVE_LIBMPEG2)
fi

AC_SUBST(LIBMPEG2_REQUIRED)

dnl
dnl libtiff
dnl

AH_TEMPLATE([HAVE_LIBTIFF], [Enable tiff codec])
 
have_libtiff=false
TIFF_REQUIRED="3.5.0"
 
OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="-ltiff"
CFLAGS=""
 
AC_MSG_CHECKING(for libtiff)
AC_TRY_LINK([#include <tiffio.h>],
            [TIFF * tiff = (TIFF*)0;
	     int i = 0;
	     /* We ensure the function is here but never call it */
             if(i)
	       TIFFReadRGBAImage(tiff, 0, 0, (uint32*)0, 0);
	     return 0;],
            [have_libtiff=true])
 
case $have_libtiff in
  true) AC_DEFINE(HAVE_LIBTIFF)
        AC_MSG_RESULT(yes)
        TIFF_LIBS=$LIBS;
        TIFF_CFLAGS=$CFLAGS ;;
  false) AC_MSG_RESULT(no); TIFF_LIBS=""; TIFF_CFLAGS="";;
esac
CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(TIFF_CFLAGS)
AC_SUBST(TIFF_LIBS)
AC_SUBST(TIFF_REQUIRED)

AM_CONDITIONAL(HAVE_LIBTIFF, test x$have_libtiff = xtrue)

if test x$have_libtiff = xtrue; then
AC_DEFINE(HAVE_LIBTIFF)
fi

dnl
dnl libsmbclient
dnl

AH_TEMPLATE([HAVE_SAMBA], [Samba support])
 
have_libsmbclient=false
SAMBA_REQUIRED="3.0.0"
 
OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="-lsmbclient"
CFLAGS=""
 
AC_MSG_CHECKING(for libsmbclient)
AC_TRY_LINK([#include <libsmbclient.h>
             #include <stdio.h>],
            [int i = 0;
	     /* We ensure the function is here but never call it */
             if(i)
		smbc_lseek(0,0,SEEK_SET);
	     return 0;],
            [have_libsmbclient=true])
 
case $have_libsmbclient in
  true) AC_DEFINE(HAVE_SAMBA)
        AC_MSG_RESULT(yes)
        SAMBA_LIBS=$LIBS;
        SAMBA_CFLAGS=$CFLAGS ;;
  false) AC_MSG_RESULT(no); SAMBA_LIBS=""; SAMBA_CFLAGS="";;
esac
CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(SAMBA_CFLAGS)
AC_SUBST(SAMBA_LIBS)
AC_SUBST(SAMBA_REQUIRED)

AM_CONDITIONAL(HAVE_SAMBA, test x$have_libsmbclient = xtrue)

if test x$have_libsmbclient = xtrue; then
AC_DEFINE(HAVE_SAMBA)
fi

dnl
dnl PNG
dnl 

AH_TEMPLATE([HAVE_LIBPNG], [Enable png codec])
 
have_libpng=false
PNG_REQUIRED="1.2.2"
 
OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="-lpng -lm -lz"
CFLAGS=""
 
AC_MSG_CHECKING(for libpng)
AC_TRY_LINK([#include <png.h>],
            [png_structp png_ptr;
             png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                               (png_voidp)0,
                                                NULL, NULL);],
            [have_libpng=true])
 
case $have_libpng in
  true) AC_DEFINE(HAVE_LIBPNG)
        AC_MSG_RESULT(yes)
        PNG_LIBS=$LIBS;
        PNG_CFLAGS=$CFLAGS ;;
  false) AC_MSG_RESULT(no); PNG_LIBS=""; PNG_CFLAGS="";;
esac
CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)
AC_SUBST(PNG_REQUIRED)

AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

if test x$have_libpng = xtrue; then
AC_DEFINE(HAVE_LIBPNG)
fi



dnl
dnl FAAD2
dnl

FAAD2_PREFIX=""
AC_ARG_WITH(faad2-prefix, [ --with-faad2-prefix=PFX   Prefix to search for faad2],FAAD2_PREFIX=${withval})

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

if test "x$FAAD2_PREFIX" = "x"; then 
CFLAGS="$GMERLIN_DEP_CFLAGS"
LIBS="$GMERLIN_DEP_LIBS -lfaad -lm"
else
CFLAGS="-I$FAAD2_PREFIX/include"
LIBS="-L$FAAD2_PREFIX/lib -lfaad -lm"
fi

FAAD2_REQUIRED="2.0"
AH_TEMPLATE([HAVE_FAAD2], [Enable FAAD2])
AC_MSG_CHECKING(for faad2)
have_faad2="false"

  AC_TRY_RUN([
    #include "faad.h"
    #include <stdio.h>
    main()
    {
    int faad_major;
    int faad_minor;
    faacDecHandle dec;

    if(sscanf(FAAD2_VERSION, "%d.%d", &faad_major, &faad_minor) < 2)
      return -1;
    dec = faacDecOpen();
    if(!dec)
      return -1;
    return 0;
    }
  ],
  [
    # program could be run
    have_faad2="true"
    AC_MSG_RESULT(yes)
    FAAD2_CFLAGS=$CFLAGS
    FAAD2_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(FAAD2_CFLAGS)
AC_SUBST(FAAD2_LIBS)
AC_SUBST(FAAD2_REQUIRED)
AM_CONDITIONAL(HAVE_FAAD2, test x$have_faad2 = xtrue)

if test x$have_faad2 = xtrue; then
AC_DEFINE(HAVE_FAAD2)
fi

dnl
dnl DVDREAD
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

CFLAGS="$GMERLIN_DEP_CFLAGS"
LIBS="$GMERLIN_DEP_LIBS -ldvdread"

DVDREAD_REQUIRED="0.9.5"
AH_TEMPLATE([HAVE_DVDREAD], [Enable libdvdread])
AC_MSG_CHECKING(for libdvdread >= 0.9.5)
have_dvdread="false"

  AC_TRY_RUN([
    #include <dvdread/dvd_reader.h>
    #include <stdio.h>
    main()
    {
#if DVDREAD_VERSION < 905
    return -1;
#else
    return 0;
#endif
    }
  ],
  [
    # program could be run
    have_dvdread="true"
    AC_MSG_RESULT(yes)
    DVDREAD_CFLAGS=""
    DVDREAD_LIBS="-ldvdread"

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(DVDREAD_CFLAGS)
AC_SUBST(DVDREAD_LIBS)
AC_SUBST(DVDREAD_REQUIRED)
AM_CONDITIONAL(HAVE_DVDREAD, test x$have_dvdread = xtrue)

if test x$have_dvdread = xtrue; then
AC_DEFINE(HAVE_DVDREAD)
fi


dnl
dnl FLAC
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
 
LIBS="-lFLAC -lm"
CFLAGS=""

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
dnl Musepack
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="$GMERLIN_DEP_LIBS -lmpcdec"
CFLAGS="$GMERLIN_DEP_CFLAGS"

MUSEPACK_REQUIRED="1.1"

AH_TEMPLATE([HAVE_MUSEPACK], [Enable Musepack])
AC_MSG_CHECKING(for libmpcdec)
have_musepack="false"

  AC_TRY_RUN([
    #include <mpcdec/mpcdec.h>
    #include <stdio.h>
    main()
    {
    mpc_streaminfo si;
    mpc_streaminfo_init(&si);
    return 0;
    }
  ],
  [
    # program could be run
    have_musepack="true"
    AC_MSG_RESULT(yes)
    MUSEPACK_CFLAGS=$CFLAGS
    MUSEPACK_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(MUSEPACK_CFLAGS)
AC_SUBST(MUSEPACK_LIBS)
AC_SUBST(MUSEPACK_REQUIRED)

AM_CONDITIONAL(HAVE_MUSEPACK, test x$have_musepack = xtrue)

if test x$have_musepack = xtrue; then
AC_DEFINE(HAVE_MUSEPACK)
fi


dnl
dnl MAD
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

MAD_REQUIRED="0.15.0"
   
LIBS="$GMERLIN_DEP_LIBS -lmad"
CFLAGS="$GMERLIN_DEP_CFLAGS"

AH_TEMPLATE([HAVE_MAD], [Enable MAD])
AC_MSG_CHECKING(for libmad 0.15.x)
have_mad="false"

  AC_TRY_RUN([
    #include "mad.h"
    #include <stdio.h>
    main()
    {
    struct mad_stream stream;
    int version_major = MAD_VERSION_MAJOR;
    int version_minor = MAD_VERSION_MINOR;
    if((version_major != 0) || (version_minor != 15))
      return -1;
    mad_stream_init(&stream);
    return 0;
    }
  ],
  [
    # program could be run
    have_mad="true"
    AC_MSG_RESULT(yes)
    MAD_CFLAGS=$CFLAGS
    MAD_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(MAD_CFLAGS)
AC_SUBST(MAD_LIBS)
AC_SUBST(MAD_REQUIRED)

AM_CONDITIONAL(HAVE_MAD, test x$have_mad = xtrue)

if test x$have_mad = xtrue; then
AC_DEFINE(HAVE_MAD)
fi

dnl
dnl liba52
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
 
LIBS="$GMERLIN_DEP_LIBS -la52 -lm"
CFLAGS="$GMERLIN_DEP_CFLAGS"
LIBA52_REQUIRED="0.7.4"
AH_TEMPLATE([HAVE_LIBA52], [Enable liba52])
AC_MSG_CHECKING(for liba52)
have_liba52="false"

  AC_TRY_RUN([
    #include <inttypes.h>
    #include <a52dec/a52.h>
    main()
    {
    a52_state_t * state = a52_init(0);
    return 0;
    }
  ],
  [
    # program could be run
    have_liba52="true"
    AC_MSG_RESULT(yes)
    LIBA52_CFLAGS=$CFLAGS
    LIBA52_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(LIBA52_CFLAGS)
AC_SUBST(LIBA52_LIBS)
AC_SUBST(LIBA52_REQUIRED)

AM_CONDITIONAL(HAVE_LIBA52, test x$have_liba52 = xtrue)

if test x$have_liba52 = xtrue; then
AC_DEFINE(HAVE_LIBA52)
fi

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
dnl CDrom support
dnl

AH_TEMPLATE([HAVE_CDIO], [ libcdio found ])

have_cdio="false"
CDIO_REQUIRED="0.77"

PKG_CHECK_MODULES(CDIO, libcdio >= $CDIO_REQUIRED, have_cdio="true", have_cdio="false")
AM_CONDITIONAL(HAVE_CDIO, test x$have_cdio = xtrue)
AC_SUBST(CDIO_REQUIRED)

if test "x$have_cdio" = "xtrue"; then
AC_DEFINE([HAVE_CDIO])
fi

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
else
echo "Missing (Go to http://gmerlin.sourceforge.net)"
fi

echo -n "libavcodec (ffmpeg):    "
if test "x$have_avcodec" = "xtrue"; then
echo "Found (CFLAGS=$AVCODEC_CFLAGS LIBS=$AVCODEC_LIBS)"
else
echo "Missing (Go to http://www.ffmpeg.org/)"
fi

echo -n "faad2:                  "
if test "x$have_faad2" = "xtrue"; then
echo "Found (CFLAGS=$FAAD2_CFLAGS LIBS=$FAAD2_LIBS)"
else
echo "Missing (Go to http://www.audiocoding.com/)"
fi

echo -n "flac:                   "
if test "x$have_flac" = "xtrue"; then
echo "Found (CFLAGS=$FLAC_CFLAGS LIBS=$FLAC_LIBS)"
else
echo "Missing (Go to http://flac.sourceforge.net/)"
fi

echo -n "vorbis:                 "
if test "x$have_vorbis" = "xtrue"; then
echo "Found (CFLAGS=$VORBIS_CFLAGS LIBS=$VORBIS_LIBS)"
else
echo "Missing (Go to http://www.vorbis.com/)"
fi

echo -n "theora:                 "
if test "x$have_theora" = "xtrue"; then
echo "Found (CFLAGS=$THEORA_CFLAGS LIBS=$THEORA_LIBS)"
else
echo "Missing (Go to http://www.theora.org/)"
fi

echo -n "speex:                  "
if test "x$have_speex" = "xtrue"; then
echo "Found (CFLAGS=$SPEEX_CFLAGS LIBS=$SPEEX_LIBS)"
else
echo "Missing (Go to http://www.speex.org/)"
fi

echo -n "libpng:                 "
if test "x$have_libpng" = "xtrue"; then
echo "Found (CFLAGS=$PNG_CFLAGS LIBS=$PNG_LIBS)"
else
echo "Missing (Go to http://www.libpng.org/pub/png/libpng.html)"
fi

echo -n "libtiff:                "
if test "x$have_libtiff" = "xtrue"; then
echo "Found (CFLAGS=$TIFF_CFLAGS LIBS=$TIFF_LIBS)"
else
echo "Missing (Go to http://www.remotesensing.org/libtiff/)"
fi

echo -n "libmpeg2 (mpeg2dec)     "
if test "x$have_libmpeg2" = "xtrue"; then
echo "Found (CFLAGS=$LIBMPEG2_CFLAGS LIBS=$LIBMPEG2_LIBS)"
else
echo "Missing (Go to http://libmpeg2.sourceforge.net/)"
fi

echo -n "libmad 0.15.x:          "
if test "x$have_mad" = "xtrue"; then
echo "Found (CFLAGS=$MAD_CFLAGS LIBS=$MAD_LIBS)"
else
echo "Missing (Go to http://mad.sourceforge.net/)"
fi

echo -n "liba52 (a52dec):        "
if test "x$have_liba52" = "xtrue"; then
echo "Found (CFLAGS=$LIBA52_CFLAGS LIBS=$LIBA52_LIBS)"
else
echo "Missing (Go to http://liba52.sourceforge.net/)"
fi

echo -n "libmpcdec:              "
if test "x$have_musepack" = "xtrue"; then
echo "Found (CFLAGS=$MUSEPACK_CFLAGS LIBS=$MUSEPACK_LIBS)"
else
echo "Missing (Go to http://www.musepack.net/)"
fi

echo -n "libcdio:                "
if test "x$have_cdio" = "xtrue"; then
echo "Found (CFLAGS=$CDIO_CFLAGS LIBS=$CDIO_LIBS)"
else
echo "Missing (Go to http://www.gnu.org/software/libcdio/)"
fi

echo -n "mjpegtools:             "
if test "x$have_mjpegtools" = "xtrue"; then
echo "Found (CFLAGS=$MJPEGTOOLS_CFLAGS LIBS=$MJPEGTOOLS_LIBS)"
else
echo "Missing (Go to http://mjpeg.sourceforge.net/)"
fi

echo -n "samba:                  "
if test "x$have_libsmbclient" = "xtrue"; then
echo "Found (CFLAGS=$SAMBA_CFLAGS LIBS=$SAMBA_LIBS)"
else
echo "Missing (Go to http://www.samba.org)"
fi

echo -n "libdvdread:             "
if test "x$have_dvdread" = "xtrue"; then
echo "Found (CFLAGS=$DVDREAD_CFLAGS LIBS=$DVDREAD_LIBS)"
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
