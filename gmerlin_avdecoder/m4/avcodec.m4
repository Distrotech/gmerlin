# Configure paths for libavcodec
# Burkhard Plaum, 2004-08-12

dnl Compile an avcodec test program and figure out the version

AC_DEFUN([ACL_CHECK_AVCODEC],[
AC_MSG_CHECKING([for build ID in libavcodec, libs: $AVCODEC_LIBS])
CFLAGS_save=$CFLAGS
LIBS_save=$LIBS

AVCODEC_CFLAGS=`echo $AVCODEC_CFLAGS | sed 's/ //'`

if test "x$AVCODEC_CFLAGS" = "x"; then
AVCODEC_CFLAGS="-I/usr/include"
fi

CFLAGS="$CFLAGS $AVCODEC_CFLAGS"

dnl Look for header
found_header="false"

AC_TRY_COMPILE([
#include <avcodec.h>],[],[found_header="true"])

if test $found_header = "false"; then
AC_TRY_COMPILE([
#include <ffmpeg/avcodec.h>],[], [found_header="true";AVCODEC_CFLAGS=${AVCODEC_CFLAGS}"/ffmpeg" ],)
fi

if test $found_header = "false"; then
AC_TRY_COMPILE([
#include <libavcodec/avcodec.h>],[], [found_header="true";AVCODEC_CFLAGS="$AVCODEC_CFLAGS/libavcodec" ],)
fi

CFLAGS="$CFLAGS $AVCODEC_CFLAGS"
LIBS="$LIBS $AVCODEC_LIBS"
avcodec_ok="false"
AC_TRY_RUN([
    #include <stdio.h>
    #include <avcodec.h>
    int main()
    {
    FILE * output;
    if(LIBAVCODEC_BUILD < $1)
      return -1;
    output=fopen("avcodec_version", "w");
    fprintf(output, AV_TOSTRING(LIBAVCODEC_VERSION));
    fclose(output);
    return 0;
    }
  ],
  [
    # program could be run
    if test "x$AVCODEC_VERSION" = "x"; then 
      AVCODEC_VERSION=`cat avcodec_version`
    fi
    rm -f avcodec_version
    avcodec_ok="true"
    AC_MSG_RESULT(ok)
  ],
  [
    # program could not be run
    AC_MSG_RESULT(failed)
  ])
CFLAGS="$CFLAGS_save"
LIBS="$LIBS_save"
])

dnl ACL_PATH_AVCODEC(BUILD_ID [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libavcodec, and define AVCODEC_CFLAGS, AVCODEC_LIBS and
dnl AVCODEC_VERSION

AC_DEFUN([ACL_PATH_AVCODEC],[
AC_ARG_WITH(avcodec,[  --with-avcodec=PFX   Prefix where libavcodec is installed (optional)], avcodec_prefix="$withval", avcodec_prefix="")
dnl We need the _save variables because PKG_CHECK_MODULES will change
dnl the other variables
AVCODEC_CFLAGS_save=""
AVCODEC_LIBS_save=""
avcodec_done="false"

dnl
dnl First preference: configure options
dnl

if test "x$avcodec_prefix" != x; then
AVCODEC_CFLAGS="-I$avcodec_prefix/include"
AVCODEC_LIBS="-L$avcodec_prefix/lib -lavcodec"
ACL_CHECK_AVCODEC([$1])
  if test "x$avcodec_ok" = "xtrue"; then
    avcodec_done="true"
  fi
fi

dnl
dnl Second Perference: Autodetect
dnl

if test "x$avcodec_done" = "xfalse"; then
  PKG_CHECK_MODULES(AVCODEC, libavcodec, avcodec_orig="true", avcodec_orig="false")
  ACL_CHECK_AVCODEC([$1])
  if test "x$avcodec_ok" = "xtrue"; then
    avcodec_done="true"
  fi
fi

if test "x$avcodec_done" = "xtrue"; then
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , :, [$3])
fi
])
