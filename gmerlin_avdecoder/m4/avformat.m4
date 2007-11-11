# Configure paths for libavformat
# Burkhard Plaum, 2004-08-12

dnl Compile an avformat test program and figure out the version

AC_DEFUN([ACL_CHECK_AVFORMAT],[
AC_MSG_CHECKING([for build ID in libavformat, libs: $AVFORMAT_LIBS])
CFLAGS_save=$CFLAGS
LIBS_save=$LIBS
CFLAGS="$CFLAGS $AVFORMAT_CFLAGS"
LIBS="$LIBS $AVFORMAT_LIBS"
avformat_ok="false"
AC_TRY_RUN([
    #include <stdio.h>
    #include <ffmpeg/avformat.h>
    int main()
    {
    FILE * output;
    if(LIBAVFORMAT_BUILD < $1)
      return -1;
    output=fopen("avformat_version", "w");
    fprintf(output, AV_TOSTRING(LIBAVFORMAT_VERSION));
    fclose(output);
    return 0;
    }
  ],
  [
    # program could be run
    if test "x$AVFORMAT_VERSION" = "x"; then 
      AVFORMAT_VERSION=`cat avformat_version`
    fi
    rm -f avformat_version
    avformat_ok="true"
    AC_MSG_RESULT(ok)
  ],
  [
    # program could not be run
    AC_MSG_RESULT(failed)
  ])
CFLAGS="$CFLAGS_save"
LIBS="$LIBS_save"
])

dnl ACL_PATH_AVFORMAT(BUILD_ID [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libavformat, and define AVFORMAT_CFLAGS, AVFORMAT_LIBS and
dnl AVFORMAT_VERSION

AC_DEFUN([ACL_PATH_AVFORMAT],[
AC_ARG_WITH(avformat,[  --with-avformat=PFX   Prefix where libavformat is installed (optional)], avformat_prefix="$withval", avformat_prefix="")
dnl We need the _save variables because PKG_CHECK_MODULES will change
dnl the other variables
AVFORMAT_CFLAGS_save=""
AVFORMAT_LIBS_save=""
avformat_done="false"

dnl
dnl First preference: configure options
dnl

if test "x$avformat_prefix" != x; then
AVFORMAT_CFLAGS="-I$avformat_prefix/include"
AVFORMAT_LIBS="-L$avformat_prefix/lib -lavformat"
ACL_CHECK_AVFORMAT([$1])
  if test "x$avformat_ok" = "xtrue"; then
    avformat_done="true"
  fi
fi

dnl
dnl Second Peference: ffmpeg_acl
dnl

if test "x$avformat_done" = "xfalse"; then
  PKG_CHECK_MODULES(AVFORMAT_ACL, avformat_acl, have_avformat_acl="true", have_avformat_acl="false")
  if test x"$have_avformat_acl" = "xtrue"; then
        AVFORMAT_CFLAGS=$AVFORMAT_ACL_CFLAGS
        AVFORMAT_LIBS=$AVFORMAT_ACL_LIBS
        ACL_CHECK_AVFORMAT([$1])
    if test "x$avformat_ok" = "xtrue"; then
      avformat_done="true"
      AVFORMAT_VERSION=`pkg-config --modversion avformat_acl`
      fi
  fi
fi

dnl
dnl Third Perference: Autodetect
dnl

if test "x$avformat_done" = "xfalse"; then
  PKG_CHECK_MODULES(AVFORMAT, libavformat, avformat_orig="true", avformat_orig="false")
  ACL_CHECK_AVFORMAT([$1])
  if test "x$avformat_ok" = "xtrue"; then
    avformat_done="true"
  fi
fi

if test "x$avformat_done" = "xtrue"; then
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , :, [$3])
fi
])
