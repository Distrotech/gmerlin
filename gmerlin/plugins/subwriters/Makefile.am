INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"
lib_LTLIBRARIES = e_subtext.la

e_subtext_la_SOURCES  = e_subtext.c
e_subtext_la_LIBADD  = @MODULE_LIBADD@


