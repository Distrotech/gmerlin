
AC_INIT
AC_CONFIG_SRCDIR([include/gmerlin/player.h])

AM_CONFIG_HEADER(include/config.h)

AM_INIT_AUTOMAKE(gmerlin, 1.2.0)

BG_VERSION=$VERSION

BG_VERSION_MAJOR=`echo $VERSION | cut -d . -f 1`
BG_VERSION_MINOR=`echo $VERSION | cut -d . -f 2`
BG_VERSION_MICRO=`echo $VERSION | cut -d . -f 3 | cut -d p -f 1`

LTVERSION_CURRENT="0"
LTVERSION_REVISION="0"
LTVERSION_AGE="0"

AC_SUBST(BG_VERSION)
AC_SUBST(BG_VERSION_MAJOR)
AC_SUBST(BG_VERSION_MINOR)
AC_SUBST(BG_VERSION_MICRO)
AC_SUBST(LTVERSION_CURRENT)
AC_SUBST(LTVERSION_REVISION)
AC_SUBST(LTVERSION_AGE)

SAVE_CFLAGS=$CFLAGS

AC_DISABLE_STATIC
dnl AM_DISABLE_STATIC

AC_PROG_LIBTOOL

CFLAGS=$SAVE_CFLAGS

dnl
dnl General features
dnl

AC_CHECK_SIZEOF(long)

AC_CHECK_SIZEOF(float)
AC_CHECK_SIZEOF(double)

AC_CHECK_FUNCS(vasprintf isatty)
AC_CHECK_FUNCS(canonicalize_file_name)
AC_C99_FUNC_LRINT
AC_C99_FUNC_LRINTF

AC_C_BIGENDIAN(,,AC_MSG_ERROR("Cannot detect endianess"))

AC_CHECK_HEADERS([sys/select.h sys/sendfile.h ifaddrs.h])

AC_CHECK_DECLS([MSG_NOSIGNAL, SO_NOSIGPIPE],,,
               [#include <sys/types.h>
	       #include <sys/socket.h>])

dnl
dnl POSIX Semaphores
dnl

GMERLIN_CHECK_SEMAPHORES

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
dnl Doxygen
dnl

test_doxygen="true"
have_doxygen="false"

AC_ARG_WITH(doxygen,
    AC_HELP_STRING([--without-doxygen],
    [disable doxygen documentation generation]),
    test_doxygen="false")

if test "x$test_doxygen" = "xtrue"; then
  AC_CHECK_PROG(DOXYGEN, doxygen, "doxygen")
  if test -z "$DOXYGEN"; then
    AC_MSG_ERROR([Doxygen not found, use --without-doxygen to disable documentation generation])
  else
    have_doxygen="true"
  fi
fi

AM_CONDITIONAL(HAVE_DOXYGEN, test "x$have_doxygen" = "xtrue")

dnl
dnl Linker flags
dnl

GMERLIN_CHECK_LDFLAGS

dnl
dnl texinfo (now a build requirement)
dnl

have_texinfo="false"

AC_CHECK_PROG(TEXINFO, makeinfo, "makeinfo")
if test -z "$TEXINFO"; then
  AC_MSG_ERROR([Texinfo not found])
fi

dnl
dnl Check for Libraries
dnl

dnl Required

LT_LIB_M

XML2_REQUIRED="2.4.0"
PKG_CHECK_MODULES(XML2, libxml-2.0 >= $XML2_REQUIRED, , AC_MSG_ERROR("libxml2 not found"))
AC_SUBST(XML2_REQUIRED)

GAVL_REQUIRED="1.4.0"
PKG_CHECK_MODULES(GAVL, gavl >= $GAVL_REQUIRED, , AC_MSG_ERROR("gavl not found"))
AC_SUBST(GAVL_REQUIRED)

FONTCONFIG_REQUIRED="2.2.3"
PKG_CHECK_MODULES(FONTCONFIG, fontconfig >= $FONTCONFIG_REQUIRED, , AC_MSG_ERROR("fontconfig not found"))

FREETYPE_REQUIRED="9.7.3"
PKG_CHECK_MODULES(FREETYPE, freetype2 >= $FREETYPE_REQUIRED, , AC_MSG_ERROR("freetype2 not found"))

ICONV_LIBS=""
AC_CHECK_LIB(iconv, libiconv_close, ICONV_LIBS="-liconv")
AC_SUBST(ICONV_LIBS)

SQLITE3_LIBS=""
AC_CHECK_LIB(sqlite3, sqlite3_exec, [SQLITE3_LIBS="-lsqlite3"], AC_MSG_ERROR("sqlite not found"))
AC_SUBST(SQLITE3_LIBS)

PKG_CHECK_MODULES(UUID, uuid, have_libuuid="true", have_libuuid="false")
AM_CONDITIONAL(HAVE_UUID, test x$have_libuuid = xtrue)

MQ_LIBS=""
AC_CHECK_LIB(rt, mq_open, MQ_LIBS="-lrt")
AC_SUBST(MQ_LIBS)

AH_TEMPLATE([HAVE_MQ], [POSIX message queues])

AM_CONDITIONAL(HAVE_MQ, test "x$MQ_LIBS" != "x")
if test "x$MQ_LIBS" != "x"; then
  AC_DEFINE(HAVE_MQ)
fi

dnl
dnl Gtk
dnl

GTK_REQUIRED="2.8.0"
AM_PATH_GTK_2_0($GTK_REQUIRED, have_gtk=true, have_gtk=false)
AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xtrue)
AC_SUBST(GTK_REQUIRED)

dnl Always be in sync with the newest gtk
GTK_CFLAGS="$GTK_CFLAGS -DGTK_DISABLE_DEPRECATED"


dnl
dnl X11
dnl

have_x="false"

X_CFLAGS=""
X_LIBS=""


AC_PATH_X

if test x$no_x != xyes; then
  if test "x$x_includes" != "x"; then
    X_CFLAGS="-I$x_includes"
  elif test -d /usr/X11R6/include; then 
    X_CFLAGS="-I/usr/X11R6/include"
  else
    X_CFLAGS=""
  fi

  if test "x$x_libraries" != "x"; then
    X_LIBS="-L$x_libraries -lX11"
  else
    X_LIBS="-lX11"
  fi
  have_x="true"
else
  PKG_CHECK_MODULES(X, x11 >= 1.0.0, have_x=true, have_x=false)
fi

if test x$have_x = xtrue; then
  X_LIBS="$X_LIBS -lXext"
else
  AC_MSG_ERROR([Gmerlin needs X11 headers])
fi


AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)
AM_CONDITIONAL(HAVE_X11, test x$have_x = xtrue)

dnl
dnl OpenGL
dnl

GMERLIN_CHECK_OPENGL

dnl
dnl Pulseaudio
dnl

GMERLIN_CHECK_PULSEAUDIO

dnl
dnl Xinerama
dnl

AH_TEMPLATE([HAVE_LIBXINERAMA],
            [Do we have xinerama installed?])

have_xinerama="false"
XINERAMA_LIBS=""

if test x$have_x = xtrue; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

CFLAGS=$X_FLAGS
LIBS="$X_LIBS -lXinerama"

AC_MSG_CHECKING(for xinerama)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/Xinerama.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
               XineramaQueryExtension(NULL, NULL, NULL);
             return 0;],
            [XINERAMA_LIBS="-lXinerama";have_xinerama=true;AC_MSG_RESULT(Yes)],
	    AC_MSG_RESULT(No))

if test x$have_xinerama = "xtrue"; then
AC_DEFINE(HAVE_LIBXINERAMA)
fi

AC_SUBST(XINERAMA_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

dnl
dnl XTest
dnl

AH_TEMPLATE([HAVE_XTEST],
            [Do we have xtest installed?])

have_xtest="false"
XTEST_LIBS=""

if test x$have_x = xtrue; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
                                                                               
CFLAGS=$X_FLAGS
LIBS="$X_LIBS -lXtst"

AC_MSG_CHECKING(for xtest)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/XTest.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
               XTestQueryExtension(NULL, NULL, NULL, NULL, NULL);
             return 0;],
            [XTEST_LIBS="-lXtst";have_xtest=true;AC_MSG_RESULT(Yes)],
	    AC_MSG_RESULT(No))

if test x$have_xtest = "xtrue"; then
AC_DEFINE(HAVE_XTEST)
fi

AC_SUBST(XTEST_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

AM_CONDITIONAL(HAVE_XTEST, test x$have_xtest = xtrue)

dnl
dnl Xv
dnl

AH_TEMPLATE([HAVE_LIBXV],
            [Do we have xv installed?])

have_xv="false"
XV_LIBS=""

if test x$have_x = xtrue; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
CFLAGS=$X_FLAGS
LIBS="$X_LIBS -lXv"

AC_MSG_CHECKING(for xv)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/Xvlib.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
		 XvQueryExtension(NULL, NULL, NULL, NULL, NULL, NULL);
             return 0;],
            [XV_LIBS="-lXv";have_xv=true;AC_MSG_RESULT(Yes)],
	    AC_MSG_RESULT("No"))

if test x$have_xv = "xtrue"; then
AC_DEFINE(HAVE_LIBXV)
fi

AC_SUBST(XV_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

AM_CONDITIONAL(HAVE_XV, test x$have_xv = xtrue)

dnl
dnl X11 DPMS extension
dnl

AH_TEMPLATE([HAVE_XDPMS],
            [Do we have X11 dpms extension installed?])

have_xdpms="false"
XDPMS_LIBS=""

if test x$have_x = xtrue; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
CFLAGS=$X_FLAGS
LIBS="$X_LIBS -lXext"

AC_MSG_CHECKING(for x11 dpms)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/dpms.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
		 DPMSQueryExtension(NULL, NULL, NULL);
             return 0;],
            [XDPMS_LIBS="-lXext";have_xdpms=true;AC_MSG_RESULT(Yes)],
	    AC_MSG_RESULT("No"))

if test x$have_xdpms = "xtrue"; then
AC_DEFINE(HAVE_XDPMS)
fi

AC_SUBST(XDPMS_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

dnl
dnl XFixes
dnl

AH_TEMPLATE([HAVE_XFIXES],
            [Do we have XFixes extension installed?])

have_xfixes="false"
XFIXES_LIBS=""

if test x$have_x = xtrue; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
CFLAGS=$X_FLAGS
LIBS="$X_LIBS -lXfixes"

AC_MSG_CHECKING(for x11 dpms)
AC_TRY_LINK([#include <X11/Xlib.h>
             #include <X11/extensions/Xfixes.h>],
            [int i = 0;
             /* We ensure the function is here but never call it */
             if(i)
		 XFixesQueryExtension(NULL, NULL, NULL);
             return 0;],
            [XFIXES_LIBS="-lXfixes";have_xfixes=true;AC_MSG_RESULT(Yes)],
	    AC_MSG_RESULT("No"))

if test x$have_xfixes = "xtrue"; then
AC_DEFINE(HAVE_XFIXES)
fi

AC_SUBST(XFIXES_LIBS)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi


dnl
dnl Libquicktime
dnl

LQT_REQUIRED="1.2.4"
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
dnl libexif
dnl

AH_TEMPLATE([HAVE_LIBEXIF],
            [Do we have libexif installed?])

PKG_CHECK_MODULES(LIBEXIF, libexif, have_libexif=true, have_libexif=false)

if test x$have_libexif = "xtrue"; then
AC_DEFINE(HAVE_LIBEXIF)
fi

AM_CONDITIONAL(HAVE_LIBEXIF, test x$have_libexif = xtrue)


dnl
dnl libv4lconvert
dnl

AH_TEMPLATE([HAVE_V4LCONVERT],
            [Do we have libv4lconvert installed?])

V4LCONVERT_REQUIRED="0.5.7"
have_v4lconvert=false

AC_ARG_ENABLE(lqt,
              AC_HELP_STRING(--disable-v4lconvert, [Disable libv4lconvert support (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_v4lconvert=true ;;
                 no) test_v4lconvert=false ;;
               esac],
	       test_v4lconvert=true)

if test x$test_v4lconvert = "xtrue"; then
PKG_CHECK_MODULES(V4LCONVERT, libv4lconvert >= $V4LCONVERT_REQUIRED, have_v4lconvert=true, have_v4lconvert=false)
fi

AM_CONDITIONAL(HAVE_V4LVCONVERT, test x$have_v4lconvert = xtrue)
AC_SUBST(V4LCONVERT_REQUIRED)

if test x$have_v4lconvert = xtrue; then
AC_DEFINE(HAVE_V4LCONVERT)
fi

dnl
dnl Libvisual
dnl

AH_TEMPLATE([HAVE_LV],
            [Do we have libvisual installed?])
AH_TEMPLATE([LV_PLUGIN_DIR],
            [libvisual plugin directory])

LV_REQUIRED="0.4.0"
have_LV=false
AC_ARG_ENABLE(libvisual,
              AC_HELP_STRING(--disable-libvisual, [Disable Libvisual support (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_lv=true ;;
                 no) test_lv=false ;;
               esac],
	       test_lv=true)

if test x$test_lv = "xtrue"; then
PKG_CHECK_MODULES(LV, libvisual-0.4 >= $LV_REQUIRED, have_lv=true, have_lv=false)
fi

if test x$have_lv = "xtrue"; then
AC_DEFINE(HAVE_LV)

lv_plugin_dir=`pkg-config --variable=pluginsbasedir libvisual-0.4`
lv_plugin_dir="$lv_plugin_dir/actor"

AC_DEFINE_UNQUOTED(LV_PLUGIN_DIR, ["$lv_plugin_dir"], [libvisual plugin directory])
fi


AM_CONDITIONAL(HAVE_LV, test x$have_lv = xtrue)
AC_SUBST(LV_REQUIRED)

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
dnl Jack
dnl

have_jack="false"

AC_ARG_ENABLE(jack,
              AC_HELP_STRING(--disable-jack, [Disable Jack (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_jack=true ;;
                 no) test_jack=false ;;
               esac],
	       test_jack=true)

JACK_REQUIRED="0.109.2"

if test x$test_jack = xtrue; then
PKG_CHECK_MODULES(JACK, jack >= $JACK_REQUIRED, have_jack="true", have_jack="false")
fi

AC_SUBST(JACK_REQUIRED)

AM_CONDITIONAL(HAVE_JACK, test x$have_jack = xtrue)

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
dnl Video4linux2
dnl

have_v4l2=false
AC_ARG_ENABLE(v4l2,
              AC_HELP_STRING(--disable-v4l2, [Disable Video4Linux (default: autodetect)]),
              [case "${enableval}" in
                 yes) test_v4l2=true ;;
                 no) test_v4l2=false ;;
               esac],
	       test_v4l2=true)

if test x$test_v4l2 = xtrue; then
AC_CHECK_HEADERS(linux/videodev2.h, have_v4l2=true)
fi

AM_CONDITIONAL(HAVE_V4L2, test x$have_v4l2 = xtrue)

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
dnl inotify
dnl

GMERLIN_CHECK_INOTIFY

dnl
dnl Applications
dnl


dnl Plugin configurator

AC_ARG_ENABLE(plugincfg,
              AC_HELP_STRING(--disable-plugincfg, [Disable plugin configurator (default: enabled)]),
              [case "${enableval}" in
                 yes) have_plugincfg=true ;;
                 no) have_plugincfg=false ;;
               esac],
	       have_plugincfg=true)

if test "x$have_gtk" != "xtrue"; then
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

if test "x$have_gtk" != "xtrue" -o "x$have_alsa" = "xfalse"; then
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

if test "x$have_gtk" != "xtrue"; then
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

if test "x$have_gtk" != "xtrue"; then
have_kbd=false
fi

if test "x$have_x" != "xtrue"; then
have_kbd=false
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

dnl Recorder

AC_ARG_ENABLE(recorder,
              AC_HELP_STRING(--disable-recorder, [Disable recorder (default: enabled)]),
              [case "${enableval}" in
                 yes) have_recorder=true ;;
                 no) have_recorder=false ;;
               esac],
	       have_recorder=true)

if test "x$have_gtk" = "xfalse"; then
have_recorder=false
fi

AM_CONDITIONAL(HAVE_RECORDER, test x$have_recorder = xtrue)


dnl Visualizer

AC_ARG_ENABLE(visualizer,
              AC_HELP_STRING(--disable-visualizer, [Disable visualizer (default: enabled)]),
              [case "${enableval}" in
                 yes) have_visualizer=true ;;
                 no) have_visualizer=false ;;
               esac],
	       have_visualizer=true)

if test "x$have_gtk" = "xfalse"; then
have_visualizer=false
fi

AM_CONDITIONAL(HAVE_VISUALIZER, test x$have_visualizer = xtrue)

dnl Thumbnailer

AC_ARG_ENABLE(thumbnailer,
              AC_HELP_STRING(--disable-thumbnailer, [Disable thumbnailer (default: enabled)]),
              [case "${enableval}" in
                 yes) have_thumbnailer=true ;;
                 no) have_thumbnailer=false ;;
               esac],
	       have_thumbnailer=true)

AM_CONDITIONAL(HAVE_THUMBNAILER, test x$have_thumbnailer = xtrue)

dnl nmjedit

AC_ARG_ENABLE(nmjedit,
              AC_HELP_STRING(--enable-nmjedit, [Enable nmjedit (default: disabled)]),
              [case "${enableval}" in
                 yes) have_nmjedit=true ;;
                 no) have_nmjedit=false ;;
               esac],
	       have_nmjedit=false)

AM_CONDITIONAL(HAVE_NMJEDIT, test "x$have_nmjedit" = "xtrue")

dnl
dnl CFLAGS
dnl

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])

CFLAGS="$CFLAGS -D_REENTRANT -D_FILE_OFFSET_BITS=64 $GAVL_CFLAGS $OPT_CFLAGS"

LQT_TRY_CFLAGS(-Wall, CFLAGS="$CFLAGS -Wall", )
LQT_TRY_CFLAGS(-Wmissing-declarations, CFLAGS="$CFLAGS -Wmissing-declarations", )
LQT_TRY_CFLAGS(-Wdeclaration-after-statement, CFLAGS="$CFLAGS -Wdeclaration-after-statement")

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

AC_DEFINE_UNQUOTED(PREFIX, "$prefix", Installation prefix)

INCLUDES='-I$(top_builddir)/include'
AC_SUBST(INCLUDES)

dnl Link all modules to libgmerlin so we can use them from within plugins
MODULE_LIBADD='$(top_builddir)/lib/libgmerlin.la'
AC_SUBST(MODULE_LIBADD)

dnl Absolute src path for doxygen

TOP_SRCDIR=`cd ${srcdir} && pwd` 
AC_SUBST(TOP_SRCDIR)

dnl LDFLAGS for plugins
GMERLIN_PLUGIN_LDFLAGS='-export-symbols $(top_srcdir)/plugin.sym'
AC_SUBST(GMERLIN_PLUGIN_LDFLAGS)

dnl Configuration Subdirectiories

AC_CONFIG_FILES([Makefile  po/Makefile.in \
gmerlin.spec \
gmerlin.pc \
gmerlin-gtk.pc \
apps/Makefile \
apps/gavftools/Makefile \
apps/plugincfg/Makefile \
apps/player/Makefile \
apps/alsamixer/Makefile \
apps/cmdlineplayer/Makefile \
apps/cmdlinerecorder/Makefile \
apps/recorder/Makefile \
apps/transcoder/Makefile \
apps/visualizer/Makefile \
apps/kbd/Makefile \
apps/thumbnailer/Makefile \
apps/nmjedit/Makefile \
apps/dbedit/Makefile \
apps/mediaserver/Makefile \
apps/mediaserver/web/Makefile \
apps/mediaserver/web/js/Makefile \
apps/mediaserver/web/icons/Makefile \
apps/mediaserver/web/css/Makefile \
doc/Makefile \
doc/Doxyfile \
icons/Makefile \
m4/Makefile \
utils/Makefile \
tests/Makefile \
include/Makefile \
include/gmerlin/Makefile \
include/gmerlin/bg_version.h \
include/gmerlin/upnp/Makefile
include/gui_gtk/Makefile \
include/upnp/Makefile \
include/x11/Makefile \
lib/Makefile \
lib/gtk/Makefile \
lib/upnp/Makefile \
lib/x11/Makefile \
osd/Makefile \
plugins/Makefile \
plugins/alsa/Makefile \
plugins/audiofilters/Makefile \
plugins/x11/Makefile \
plugins/cdaudio/Makefile \
plugins/edl/Makefile \
plugins/esd/Makefile \
plugins/gavl/Makefile \
plugins/jack/Makefile \
plugins/lqt/Makefile \
plugins/oss/Makefile \
plugins/jpeg/Makefile \
plugins/mikmod/Makefile \
plugins/png/Makefile \
plugins/postprocessors/Makefile \
plugins/pulseaudio/Makefile \
plugins/subwriters/Makefile \
plugins/image/Makefile \
plugins/tiff/Makefile \
plugins/v4l2/Makefile \
plugins/videofilters/Makefile \
plugins/visualization/Makefile \
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

echo -n "Jack:                "
if test "x$have_jack" = "xtrue"; then
echo "Yes (CFLAGS=$JACK_CFLAGS LIBS=$JACK_LIBS)"
else
if test "x$test_jack" = "xtrue"; then
echo "Missing (Go to http://jackit.sourceforge.net/)"
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

echo -n "Libvisual:           "
if test "x$have_lv" = "xtrue"; then
echo "Yes (CFLAGS=$LV_CFLAGS LIBS=$LV_LIBS)"
else
if test "x$test_lv" = "xtrue"; then
echo "Missing (Go to http://libvisual.sourceforge.net/)"
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
echo "Missing (Go to http://www.remotesensing.org/libtiff/)"
else
echo "Disabled"
fi
fi

echo -n "pulseaudio:          "
if test "x$have_pulseaudio" = "xtrue"; then
echo "Yes (CFLAGS=$PULSEAUDIO_CFLAGS LIBS=$PULSEAUDIO_LIBS)"
else
if test "x$test_pulseaudio" = "xtrue"; then
echo "Missing (Go to http://www.pulseaudio.org)"
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

echo -n "Video4Linux2:        "
if test "x$have_v4l2" = "xtrue"; then
echo "Yes"
else
if test "x$test_v4l2" = "xtrue"; then
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

echo -n "OpenGL:              "
if test "x$have_GL" = "xtrue"; then
echo "Yes (CFLAGS=$GL_CFLAGS, LIBS=$GL_LIBS)"
else
echo "Missing"
fi

echo -n "GLX:                 "
if test "x$have_GLX" = "xtrue"; then
echo "Yes (CFLAGS=$GLX_CFLAGS, LIBS=$GLX_LIBS)"
else
echo "Missing"
fi

echo -n "Xinerama:            "
if test "x$have_xinerama" = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "Xtest:               "
if test "x$have_xtest" = "xtrue"; then
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

echo -n "XFixes extension:    "
if test "x$have_xfixes" = "xtrue"; then
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

echo -n "libv4lconvert:       "
if test "x$have_v4lconvert" = "xtrue"; then
echo "Yes (CFLAGS=$V4LCONVERT_CFLAGS LIBS=$V4LCONVERT_LIBS)"
else
if test "x$test_v4lconvert" = "xtrue"; then
echo "Missing (Go to http://people.atrpms.net/~hdegoede/)"
else
echo "Disabled"
fi
fi

echo -n "libexif:             "
if test "x$have_libexif" = "xtrue"; then
echo "Yes (CFLAGS=$LIBEXIF_CFLAGS LIBS=$LIBEXIF_LIBS)"
else
echo "Missing (Go to http://libexif.sourceforge.net/)"
fi


echo
echo "=== Applications: ================================="

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

echo -n "Recorder:            "
if test "x$have_recorder" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "Visualizer:          "
if test "x$have_visualizer" = "xtrue"; then
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

echo -n "Thumbnailer:         "
if test "x$have_thumbnailer" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo -n "nmjedit:             "
if test "x$have_nmjedit" = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi

echo
echo "If you installed a library but it was not detected, check the file INSTALL"
echo "for troubleshooting tips. Also note that if the configure script reaches"
echo "this point, all missing packages are optional so compilation should succeed"
echo "anyway."
