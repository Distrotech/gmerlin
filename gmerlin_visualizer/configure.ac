AC_INIT(src/mainwindow.c)

AM_CONFIG_HEADER(src/config.h)

AM_INIT_AUTOMAKE(gmerlin-visualizer, 1.0.3pre1)
AC_PROG_LIBTOOL

dnl
dnl Required stuff
dnl

dnl gtk-1.2.x

GTK_REQUIRED="1.2.8"
AM_PATH_GTK($GTK_REQUIRED, , AC_MSG_ERROR(gmerlin_visualizer needs gtk with headers, 1))

AC_SUBST(GTK_REQUIRED)
AC_SUBST(GTK_CFLAGS, $GTK_CFLAGS)
AC_SUBST(GTK_LIBS, $GTK_LIBS)

dnl gmerlin

GMERLIN_REQUIRED="0.3.3pre1"

PKG_CHECK_MODULES(GMERLIN, gmerlin = $GMERLIN_REQUIRED, ,
                  AC_MSG_ERROR(gmerlin_visualizer needs gmerlin with headers))

GMERLIN_DATA_DIR=`pkg-config --variable=prefix gmerlin`
GMERLIN_DATA_DIR="$GMERLIN_DATA_DIR/share/gmerlin"
echo "Gmerlin data dir: $GMERLIN_DATA_DIR"

AC_SUBST(GMERLIN_REQUIRED)
AC_SUBST(GMERLIN_CFLAGS, $GMERLIN_CFLAGS)
AC_SUBST(GMERLIN_LIBS, $GMERLIN_LIBS)
AC_SUBST(GMERLIN_DATA_DIR, $GMERLIN_DATA_DIR)

dnl
dnl Optional libraries
dnl 

dnl xmms1

XMMS1_REQUIRED="1.2.2"
have_xmms1="false"
AH_TEMPLATE([HAVE_XMMS1], [xmm1 libraries are there])

AM_PATH_XMMS($XMMS_REQUIRED, have_xmms1="true", AC_MSG_ERROR(gmerlin_visualizer needs xmms with headers, 1))

AM_CONDITIONAL(HAVE_XMMS1, test x$have_xmms1 = xtrue)
 
if test x$have_xmms1 = xtrue; then
AC_DEFINE(HAVE_XMMS1)
fi

AC_SUBST(XMMS1_REQUIRED)
AC_SUBST(XMMS_CFLAGS, $XMMS_CFLAGS)
AC_SUBST(XMMS_LIBS, $XMMS_LIBS)

AC_DEFINE_UNQUOTED(XMMS_VISUALIZATION_PLUGIN_DIR, "$XMMS_VISUALIZATION_PLUGIN_DIR", Directory for xmms plugins)

dnl libvisual

AH_TEMPLATE([HAVE_LIBVISUAL], [libvisual is here])
AH_TEMPLATE([LIBVISUAL_PLUGIN_BASE], [plugin directory for libvisual])

LIBVISUAL_REQUIRED="0.3.0"
have_libvisual="false"

dnl Disable libvisual for now
dnl PKG_CHECK_MODULES(LIBVISUAL, libvisual >= $LIBVISUAL_REQUIRED, have_libvisual="true",have_libvisual="false")

if test x$have_libvisual = xtrue; then
    LIBVISUAL_PLUGIN_BASE=`pkg-config --variable=pluginsbasedir libvisual`
    AC_DEFINE_UNQUOTED(LIBVISUAL_PLUGIN_BASE, "$LIBVISUAL_PLUGIN_BASE", Directory for libvisual plugins)
    AC_DEFINE(HAVE_LIBVISUAL)
fi

AM_CONDITIONAL(HAVE_LIBVISUAL, test x$have_libvisual = xtrue)

dnl Check for libXx86vm (needed by goom-1.8.0)

AC_PATH_X

CFLAGS_save=$CFLAGS
LIBS_save=$LIBS
CFLAGS="$CFLAGS $GTK_CFLAGS"
LIBS="$LIBS $GTK_LIBS"

AC_CHECK_LIB(Xxf86vm,XF86VidModeQueryExtension,[
AC_DEFINE(XF86VIDMODE,1,[XF86VidMode extension is present])
VM_LIBS="-lXxf86vm"],,-L$x_libraries -lXext)
AC_SUBST(VM_LIBS)

CFLAGS=$CFLAGS_save
LIBS=$LIBS_save

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])
CFLAGS="-Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 $OPT_CFLAGS"

AC_OUTPUT(Makefile gmerlin-visualizer.spec gmerlin-visualizer.desktop src/Makefile m4/Makefile)
