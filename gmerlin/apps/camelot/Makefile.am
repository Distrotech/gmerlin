AM_CFLAGS = @XML2_CFLAGS@ @GTK_CFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-camelot.desktop

bin_PROGRAMS = camelot

camelot_SOURCES = webcam.c webcam_window.c main.c
camelot_LDADD = \
@top_srcdir@/lib/gtk/libgmerlin_gtk.la \
@top_srcdir@/lib/libgmerlin.la

noinst_HEADERS = webcam.h webcam_window.h
