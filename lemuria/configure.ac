AC_INIT
AC_CONFIG_SRCDIR(include/lemuria.h)
AM_CONFIG_HEADER(include/config.h)

AM_INIT_AUTOMAKE(lemuria, 2.0.0)

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl
dnl gmerlin
dnl 

have_gmerlin="false"

GMERLIN_REQUIRED="0.3.6"

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

dnl LDFLAGS for plugins
GMERLIN_PLUGIN_LDFLAGS="-export-symbols "`pkg-config --variable=prefix gmerlin`"/share/gmerlin/plugin.sym"

fi

AC_SUBST(GMERLIN_PLUGIN_DIR)
AC_SUBST(GMERLIN_REQUIRED)
AC_SUBST(GMERLIN_PLUGIN_LDFLAGS)

AM_CONDITIONAL(HAVE_GMERLIN, test x$have_gmerlin = xtrue)

dnl
dnl Libvisual
dnl

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
lv_plugin_dir=`pkg-config --variable=pluginsbasedir libvisual-0.4`
lv_plugin_dir="$lv_plugin_dir/actor"

AC_DEFINE_UNQUOTED(LV_PLUGIN_DIR, ["$lv_plugin_dir"], [libvisual plugin directory])
fi
AM_CONDITIONAL(HAVE_LV, test x$have_lv = xtrue)
AC_SUBST(LV_REQUIRED)

dnl
dnl Installation prefix to increase the chances to find goom
dnl

if test "x$prefix" = xNONE; then
  prefix="${ac_default_prefix}"
fi

AC_DEFINE_UNQUOTED(PREFIX, "$prefix", Installation prefix)

dnl
dnl X11
dnl

X_CLAGS=""
X_LIBS=""

AC_PATH_X
if test x$no_x != xyes; then
X_FLAGS="-I$x_includes"
X_LIBS="-L$x_libraries -lX11 -lXext"
have_x="true"
else
AC_MSG_ERROR(Lemuria X11 headers, 1)
fi

AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)

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
CFLAGS=$X_FLAGS
LIBS=$X_LIBS
AC_CHECK_LIB(Xinerama, XineramaQueryExtension,
             XINERAMA_LIBS="-lXinerama";have_xinerama="true")
if test x$have_xinerama="xtrue"; then
AC_DEFINE(HAVE_LIBXINERAMA)
fi
AC_SUBST(XINERAMA_LIBS)
CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS
fi

dnl
dnl OpenGL
dnl
MDL_HAVE_OPENGL

if test x$have_GL = "xno"; then
  AC_MSG_ERROR(Lemuria needs OpenGL headers, 1)
else
  echo "Found OpenGL"
fi

if test x$have_GLU = "xno"; then
  AC_MSG_ERROR(Lemuria needs GLU headers, 1)
else
  echo "Found GLU"
fi

LQT_OPT_CFLAGS($host_cpu, ["--fast-math -O2"])

CFLAGS="$OPT_CFLAGS $GL_CFLAGS"

LQT_TRY_CFLAGS(-Wall, CFLAGS="$CFLAGS -Wall", )
LQT_TRY_CFLAGS(-Wmissing-declarations, CFLAGS="$CFLAGS -Wmissing-declarations", )
LQT_TRY_CFLAGS(-Wdeclaration-after-statement, CFLAGS="$CFLAGS -Wdeclaration-after-statement")


if test "x$LQT_DEBUG" = "xtrue"; then
  CFLAGS="$CFLAGS -DDEBUG"
fi

AC_CONFIG_SUBDIRS(lib/xaos)

AC_SUBST(GL_LIBS,   $GL_LIBS)

AC_OUTPUT(Makefile lib/Makefile lib/meshes/Makefile lib/images/Makefile lib/gradients/Makefile plugins/Makefile plugins/gmerlin/Makefile include/Makefile lemuria.spec)

echo
echo
echo "Lemuria configuration summary"
echo
echo

echo -n "Libvisual: "
if test "x$have_lv" = "xtrue"; then
echo "Yes (CFLAGS=$LV_CFLAGS LIBS=$LV_LIBS)"
else
if test "x$test_lv" = "xtrue"; then
echo "Missing (Go to http://libvisual.sourceforge.net/)"
else
echo "Disabled"
fi
fi

echo -n "Gmerlin:   "
if test "x$have_gmerlin" = "xtrue"; then
echo "Yes (CFLAGS=$GMERLIN_CFLAGS LIBS=$GMERLIN_LIBS)"
else
if test "x$test_gmerlin" = "xtrue"; then
echo "Missing (Go to http://gmerlin.sourceforge.net/)"
else
echo "Disabled"
fi
fi

echo
echo "If you installed a library but it was not detected, check the file INSTALL"
echo "for troubleshooting tips. Also note that if the configure script reaches"
echo "this point, all missing packages are optional so compilation should succeed"
echo "anyway."
