INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = @X_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = \
ov_x11.la

ov_x11_la_SOURCES = ov_x11.c x11_window.c
ov_x11_la_LIBADD =  @GMERLIN_DEP_LIBS@ @X_LIBS@ @XINERAMA_LIBS@ @XV_LIBS@

noinst_HEADERS = x11_window.h

