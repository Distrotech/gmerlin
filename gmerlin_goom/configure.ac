AC_INIT(include/goom.h)
AM_CONFIG_HEADER(include/config.h)

AM_DISABLE_STATIC
AM_INIT_AUTOMAKE(gmerlin_Goom, 2k4)

ACLOCAL="$ACLOCAL -I m4"

AM_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC

AC_C_BIGENDIAN

dnl Get the CFlags
CFLAGS="${CFLAGS}"
LDFLAGS="${LDFLAGS}"

dnl *** MMX ***

dnl rm -f mmx_zoom.s
HAVE_MMX="no"
HAVE_PPC="no"
MACTARGET="no"

dnl HOST
case "$host" in
*-apple-darwin*)
	AC_ARG_ENABLE(mac_frontends, [The Mac iTunes plugin and Standalone application], [MACTARGET="$enableval"], [MACTARGET="yes"])
	if test "x$MACTARGET" = "xyes"; then
		MACTARGET="yes"
		MACFOLDER="mac"
		AC_SUBST(MACFOLDER)
	fi
  	CCAS='$(CC)'
  	AC_SUBST(CCAS)
	;;
*-*-cygwin*)
  	CFLAGS="$CFLAGS -mno-cygwin -mwindows"
	LDFLAGS="$LDFLAGS -lmingw32"
	;;
esac

dnl ARCH
case "$host" in
i*86-*-*)
	AC_DEFINE(HAVE_MMX,[],[MMX support])
	AC_DEFINE(CPU_X86,[],[X86 architecture])
	HAVE_MMX="yes"
	;;

powerpc-*-*)
  	CCASFLAGS=-force_cpusubtype_ALL
  	AC_SUBST(CCASFLAGS)
	AC_DEFINE(CPU_POWERPC,[],[PowerPC architecture])
	HAVE_PPC="yes"
	;;

esac
AM_CONDITIONAL(HAVE_MMX,test "x$HAVE_MMX" = "xyes")
AM_CONDITIONAL(HAVE_PPC,test "x$HAVE_PPC" = "xyes")
AM_CONDITIONAL(MACTARGET,test "x$MACTARGET" = "xyes")

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
dnl
dnl

AC_CHECK_HEADER(pthread.h,,AC_MSG_ERROR([*** POSIX thread support not installed - please install first ***]))

PTHREAD_LIBS=error
AC_CHECK_LIB(pthread, pthread_attr_init, PTHREAD_LIBS="-lpthread")

if test "x$PTHREAD_LIBS" = xerror; then
    AC_CHECK_LIB(pthreads, pthread_attr_init, PTHREAD_LIBS="-lpthreads")
fi

if test "x$PTHREAD_LIBS" = xerror; then
    AC_CHECK_LIB(c_r, pthread_attr_init, PTHREAD_LIBS="-lc_r")
fi

if test "x$PTHREAD_LIBS" = xerror; then
    AC_CHECK_FUNC(pthread_attr_init, PTHREAD_LIBS="")
fi

AC_SUBST(PTHREAD_LIBS)

dnl AC_DEFINE(USE_ASM_MMX)
dnl ln -s mmx_zoom_x86.s mmx_zoom.s ;;
dnl *)
dnl ln -s mmx_zoom_dummy.s mmx_zoom.s ;;
dnl esac

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

AC_OUTPUT(Makefile include/Makefile lib/Makefile plugins/Makefile plugins/gmerlin/Makefile libgoom2.pc)

dnl *** nice user info ***

AC_MSG_NOTICE([goom2k4 was configured with the following options:])
if test "x$HAVE_PPC" = "xyes"; then
  AC_MSG_NOTICE([ ** PPC support enabled])
else
  AC_MSG_NOTICE([    PPC support disabled])
fi
if test "x$HAVE_MMX" = "xyes"; then
  AC_MSG_NOTICE([ ** MMX support enabled])
else
  AC_MSG_NOTICE([    MMX support disabled])
fi
if test "x$have_gmerlin" = "xtrue"; then
  AC_MSG_NOTICE([ ** Gmerlin plugin will be built])
else
  AC_MSG_NOTICE([    Gmerlin plugin will not be built])
fi
AC_MSG_NOTICE([ ** goom lib will be built])
