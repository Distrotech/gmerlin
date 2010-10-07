INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


vis_scope_la_LIBADD = @MODULE_LIBADD@ -lm
plugin_LTLIBRARIES = \
vis_scope.la

vis_scope_la_SOURCES  = vis_scope.c ../videofilters/colormatrix.c

