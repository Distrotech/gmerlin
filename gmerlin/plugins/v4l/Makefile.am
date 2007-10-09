libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = i_v4l.la

i_v4l_la_SOURCES  = i_v4l.c pwc.c

noinst_HEADERS = pwc-ioctl.h pwc.h

