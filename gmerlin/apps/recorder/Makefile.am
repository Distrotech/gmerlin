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
Desktop_DATA = gmerlin-recorder.desktop

bin_PROGRAMS = gmerlin_recorder

gmerlin_recorder_SOURCES = gmerlin_recorder.c recorder_window.c

gmerlin_recorder_LDADD = \
$(top_builddir)/lib/gtk/libgmerlin_gtk.la \
$(top_builddir)/lib/libgmerlin.la

noinst_HEADERS = recorder_window.h
