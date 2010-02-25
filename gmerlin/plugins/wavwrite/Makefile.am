plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = e_wav.la

e_wav_la_SOURCES  = e_wav.c
e_wav_la_LIBADD  = @MODULE_LIBADD@

