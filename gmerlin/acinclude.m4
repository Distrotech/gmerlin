# Configure paths for libquicktime
# Stolen from SDL for libquicktime by Arthur Peters
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

# Added LQT_PLUGIN_* variables for libquicktime

dnl AC_PATH_LQT([MINIMUM-VERSION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND], [USE-LQT-INCLUDES], [MODULES...])
dnl Test for libquicktime, and define LQT_CFLAGS, LQT_LIBS, LQT_PLUGIN_DIR, LQT_PLUGIN_CFLAGS, and LQT_PLUGIN_LIBS
dnl
AC_DEFUN(AC_PATH_LQT,
[dnl 
dnl Get the cflags and libraries from the lqt-config script
dnl
AC_ARG_WITH(lqt-prefix,[  --with-lqt-prefix=PFX   Prefix where libquicktime is installed (optional)],
            lqt_prefix="$withval", lqt_prefix="")
AC_ARG_WITH(lqt-exec-prefix,[  --with-lqt-exec-prefix=PFX Exec prefix where libquicktime is installed (optional)],
            lqt_exec_prefix="$withval", lqt_exec_prefix="")
AC_ARG_ENABLE(lqttest, [  --disable-lqttest       Do not try to compile and run a test libquicktime program],
		    , enable_lqttest=yes)

  use_lqt_includes=ifelse([$4],[],no,$4)
  if test x$use_lqt_includes == xyes; then
     lqtconf_args="$lqtconf_args --use-lqt-includes"
  fi

  if test x$lqt_exec_prefix != x ; then
     lqtconf_args="$lqtconf_args --exec-prefix=$lqt_exec_prefix"
     if test x${LQT_CONFIG+set} != xset ; then
        LQT_CONFIG=$lqt_exec_prefix/bin/lqt-config
     fi
  fi
  if test x$lqt_prefix != x ; then
     lqtconf_args="$lqtconf_args --prefix=$lqt_prefix"
     if test x${LQT_CONFIG+set} != xset ; then
        LQT_CONFIG=$lqt_prefix/bin/lqt-config
     fi
  fi

  AC_PATH_PROG(LQT_CONFIG, lqt-config, no)
  min_lqt_version=ifelse([$1],[],0.9.0,$1)
  AC_MSG_CHECKING(for libquicktime - version >= $min_lqt_version)
  no_lqt=""
  lqt_modules="ifelse([$5],[],[],$5)"
  if test "$lqt_modules"; then
     $LQT_CONFIG $lqt_modules 2> /dev/null > /dev/null
     if test $? -ne 0 && test "$LQT_CONFIG" != "no"; then
	no_lqt=yes
	bad_params=yes
     else
	lqtconf_args="$lqtconf_args $lqt_modules"
     fi
  fi

  if test "$LQT_CONFIG" = "no" || test "x$no_lqt" != x; then
    no_lqt=yes
  else
    LQT_CFLAGS=`$LQT_CONFIG $lqtconf_args --cflags`
    LQT_LIBS=`$LQT_CONFIG $lqtconf_args --libs`
    LQT_PLUGIN_LIBS=`$LQT_CONFIG $lqtconf_args --plugin-libs`
    LQT_PLUGIN_CFLAGS=`$LQT_CONFIG $lqtconf_args --plugin-cflags`
    LQT_PLUGIN_DIR=`$LQT_CONFIG $lqtconf_args --plugin-dir`

    lqt_major_version=`$LQT_CONFIG $lqtconf_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
    lqt_minor_version=`$LQT_CONFIG $lqtconf_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`
    lqt_micro_version=`$LQT_CONFIG $lqtconf_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\3/'`
    if test "x$enable_lqttest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $LQT_CFLAGS"
      LIBS="$LIBS $LQT_LIBS"
dnl
dnl Now check if the installed libquicktime is sufficiently new. (Also sanity
dnl checks the results of lqt-config to some extent
dnl
      rm -f conf.lqttest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <quicktime/quicktime.h>

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

void func()
{
  quicktime_t *file = quicktime_open( "file.mov", 0, 1 );
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.lqttest");
  */
  { FILE *fp = fopen("conf.lqttest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_lqt_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_lqt_version");
     exit(1);
   }

   if (($lqt_major_version > major) ||
      (($lqt_major_version == major) && ($lqt_minor_version > minor)) ||
      (($lqt_major_version == major) && ($lqt_minor_version == minor) && ($lqt_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'lqt-config --version' returned %d.%d.%d, but the minimum version\n", $lqt_major_version, $lqt_minor_version, $lqt_micro_version);
      printf("*** of libquicktime required is %d.%d.%d. If lqt-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If lqt-config was wrong, set the environment variable LQT_CONFIG\n");
      printf("*** to point to the correct copy of lqt-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_lqt=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi

  if test "x$no_lqt" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$bad_params" = "yes" ; then
	echo "*** The list of modules provided (\"$lqt_modules\") contains an"
	echo "*** invalid modules or is malformed in some other way. It"
	echo "*** should be space seperated."
     elif test "$LQT_CONFIG" = "no" ; then
       echo "*** The lqt-config script installed by libquicktime could not be found"
       echo "*** If libquicktime was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LQT_CONFIG environment variable to the"
       echo "*** full path to lqt-config."
     else
       if test -f conf.lqttest ; then
        :
       else
          echo "*** Could not run libquicktime test program, checking why..."
          CFLAGS="$CFLAGS $LQT_CFLAGS"
          LIBS="$LIBS $LQT_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <quicktime/quicktime.h>

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding libquicktime or finding the wrong"
          echo "*** version of libquicktime. If it is not finding libquicktime, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means libquicktime was incorrectly installed"
          echo "*** or that you have moved libquicktime since it was installed. In the latter case, you"
          echo "*** may want to edit the lqt-config script: $LQT_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     LQT_CFLAGS=""
     LQT_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(LQT_CFLAGS)
  AC_SUBST(LQT_LIBS)
  AC_SUBST(LQT_PLUGIN_LIBS)
  AC_SUBST(LQT_PLUGIN_CFLAGS)
  AC_SUBST(LQT_PLUGIN_DIR)
  rm -f conf.lqttest
])

dnl AC_TRY_CFLAGS (CFLAGS, [ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check if $CC supports a given set of cflags
AC_DEFUN([AC_TRY_CFLAGS],
    [AC_MSG_CHECKING([if $CC supports $1 flags])
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$1"
    AC_TRY_COMPILE([],[],[ac_cv_try_cflags_ok=yes],[ac_cv_try_cflags_ok=no])
    CFLAGS="$SAVE_CFLAGS"
    AC_MSG_RESULT([$ac_cv_try_cflags_ok])
    if test x"$ac_cv_try_cflags_ok" = x"yes"; then
        ifelse([$2],[],[:],[$2])
    else
        ifelse([$3],[],[:],[$3])
    fi])

