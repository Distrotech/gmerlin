AM_CFLAGS = @GTK_CFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-transcoder.desktop

bin_PROGRAMS = gmerlin_transcoder

gmerlin_transcoder_SOURCES = \
main.c \
pluginwindow.c \
transcoder_window.c \
trackdialog.c \
tracklist.c

gmerlin_transcoder_LDADD = \
@top_srcdir@/lib/gtk/libgmerlin_gtk.la \
@top_srcdir@/lib/libgmerlin.la

noinst_HEADERS = transcoder_window.h tracklist.h pluginwindow.h
