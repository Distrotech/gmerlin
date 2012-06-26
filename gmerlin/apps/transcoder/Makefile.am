plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@XML2_CFLAGS@ \
@GTK_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-transcoder.desktop

bin_PROGRAMS = gmerlin_transcoder gmerlin_transcoder_remote

dist_man_MANS = gmerlin_transcoder.1 gmerlin_transcoder_remote.1

gmerlin_transcoder_SOURCES = \
ppwidget.c \
main.c \
transcoder_window.c \
trackdialog.c \
tracklist.c

gmerlin_transcoder_remote_SOURCES = gmerlin_transcoder_remote.c

gmerlin_transcoder_LDADD = \
$(top_builddir)/lib/gtk/libgmerlin_gtk.la \
$(top_builddir)/lib/libgmerlin.la @GTK_LIBS@

gmerlin_transcoder_remote_LDADD = \
$(top_builddir)/lib/libgmerlin.la

noinst_HEADERS = \
ppwidget.h \
transcoder_remote.h \
transcoder_window.h \
tracklist.h \
trackdialog.h

EXTRA_DIST = gmerlin-transcoder.desktop
