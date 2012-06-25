plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@XML2_CFLAGS@ \
@GTK_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-visualizer.desktop

bin_PROGRAMS = gmerlin_visualizer

gmerlin_visualizer_SOURCES = \
gmerlin_visualizer.c

gmerlin_visualizer_LDADD = $(top_builddir)/lib/libgmerlin.la $(top_builddir)/lib/gtk/libgmerlin_gtk.la @GTK_LIBS@

EXTRA_DIST = gmerlin-visualizer.desktop
