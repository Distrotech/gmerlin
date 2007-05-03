
AC_INIT
AC_CONFIG_SRCDIR([include/player.h])

AM_CONFIG_HEADER(include/config.h)

AM_INIT_AUTOMAKE(gmerlin, 0.3.6)

BG_VERSION=$VERSION

BG_VERSION_MAJOR=`echo $VERSION | cut -d . -f 1`
BG_VERSION_MINOR=`echo $VERSION | cut -d . -f 2`
BG_VERSION_MICRO=`echo $VERSION | cut -d . -f 3 | cut -d p -f 1`
AC_SUBST(BG_VERSION)
AC_SUBST(BG_VERSION_MAJOR)
AC_SUBST(BG_VERSION_MINOR)
AC_SUBST(BG_VERSION_MICRO)


AC_DISABLE_STATIC
dnl AM_DISABLE_STATIC

AC_PROG_LIBTOOL

dnl
dnl General features
dnl

AC_CHECK_SIZEOF(long)
AC_C_BIGENDIAN
AC_CHECK_FUNCS(vasprintf)

AC_CHECK_DECLS([MSG_NOSIGNAL, SO_NOSIGPIPE],,,
               [#include <sys/types.h>
	       #include <sys/socket.h>])

dnl
dnl Gettext
dnl

AM_GNU_GETTEXT([external])


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
  AC_MSG_CHECKING(for --rpath option)
  OLD_LDFLAGS=$LDFLAGS
  LDFLAGS="$LDFLAGS -Wl,--rpath -Wl,$GMERLIN_DEP_DIR/lib"  
  AC_TRY_LINK([],[],
              [GMERLIN_DEP_RPATH="-Wl,--rpath -Wl,$GMERLIN_DEP_DIR/lib"; AC_MSG_RESULT(Supported)],
	      [GMERLIN_DEP_RPATH="";AC_MSG_RESULT(Unsupported)])
  LDFLAGS=$OLD_LDFLAGS
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
dnl Check for Libraries
dnl

dnl Required

XML2_REQUIRED="2.4.0"
PKG_CHECK_MODULES(XML2, libxml-2.0 >= $XML2_REQUIRED, , AC_MSG_ERROR("libxml2 not found"))
AC_SUBST(XML2_REQUIRED)

GAVL_REQUIRED="0.2.6"
PKG_CHECK_MODULES(GAVL, gavl = $GAVL_REQUIRED, , AC_MSG_ERROR("gavl not found"))
AC_SUBST(GAVL_REQUIRED)

FONTCONFIG_REQUIRED="2.2.3"
PKG_CHECK_MODULES(FONTCONFIG, fontconfig >= $FONTCONFIG_REQUIRED, , AC_MSG_ERROR("fontconfig not found"))

FREETYPE_REQUIRED="9.7.3"
PKG_CHECK_MODULES(FREETYPE, freetype2 >= $FREETYPE_REQUIRED, , AC_MSG_ERROR("freetype2 not found"))

ICONV_LIBS=
AC_CHECK_LIB(iconv, libiconv_close, ICONV_LIBS="-liconv")
AC_SUBST(ICONV_LIBS)

dnl
dnl Gtk
dnl

GTK_REQUIRED="2.2.0"
AM_PATH_GTK_2_0($GTK_REQUIRED, have_gtk=true, have_gtk=false)
AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xtrue)
AC_SUBST(GTK_REQUIRED)

dnl Always be in sync with the newest gtk
GTK_CFLAGS="$GTK_CFLAGS -DGTK_DISABLE_DEPRECATED"

dnl
dnl X11
dnl

have_x="false"

X_CLAGS=""
X_LIBS=""


AC_PATH_X

if test x$no_x != xyes; then
  if test "x$x_includes" != "x"; then
    X_FLAGS="-I$x_includes"
  else
    X_FLAGS=""
  fi

  if test "x$x_libraries" != "x"; then
    X_LIBS="-L$x_libraries -lX11 -lXext"
  else
    X_LIBS="-lX11 -lXext"
  fi
have_x="true"
fi

AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)
AM_CONDITIONAL(HAVE_X11, test x$no_x != xyes)

dnl
dnl Xinerama
dnl

AH_TEMPLATE([HAVE_LIBXINERAMA],
            [Do we have xinerama installed?])

have_xinerama="false"
XINERAMA_LIBS=""

if test x$no_x != xyes; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
OLD_LDFLAGS=$LDFLAGS
                                                                               
CFLAGS=$X_FLAGS
LDFLAGS="$X_LIBS $LDFLAGS -lXinerama"
LIBS=$X_LIBS

AC_MSG_CHECKING(for xinerama)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/Xinerama.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
               XineramaQueryExtension(NULL, NULL, NULL);
             return 0;],
            [XINERAMA_LIBS="-lXinerama";have_xinerama=true;AC_MSG_RESULT("Yes")],
	    AC_MSG_RESULT("No"))

if test x$have_xinerama = "xtrue"; then
AC_DEFINE(HAVE_LIBXINERAMA)
fi

AC_SUBST(XINERAMA_LIBS)

CFLAGS=$OLD_CFLAGS
LDFLAGS=$OLD_LDFLAGS
LIBS=$OLD_LIBS

fi

dnl
dnl Xv
dnl

AH_TEMPLATE([HAVE_LIBXV],
            [Do we have xv installed?])

have_xv="false"
XV_LIBS=""

if test x$no_x != xyes; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
OLD_LDFLAGS=$LDFLAGS                                                                               
CFLAGS=$X_FLAGS
LIBS=$X_LIBS
LDFLAGS="$X_LIBS $LDFLAGS -lXv"

AC_MSG_CHECKING(for xv)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/Xvlib.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
		 XvQueryExtension(NULL, NULL, NULL, NULL, NULL, NULL);
             return 0;],
            [XV_LIBS="-lXv";have_xv=true;AC_MSG_RESULT("Yes")],
	    AC_MSG_RESULT("No"))

if test x$have_xv = "xtrue"; then
AC_DEFINE(HAVE_LIBXV)
fi

AC_SUBST(XV_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS
LDFLAGS=$OLD_LDFLAGS

fi

dnl
dnl X11 DPMS extension
dnl

AH_TEMPLATE([HAVE_XDPMS],
            [Do we have X11 dpms extension installed?])

have_xdpms="false"
XDPMS_LIBS=""

if test x$no_x != xyes; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
OLD_LDFLAGS=$LDFLAGS                                                                               
CFLAGS=$X_FLAGS
LIBS=$X_LIBS
LDFLAGS="$X_LIBS $LDFLAGS -lXext"

AC_MSG_CHECKING(for x11 dpms)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/dpms.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
		 DPMSQueryExtension(NULL, NULL, NULL);
             return 0;],
            [XDPMS_LIBS="-lXext";have_xdpms=true;AC_MSG_RESULT("Yes")],
	    AC_MSG_RESULT("No"))

if test x$have_xdpms = "xtrue"; then
AC_DEFINE(HAVE_XDPMS)
fi

AC_SUBST(XDPMS_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS
LDFLAGS=$OLD_LDFLAGS

fi

dnl
dnl Libquicktime
dnl

LQT_REQUIRED="1.0.0"
have_lqt=false
AC_ARG_ENABLE(lqt,
              AC_HELP_STRING(--disable-lqt, [Disable Libquicktime support (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_lqt=true ;;
                 no) test_lqt=false ;;
               esac],
	       test_lqt=true)

if test x$test_lqt = "xtrue"; then
PKG_CHECK_MODULES(LQT, libquicktime >= $LQT_REQUIRED, have_lqt=true, have_lqt=false)
fi

dnl AC_PATH_LQT($LQT_REQUIRED,  have_lqt=true, have_lqt=false)

AM_CONDITIONAL(HAVE_LQT, test x$have_lqt = xtrue)
AC_SUBST(LQT_REQUIRED)

dnl
dnl CDrom support
dnl

have_cdio="false"

CDIO_REQUIRED="0.76"

AC_ARG_ENABLE(libcdio,
[AC_HELP_STRING([--disable-libcdio],[Disable libcdio (default: autodetect)])],
[case "${enableval}" in
   yes) test_cdio=true ;;
   no)  test_cdio=false ;;
esac],[test_cdio=true])

if test x$test_cdio = xtrue; then
PKG_CHECK_MODULES(CDIO, libcdio >= $CDIO_REQUIRED, have_cdio="true", have_cdio="false")

if test x$have_cdio = xtrue; then
PKG_CHECK_MODULES(CDIO_CDDA, libcdio_cdda >= $CDIO_REQUIRED, have_cdio="true", have_cdio="false")
fi

if test x$have_cdio = xtrue; then
PKG_CHECK_MODULES(CDIO_PARANOIA, libcdio_paranoia >= $CDIO_REQUIRED, have_cdio="true", have_cdio="false")
fi

fi

AM_CONDITIONAL(HAVE_CDIO, test x$have_cdio = xtrue)
AC_SUBST(CDIO_REQUIRED)

dnl
dnl Musicbrainz
dnl

AH_TEMPLATE([HAVE_MUSICBRAINZ],
            [Do we have libmusicbrainz installed?])

MUSICBRAINZ_REQUIRED="2.0.2"
have_musicbrainz=false
AC_ARG_ENABLE(musicbrainz,
              AC_HELP_STRING(--disable-musicbrainz, [Disable musicbrainz support (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_musicbrainz=true ;;
                 no) test_musicbrainz=false ;;
               esac],
	       test_musicbrainz=true)

dnl We check for musicbrainz only if we already detected CDROM support
if test x$have_cdio = "xfalse"; then
test_musicbrainz=false
fi

if test x$test_musicbrainz = "xtrue"; then
PKG_CHECK_MODULES(MUSICBRAINZ, libmusicbrainz >= $MUSICBRAINZ_REQUIRED, have_musicbrainz=true, have_musicbrainz=false)
fi

if test x$have_musicbrainz = "xtrue"; then
AC_DEFINE(HAVE_MUSICBRAINZ)
fi

AM_CONDITIONAL(HAVE_MUSICBRAINZ, test x$have_musicbrainz = xtrue)
AC_SUBST(MUSICBRAINZ_REQUIRED)

dnl
dnl Libcddb
dnl

AH_TEMPLATE([HAVE_LIBCDDB],
            [Do we have libcddb installed?])

dnl LIBCDDB_REQUIRED="1.2.1"
LIBCDDB_REQUIRED="1.0.2"
have_cddb=false
AC_ARG_ENABLE(cddb,
              AC_HELP_STRING(--disable-cddb, [Disable cddb support (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_cddb=true ;;
                 no) test_cddb=false ;;
               esac],
	       test_cddb=true)

dnl We check for cddb only of we already detected CDROM support
if test x$have_cdio = "xfalse"; then
test_cddb=false
fi

if test x$test_cddb = "xtrue"; then
PKG_CHECK_MODULES(CDDB, libcddb >= $LIBCDDB_REQUIRED, have_cddb=true, have_cddb=false)
fi

if test x$have_cddb = "xtrue"; then
AC_DEFINE(HAVE_LIBCDDB)
fi

AM_CONDITIONAL(HAVE_LIBCDDB, test x$have_cddb = xtrue)
AC_SUBST(LIBCDDB_REQUIRED)


dnl
dnl ESounD
dnl

ESD_REQUIRED="0.2.19"
have_esd=false
AC_ARG_ENABLE(esd,
              AC_HELP_STRING(--disable-esd, [Disable EsounD (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_esd=true ;;
                 no) test_esd=false ;;
               esac],
	       test_esd=true)

if test x$test_esd = xtrue; then
AM_PATH_ESD($ESD_REQUIRED, have_esd=true, have_esd=false)
fi

AM_CONDITIONAL(HAVE_ESD,           test x$have_esd = xtrue)
AC_SUBST(ESD_REQUIRED)

dnl
dnl ALSA
dnl

ALSA_REQUIRED="1.0.0"
have_alsa="false"

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

AC_ARG_ENABLE(alsa,
              AC_HELP_STRING(--disable-alsa, [Disable Alsa (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_alsa=true ;;
                 no) test_alsa=false ;;
               esac],
	       test_alsa=true)
		 
if test x$test_alsa = xtrue; then
  AM_PATH_ALSA(1.0.0, have_alsa="true", have_alsa="false")
fi

dnl CFLAGS and LIBS must be cleared after AM_PATH_ALSA
CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS


AM_CONDITIONAL(HAVE_ALSA, test x$have_alsa = xtrue)
AC_SUBST(ALSA_REQUIRED)

dnl
dnl OSS
dnl

have_oss=false
AC_ARG_ENABLE(oss,
              AC_HELP_STRING(--disable-oss, [Disable OSS (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_oss=true ;;
                 no) test_oss=false ;;
               esac],
	       test_oss=true)

if test x$test_oss = xtrue; then
AC_CHECK_HEADERS(sys/soundcard.h soundcard.h, have_oss=true)
fi
AM_CONDITIONAL(HAVE_OSS, test x$have_oss = xtrue)

dnl
dnl Video4linux
dnl

have_v4l=false
AC_ARG_ENABLE(v4l,
              AC_HELP_STRING(--disable-v4l, [Disable Video4Linux (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_v4l=true ;;
                 no) test_v4l=false ;;
               esac],
	       test_v4l=true)

if test x$test_v4l = xtrue; then
AC_CHECK_HEADERS(linux/videodev.h, have_v4l=true)
fi

AM_CONDITIONAL(HAVE_V4L, test x$have_v4l = xtrue)

dnl
dnl libjpeg
dnl

GMERLIN_CHECK_LIBJPEG

dnl
dnl png
dnl

GMERLIN_CHECK_LIBPNG

dnl
dnl libtiff
dnl 

GMERLIN_CHECK_LIBTIFF

dnl
dnl Applications
dnl

dnl Camelot

AC_ARG_ENABLE(camelot,
              AC_HELP_STRING(--disable-camelot, [Disable Camelot (default: enabled)]),
              [case "${enableval}" in
                 yes) have_camelot=true ;;
                 no) have_camelot=false ;;
               esac],
	       have_camelot=true)

if test "x$have_gtk" = "xfalse" -o "x$have_v4l" = "xfalse"; then
have_camelot=false
fi
AM_CONDITIONAL(HAVE_CAMELOT, test x$have_camelot = xtrue)

dnl Plugin configurator

AC_ARG_ENABLE(plugincfg,
              AC_HELP_STRING(--disable-plugincfg, [Disable plugin configurator (default: enabled)]),
              [case "${enableval}" in
                 yes) have_plugincfg=true ;;
                 no) have_plugincfg=false ;;
               esac],
	       have_plugincfg=true)

if test "x$have_gtk" = "xfalse"; then
have_plugincfg=false
fi

AM_CONDITIONAL(HAVE_PLUGINCFG, test x$have_plugincfg = xtrue)

dnl Alsamixer

AC_ARG_ENABLE(alsamixer,
              AC_HELP_STRING(--disable-alsamixer, [Disable alsamixer (default: enabled)]),
              [case "${enableval}" in
                 yes) have_alsamixer=true ;;
                 no) have_alsamixer=false ;;
               esac],
	       have_alsamixer=true)

if test "x$have_gtk" = "xfalse" -o "x$have_alsa" = "xfalse"; then
have_alsamixer=false
fi

AM_CONDITIONAL(HAVE_ALSAMIXER, test x$have_alsamixer = xtrue)

dnl Player

AC_ARG_ENABLE(player,
              AC_HELP_STRING(--disable-player, [Disable player (default: enabled)]),
              [case "${enableval}" in
                 yes) have_player=true ;;
                 no) have_player=false ;;
               esac],
	       have_player=true)

if test "x$have_gtk" = "xfalse"; then
have_player=false
fi

AM_CONDITIONAL(HAVE_PLAYER, test x$have_player = xtrue)

dnl kbd

AC_ARG_ENABLE(kbd,
              AC_HELP_STRING(--disable-kbd, [Disable keyboard daemon (default: enabled)]),
              [case "${enableval}" in
                 yes) have_kbd=true ;;
                 no) have_kbd=false ;;
               esac],
	       have_kbd=true)

if test "x$have_gtk" = "xfalse"; then
have_kdb=false
fi

AM_CONDITIONAL(HAVE_KBD, test x$have_kbd = xtrue)



dnl Transcoder

AC_ARG_ENABLE(transcoder,
              AC_HELP_STRING(--disable-transcoder, [Disable transcoder (default: enabled)]),
              [case "${enableval}" in
                 yes) have_transcoder=true ;;
                 no) have_transcoder=false ;;
               esac],
	       have_transcoder=true)

if test "x$have_gtk" = "xfalse"; then
have_transcoder=false
fi

AM_CONDITIONAL(HAVE_TRANSCODER, test x$have_transcoder = xtrue)

dnl
dnl CFLAGS
dnl

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])

CFLAGS="$CFLAGS -D_REENTRANT -D_FILE_OFFSET_BITS=64 $GAVL_CFLAGS $OPT_CFLAGS"

LQT_TRY_CFLAGS("-Wall", CFLAGS="$CFLAGS -Wall", )
LQT_TRY_CFLAGS("-Wmissing-declarations", CFLAGS="$CFLAGS -Wmissing-declarations", )
LQT_TRY_CFLAGS("-Wdeclaration-after-statement", CFLAGS="$CFLAGS -Wdeclaration-after-statement")


dnl
dnl Check for SIMD
dnl

GAVL_CHECK_SIMD($host_cpu, "$OPT_CFLAGS")

dnl
dnl libxml is necessary for all sources
dnl

CFLAGS="$CFLAGS $XML2_CFLAGS"

LIBS="$GAVL_LIBS"

dnl Output variables

if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix="${prefix}"
fi

GMERLIN_PLUGIN_DIR="$exec_prefix/lib/$PACKAGE/plugins"
GMERLIN_DATA_DIR="$prefix/share/$PACKAGE"

AC_DEFINE_UNQUOTED(GMERLIN_DATA_DIR, "$GMERLIN_DATA_DIR", Directory for other data)
AC_DEFINE_UNQUOTED(GMERLIN_PLUGIN_DIR, "$GMERLIN_PLUGIN_DIR", Directory for plugins)

AC_SUBST(GMERLIN_DATA_DIR)
AC_SUBST(GMERLIN_PLUGIN_DIR)

INCLUDES='-I$(top_builddir)/include'
AC_SUBST(INCLUDES)

dnl Configuration Subdirectiories

AC_CONFIG_FILES([Makefile  po/Makefile.in \
gmerlin.spec \
gmerlin.pc \
gmerlin-gtk.pc \
apps/Makefile \
apps/plugincfg/Makefile \
apps/plugincfg/gmerlin-plugincfg.desktop \
apps/player/Makefile \
apps/player/gmerlin-player.desktop \
apps/alsamixer/Makefile \
apps/alsamixer/gmerlin-alsamixer.desktop \
apps/camelot/Makefile \
apps/camelot/gmerlin-camelot.desktop \
apps/transcoder/Makefile \
apps/transcoder/gmerlin-transcoder.desktop \
apps/kbd/Makefile \
apps/kbd/gmerlin-kbd.desktop \
doc/Makefile \
icons/Makefile \
m4/Makefile \
tests/Makefile \
include/Makefile \
include/bg_version.h \
include/gui_gtk/Makefile \
lib/Makefile \
lib/gtk/Makefile \
osd/Makefile \
plugins/Makefile \
plugins/alsa/Makefile \
plugins/audiofilters/Makefile \
plugins/x11/Makefile \
plugins/cdaudio/Makefile \
plugins/esd/Makefile \
plugins/lqt/Makefile \
plugins/oss/Makefile \
plugins/jpeg/Makefile \
plugins/mikmod/Makefile \
plugins/png/Makefile \
plugins/postprocessors/Makefile \
plugins/subwriters/Makefile \
plugins/image/Makefile \
plugins/tiff/Makefile \
plugins/v4l/Makefile \
plugins/videofilters/Makefile \
plugins/wavwrite/Makefile \
skins/Makefile \
skins/Default/Makefile])

AC_OUTPUT

echo "=== Libraries: ===================================="

echo -n "Gtk-2.0:             "
if test "x$have_gtk" = "xtrue"; then
echo "Yes"
else
echo "Missing (GUI applications won't be compiled! Go to http://www.gtk.org/)"
fi

echo
echo "=== Plugins: ======================================"

echo -n "EsounD:              "
if test "x$have_esd" = "xtrue"; then
echo "Yes (CFLAGS=$ESD_CFLAGS LIBS=$ESD_LIBS)"
else
if test "x$test_esd" = "xtrue"; then
echo "Missing (Go to http://www.tux.org/~ricdude/EsounD.html)"
else
echo "Disabled"
fi
fi

echo -n "Alsa:                "
if test "x$have_alsa" = "xtrue"; then
echo "Yes (CFLAGS=$ALSA_CFLAGS LIBS=$ALSA_LIBS)"
else
if test "x$test_alsa" = "xtrue"; then
echo "Missing (Go to http://www.alsa-project.org/)"
else
echo "Disabled"
fi
fi

echo -n "Libquicktime:        "
if test "x$have_lqt" = "xtrue"; then
echo "Yes (CFLAGS=$LQT_CFLAGS LIBS=$LQT_LIBS)"
else
if test "x$test_lqt" = "xtrue"; then
echo "Missing (Go to http://libquicktime.sourceforge.net/)"
else
echo "Disabled"
fi
fi

echo -n "Libjpeg:             "
if test "x$have_libjpeg" = "xtrue"; then
echo "Yes (CFLAGS=$JPEG_CFLAGS LIBS=$JPEG_LIBS)"
else
if test "x$test_libjpeg" = "xtrue"; then
echo "Missing (Go to http://www.ijg.org/)"
else
echo "Disabled"
fi
fi

echo -n "Libpng:              "
if test "x$have_libpng" = "xtrue"; then
echo "Yes (CFLAGS=$PNG_CFLAGS LIBS=$PNG_LIBS)"
else
if test "x$test_libpng" = "xtrue"; then
echo "Missing (Go to http://www.libpng.org/pub/png/libpng.html)"
else
echo "Disabled"
fi
fi

echo -n "Libtiff:             "
if test "x$have_libtiff" = "xtrue"; then
echo "Yes (CFLAGS=$TIFF_CFLAGS LIBS=$TIFF_LIBS)"
else
if test "x$test_libtiff" = "xtrue"; then
echo "Missing (http://www.remotesensing.org/libtiff/)"
else
echo "Disabled"
fi
fi

echo -n "OSS:                 "
if test "x$have_oss" = "xtrue"; then
echo "Yes"
else
if test "x$test_oss" = "xtrue"; then
echo "Missing (Probably unsupported architecture)"
else
echo "Disabled"
fi
fi

echo -n "Video4Linux:         "
if test "x$have_v4l" = "xtrue"; then
echo "Yes"
else
if test "x$test_v4l" = "xtrue"; then
echo "Missing (Probably unsupported architecture)"
else
echo "Disabled"
fi
fi

echo -n "X11:                 "
if test "x$have_x" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "Xinerama:            "
if test "x$have_xinerama" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "Xvideo extension:    "
if test "x$have_xv" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "X11 dpms extension:  "
if test "x$have_xdpms" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libcdio:             "
if test "x$have_cdio" = "xtrue"; then
echo "Yes (CFLAGS=$CDIO_CFLAGS $CDIO_CDDA_CFLAGS $CDIO_PARANOIA_CFLAGS LIBS=$CDIO_LIBS $CDIO_CDDA_LIBS $CDIO_PARANOIA_LIBS)"
else
if test "x$test_cdio" = "xtrue"; then
echo "Missing (Go to http://www.gnu.org/software/libcdio/)"
else
echo "Disabled"
fi
fi

echo -n "Musicbrainz:         "
if test "x$have_musicbrainz" = "xtrue"; then
echo "Yes (CFLAGS=$MUSICBRAINZ_CFLAGS LIBS=$MUSICBRAINZ_LIBS)"
else
if test "x$test_musicbrainz" = "xtrue"; then
echo "Missing (Go to http://musicbrainz.org/products/client/index.html)"
else
echo "Disabled"
fi
fi

echo -n "libcddb:             "
if test "x$have_cddb" = "xtrue"; then
echo "Yes (CFLAGS=$CDDB_CFLAGS LIBS=$CDDB_LIBS)"
else
if test "x$test_cddb" = "xtrue"; then
echo "Missing (Go to http://libcddb.sourceforge.net)"
else
echo "Disabled"
fi
fi

echo
echo "=== Applications: ================================="

echo -n "Camelot:             "
if test "x$have_camelot" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "Plugin Configurator: "
if test "x$have_plugincfg" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "Alsamixer:           "
if test "x$have_alsamixer" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "Player:              "
if test "x$have_player" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "Transcoder:          "
if test "x$have_transcoder" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "Keyboard daemon:     "
if test "x$have_kbd" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo
echo "If you installed a library but it was not detected, check the file INSTALL"
echo "for troubleshooting tips. Also note that if the configure script reaches"
echo "this point, all missing packages are optional so compilation should succeed"
echo "anyway."
