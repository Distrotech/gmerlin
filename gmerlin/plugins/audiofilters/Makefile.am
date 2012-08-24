plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = \
fa_sampleformat.la \
fa_samplerate.la \
fa_volume.la 

fa_volume_la_SOURCES  = fa_volume.c 
fa_volume_la_LIBADD  =  @MODULE_LIBADD@

fa_sampleformat_la_SOURCES  = fa_sampleformat.c
fa_sampleformat_la_LIBADD  =  @MODULE_LIBADD@

fa_samplerate_la_SOURCES  = fa_samplerate.c
fa_samplerate_la_LIBADD  =  @MODULE_LIBADD@
