INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"
lib_LTLIBRARIES = i_v4l2.la

i_v4l2_la_SOURCES  = i_v4l2.c v4l2_common.c
i_v4l2_la_LIBADD  = @MODULE_LIBADD@

noinst_HEADERS = v4l2_common.h

