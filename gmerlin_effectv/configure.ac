AC_INIT
AC_CONFIG_SRCDIR([include/gmerlin_effectv.h])

AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(gmerlin-effectv, 1.1.0)

AC_DISABLE_STATIC

CFLAGS_SAVE=$CFLAGS
AC_PROG_LIBTOOL
CFLAGS=$CFLAGS_SAVE

dnl
dnl Check for Dependencies package
dnl

GMERLIN_DEP_DIR=/opt/gmerlin

if test -d $GMERLIN_DEP_DIR; then
  have_opt_gmerlin="true"
  export PKG_CONFIG_PATH=$GMERLIN_DEP_DIR/lib/pkgconfig:$PKG_CONFIG_PATH
  GMERLIN_DEP_CFLAGS="-I$GMERLIN_DEP_DIR/include"
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

LDFLAGS=$GMERLIN_DEP_RPATH
AC_SUBST(GMERLIN_DEP_LIBS)

dnl
dnl Features
dnl

AC_CHECK_SIZEOF(float,4)
AC_C_BIGENDIAN(,,AC_MSG_ERROR("Cannot detect endianess"))

dnl
dnl Required libraries
dnl

GMERLIN_REQUIRED="0.4.0"
PKG_CHECK_MODULES(GMERLIN, gmerlin >= $GMERLIN_REQUIRED, , AC_MSG_ERROR("gmerlin not found"))

gmerlin_plugindir='$(libdir)/gmerlin/plugins'

dnl LDFLAGS for plugins
GMERLIN_PLUGIN_LDFLAGS="-export-symbols "`pkg-config --variable=prefix gmerlin`"/share/gmerlin/plugin.sym"

AC_SUBST(GMERLIN_PLUGIN_LDFLAGS)
AC_SUBST(gmerlin_plugindir)
AC_SUBST(GMERLIN_REQUIRED)

AM_GNU_GETTEXT([external])

dnl
dnl CFLAGS
dnl

LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer -ffast-math"])

CFLAGS="$CFLAGS -D_REENTRANT -D_FILE_OFFSET_BITS=64 $OPT_CFLAGS $GMERLIN_CFLAGS"

LQT_TRY_CFLAGS(-Wall, CFLAGS="$CFLAGS -Wall")
LQT_TRY_CFLAGS(-Wmissing-declarations, CFLAGS="$CFLAGS -Wmissing-declarations" )
LQT_TRY_CFLAGS(-Wdeclaration-after-statement, CFLAGS="$CFLAGS -Wdeclaration-after-statement")

dnl
dnl LIBS
dnl

LIBS="$GMERLIN_LIBS"

if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix="${prefix}"
fi

AC_CONFIG_FILES([Makefile  po/Makefile.in \
include/Makefile \
lib/Makefile \
m4/Makefile \
utils/Makefile \
plugins/Makefile \
])

AC_OUTPUT
