AM_CFLAGS = @XML2_CFLAGS@ @GTK_CFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-transcoder.desktop

bin_PROGRAMS = gmerlin_transcoder gmerlin_transcoder_remote

gmerlin_transcoder_SOURCES = \
encoderwidget.c \
main.c \
pluginwindow.c \
transcoder_window.c \
trackdialog.c \
tracklist.c

gmerlin_transcoder_remote_SOURCES = gmerlin_transcoder_remote.c

gmerlin_transcoder_LDADD = \
@top_srcdir@/lib/gtk/libgmerlin_gtk.la \
@top_srcdir@/lib/libgmerlin.la

gmerlin_transcoder_remote_LDADD = \
@top_srcdir@/lib/libgmerlin.la

noinst_HEADERS = \
encoderwidget.h \
transcoder_remote.h \
transcoder_window.h \
tracklist.h \
pluginwindow.h \
trackdialog.h
