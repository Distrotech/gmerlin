AC_INIT(src/mainwindow.c)

AM_CONFIG_HEADER(src/config.h)

AM_INIT_AUTOMAKE(gmerlin-visualizer, 1.0.1)
AC_PROG_LIBTOOL

GMERLIN_REQUIRED="0.3.2pre1"

PKG_CHECK_MODULES(GMERLIN, gmerlin = $GMERLIN_REQUIRED, ,
                  AC_MSG_ERROR(gmerlin_visualizer needs gmerlin with headers))

AC_SUBST(GMERLIN_REQUIRED)

GMERLIN_DATA_DIR=`pkg-config --variable=prefix gmerlin`
GMERLIN_DATA_DIR="$GMERLIN_DATA_DIR/share/gmerlin"
echo "Gmerlin data dir: $GMERLIN_DATA_DIR"

XMMS1_REQUIRED="1.2.2"
have_xmms1="false"
AH_TEMPLATE([HAVE_XMMS1], [Vorbis libraries are there])

AM_PATH_XMMS($XMMS_REQUIRED, have_xmms1="true", AC_MSG_ERROR(gmerlin_visualizer needs xmms with headers, 1))

AM_CONDITIONAL(HAVE_XMMS1, test x$have_xmms1 = xtrue)
 
if test x$have_xmms1 = xtrue; then
AC_DEFINE(HAVE_XMMS1)
fi

AC_SUBST(XMMS_REQUIRED)

GTK_REQUIRED="1.2.8"
AM_PATH_GTK($GTK_REQUIRED, , AC_MSG_ERROR(gmerlin_visualizer needs gtk with headers, 1))
AC_SUBST(GTK_REQUIRED)

dnl Check for libXx86vm (needed by goom-1.8.0)

AC_PATH_X

CFLAGS_save=$CFLAGS
LIBS_save=$LIBS
CFLAGS="$CFLAGS $GTK_CFLAGS"
LIBS="$LIBS $GTK_LIBS"

MM_LIBS=
AC_CHECK_LIB(Xxf86vm,XF86VidModeQueryExtension,[
AC_DEFINE(XF86VIDMODE,1,[XF86VidMode extension is present])
VM_LIBS="-lXxf86vm"],,-L$x_libraries -lXext)

dnl DGA_LIBS=
dnl if test x$dga = "xyes"; then
dnl          AC_CHECK_LIB(Xxf86dga,XF86DGAQueryExtension,[
dnl          AC_DEFINE(XF86DGA,1,[XF86DGA extension is present])
dnl          DGA_LIBS="-lXxf86dga"],,-L$x_libraries -lXext)
dnl fi

CFLAGS=$CFLAGS_save
LIBS=$LIBS_save

CFLAGS="-Wall"

have_debug=false

AC_ARG_ENABLE(debug,
AC_HELP_STRING([--enable-debug],[Enable debugging (disable optimization)]),
[case "${enableval}" in
   yes) have_debug=true ;;
   no)  have_debug=false ;;
esac],have_debug=false)

if test x$have_debug = "xtrue"; then
 CFLAGS="-g $CFLAGS"
else
 CFLAGS="-O2 $CFLAGS"
fi

AC_DEFINE_UNQUOTED(XMMS_VISUALIZATION_PLUGIN_DIR, "$XMMS_VISUALIZATION_PLUGIN_DIR", Directory for xmms plugins)

dnl AC_SUBST(DGA_LIBS)
AC_SUBST(VM_LIBS)


AC_SUBST(GMERLIN_CFLAGS, $GMERLIN_CFLAGS)
AC_SUBST(GMERLIN_LIBS, $GMERLIN_LIBS)

AC_SUBST(GMERLIN_DATA_DIR, $GMERLIN_DATA_DIR)

AC_SUBST(GTK_CFLAGS, $GTK_CFLAGS)
AC_SUBST(GTK_LIBS, $GTK_LIBS)

AC_SUBST(XMMS_CFLAGS, $XMMS_CFLAGS)
AC_SUBST(XMMS_LIBS, $XMMS_LIBS)

AC_OUTPUT(Makefile gmerlin-visualizer.spec gmerlin-visualizer.desktop src/Makefile)
