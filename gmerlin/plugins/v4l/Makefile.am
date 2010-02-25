INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = i_v4l.la

i_v4l_la_SOURCES  = i_v4l.c pwc.c
i_v4l_la_LIBADD  = @MODULE_LIBADD@

noinst_HEADERS = pwc-ioctl.h pwc.h

