AC_INIT
AC_CONFIG_SRCDIR([include/avdec.h])

AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(gmerlin_avdec, 0.1.1pre1)

AC_DISABLE_STATIC
AM_PROG_AS_MOD
AC_PROG_LIBTOOL

dnl
dnl General stuff
dnl

AC_CHECK_HEADERS(byteswap.h)

dnl
dnl Check for Libraries
dnl

GAVL_REQUIRED="0.2.0"

PKG_CHECK_MODULES(GAVL, gavl >= $GAVL_REQUIRED, , AC_MSG_ERROR("gavl not found"))
AC_SUBST(GAVL_REQUIRED)

dnl
dnl Optional Libraries
dnl

dnl
dnl gmerlin
dnl 

have_gmerlin="false"

GMERLIN_REQUIRED="0.3.1pre1"

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

AVCODEC_BUILD="4719"

dnl PKG_CHECK_MODULES(AVCODEC, avcodec_acl = $AVCODEC_REQUIRED, have_avcodec="true", have_avcodec="false")

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
        PNG_LIBS=$LIBS ;
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
CFLAGS=""
LIBS="-lfaad -lm"
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
dnl MAD
dnl

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

MAD_REQUIRED="0.15.0"
   
LIBS="-lmad"
CFLAGS=""

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
 
LIBS="-la52 -lm"
CFLAGS=""
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

AM_CONDITIONAL(HAVE_LIBA52, test x$have_mad = xtrue)

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
dnl VCD support
dnl

AC_CHECK_HEADERS([linux/cdrom.h], have_vcd="true", have_vcd="false")
AM_CONDITIONAL(HAVE_VCD, test x"$have_vcd" = "xtrue")

AC_CHECK_HEADERS([linux/hdreg.h])

dnl
dnl Common CFLAGS
dnl

CFLAGS="-Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 $GAVL_CFLAGS"

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
  true) CFLAGS="-g $CFLAGS";;
  false) CFLAGS="$CFLAGS $OPT_CFLAGS";;
esac

LIBS="$GAVL_LIBS"

if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix="${prefix}"
fi

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
plugins/Makefile \
tests/Makefile ])

AC_OUTPUT

echo "Configuration: "

echo -n "AVCodec:                "
if test "x$have_avcodec" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "faad2:                  "
if test "x$have_faad2" = "xtrue"; then
echo "Yes CFLAGS=$FAAD2_CFLAGS LIBS=$FAAD2_LIBS"
else
echo "Missing"
fi

echo -n "flac:                   "
if test "x$have_flac" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi


echo -n "vorbis:                 "
if test "x$have_vorbis" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libpng:                 "
if test "x$have_libpng" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libmpeg2:               "
if test "x$have_libmpeg2" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libmad 0.15.x:          "
if test "x$have_mad" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "liba52:                 "
if test "x$have_liba52" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

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

echo -n "VCD Support:            "
if test "x$have_vcd" = "xtrue"; then
echo "Enabled"
else
echo "Disabled (Unsupported Architecture)"
fi
