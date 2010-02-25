plugindir=$(pkglibdir)/plugins

INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = i_edl.la 

i_edl_la_SOURCES = i_edl.c 
i_edl_la_LIBADD =  @MODULE_LIBADD@

