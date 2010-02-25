plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\"  \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = i_mikmod.la

i_mikmod_la_SOURCES  = i_mikmod.c 
i_mikmod_la_LIBADD = @GMERLIN_DEP_LIBS@ @MODULE_LIBADD@

