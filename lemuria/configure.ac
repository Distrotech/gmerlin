AC_INIT
AC_CONFIG_SRCDIR(include/lemuria.h)
AM_CONFIG_HEADER(include/config.h)

AM_INIT_AUTOMAKE(lemuria, 2.0.0)

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

dnl
dnl Gmerlin
dnl

PKG_CHECK_MODULES(GAVL, gavl >= "0.2.6", ,
                  AC_MSG_ERROR(lemuria needs gavl with headers))

PKG_CHECK_MODULES(GMERLIN, gmerlin >= "0.3.6", ,
                  AC_MSG_ERROR(lemuria needs gmerlin with headers))

GMERLIN_PLUGIN_DIR=`pkg-config --variable=prefix gmerlin`"/lib/gmerlin/plugins"
AC_SUBST(GMERLIN_PLUGIN_DIR)

dnl LDFLAGS for plugins
GMERLIN_PLUGIN_LDFLAGS="-export-symbols "`pkg-config --variable=prefix gmerlin`"/share/gmerlin/plugin.sym"

AC_SUBST(GMERLIN_PLUGIN_LDFLAGS)

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

if test x$have_GLX = "xno"; then
  AC_MSG_ERROR(Lemuria needs GLX headers, 1)
else
  echo "Found GLX"
fi


have_esd=true
AM_PATH_ESD(0.2.19, , have_esd=false)

LQT_OPT_CFLAGS($host_cpu, ["--fast-math -O2"])

CFLAGS="$OPT_CFLAGS $GL_CFLAGS"

LQT_TRY_CFLAGS(-Wall, CFLAGS="$CFLAGS -Wall", )
LQT_TRY_CFLAGS(-Wmissing-declarations, CFLAGS="$CFLAGS -Wmissing-declarations", )
LQT_TRY_CFLAGS(-Wdeclaration-after-statement, CFLAGS="$CFLAGS -Wdeclaration-after-statement")


if test "x$LQT_DEBUG" = "xtrue"; then
  CFLAGS="$CFLAGS -DDEBUG"
fi

AM_CONDITIONAL(HAVE_ESD, test x$have_esd = xtrue)

AC_CONFIG_SUBDIRS(lib/xaos)

# libdir=$XMMS_VISUALIZATION_PLUGIN_DIR

AC_SUBST(ESD_CFLAGS, $ESD_CFLAGS)
AC_SUBST(ESD_LIBS, $ESD_LIBS)

AC_SUBST(GTK_CFLAGS, $GTK_CFLAGS)
AC_SUBST(GTK_LIBS,   $GTK_LIBS)
AC_SUBST(GL_LIBS,   $GL_LIBS)

AH_TEMPLATE([PLUGIN_DIR], "Where to find goom")

AC_DEFINE_UNQUOTED([PLUGIN_DIR], "$XMMS_VISUALIZATION_PLUGIN_DIR")
AC_OUTPUT(Makefile lib/Makefile lib/meshes/Makefile lib/images/Makefile lib/gradients/Makefile plugins/Makefile plugins/gmerlin/Makefile src/Makefile include/Makefile lemuria.spec)
