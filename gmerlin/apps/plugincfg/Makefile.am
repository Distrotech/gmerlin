plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@XML2_CFLAGS@ \
@GTK_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


bin_PROGRAMS = gmerlin_plugincfg

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-plugincfg.desktop

gmerlin_plugincfg_SOURCES = gmerlin_plugincfg.c
gmerlin_plugincfg_LDADD = \
$(top_builddir)/lib/gtk/libgmerlin_gtk.la \
$(top_builddir)/lib/libgmerlin.la
