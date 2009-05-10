AC_INIT(gmerlin-mozilla, 1.0.0pre1)
AC_CONFIG_SRCDIR([plugin/plugin.c])
AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(1.8.5)
AC_PROG_LIBTOOL



dnl
dnl Check for Libraries
dnl

GMERLIN_REQUIRED="0.4.0"

PKG_CHECK_MODULES(GMERLIN, gmerlin >= $GMERLIN_REQUIRED, , AC_MSG_ERROR("gmerlin not found"))
AC_SUBST(GMERLIN_REQUIRED)

GMERLIN_GTK_REQUIRED="0.4.0"

PKG_CHECK_MODULES(GMERLIN_GTK, gmerlin-gtk >= $GMERLIN_GTK_REQUIRED, , AC_MSG_ERROR("gmerlin not found"))
AC_SUBST(GMERLIN_GTK_REQUIRED)

GMERLIN_CHECK_INOTIFY

if test "x$have_inotify" != "xtrue"; then
  AC_MSG_ERROR(No inotify found, need recent linux kernel)
fi

dnl
dnl Check for Mozilla
dnl

AC_MSG_NOTICE([Determining mozilla/firefox packages to build against])
PKG_CHECK_MODULES(MOZPLUG,[mozilla-plugin],[AC_SUBST(MOZPLUG_CFLAGS) MOZPLUG_CFLAGS="${MOZPLUG_CFLAGS} -I`$PKG_CONFIG --variable=includedir mozilla-plugin`"
MOZILLA_HOME=`$PKG_CONFIG --variable=libdir mozilla-plugin`
],[AC_MSG_WARN([mozilla-plugin not found])])

if test "$MOZPLUG_CFLAGS" = ""; then
    PKG_CHECK_MODULES(MOZPLUG,[firefox-plugin],[AC_SUBST(MOZPLUG_CFLAGS) MOZPLUG_CFLAGS="${MOZPLUG_CFLAGS} -I`$PKG_CONFIG --variable=includedir firefox-plugin`"
    MOZILLA_HOME=`$PKG_CONFIG --variable=libdir firefox-plugin`
],[AC_MSG_WARN([firefox-plugin not found])])
fi
if test "$MOZPLUG_CFLAGS" = ""; then
    PKG_CHECK_MODULES(MOZPLUG,[seamonkey-plugin],[AC_SUBST(MOZPLUG_CFLAGS) MOZPLUG_CFLAGS="${MOZPLUG_CFLAGS} -I`$PKG_CONFIG --variable=includedir seamonkey-plugin`"
	    MOZILLA_HOME=`$PKG_CONFIG --variable=libdir seamonkey-plugin`
],[AC_MSG_WARN([seamonkey-plugin not found])])
fi
if test "$MOZPLUG_CFLAGS" = ""; then
    PKG_CHECK_MODULES(MOZPLUG,[xulrunner-plugin],[AC_SUBST(MOZPLUG_CFLAGS) MOZPLUG_CFLAGS="${MOZPLUG_CFLAGS} -I`$PKG_CONFIG --variable=includedir xulrunner-plugin`"
	    MOZILLA_HOME=`$PKG_CONFIG --variable=libdir xulrunner-plugin`
	    ],[AC_MSG_WARN([xulrunner-plugin not found])])
fi
if test "$MOZPLUG_CFLAGS" = ""; then
    PKG_CHECK_MODULES(MOZPLUG,[libxul],[AC_SUBST(MOZPLUG_CFLAGS) MOZPLUG_CFLAGS="${MOZPLUG_CFLAGS} -I`$PKG_CONFIG --variable=includedir libxul`"
	    MOZILLA_HOME=`$PKG_CONFIG --variable=libdir libxul`
	    ],[AC_MSG_WARN([libxul not found])])
fi
if test "$MOZPLUG_CFLAGS" = ""; then
    PKG_CHECK_MODULES(MOZPLUG,[iceape-plugin iceape-xpcom],[AC_SUBST(MOZPLUG_CFLAGS) MOZPLUG_CFLAGS="${MOZPLUG_CFLAGS} -I`$PKG_CONFIG --variable=includedir iceape-plugin`"
	    MOZILLA_HOME=`$PKG_CONFIG --variable=libdir iceape-plugin`
	    ],[AC_MSG_WARN([iceape-plugin not found])])
fi

if test "$MOZPLUG_CFLAGS" = ""; then
    AC_MSG_ERROR([Unable to find mozilla or firefox development files])
fi

AC_SUBST(MOZILLA_HOME)

AC_SUBST(MOZPLUG_CFLAGS)


CFLAGS="-g -Wmissing-declarations -Wall"

dnl
dnl 
dnl

if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi

PKGLIBDIR="$prefix/lib/$PACKAGE"
AC_SUBST(PKGLIBDIR)

GMERLIN_MOZILLA_DATA_DIR="$prefix/share/$PACKAGE"
AC_DEFINE_UNQUOTED(GMERLIN_MOZILLA_DATA_DIR, "$GMERLIN_MOZILLA_DATA_DIR", Directory for other data)
AC_SUBST(GMERLIN_MOZILLA_DATA_DIR)

dnl
dnl Output
dnl 

AC_CONFIG_FILES([Makefile 
gmerlin-mozilla-enable \
gmerlin-mozilla-disable \
include/Makefile \
plugin/Makefile \
skins/Makefile \
skins/Default/Makefile \
m4/Makefile ])

AC_OUTPUT
