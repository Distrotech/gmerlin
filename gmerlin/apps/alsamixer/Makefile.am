AM_CFLAGS = @GTK_CFLAGS@ @ALSA_CFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-alsamixer.desktop

bin_PROGRAMS = gmerlin_alsamixer

gmerlin_alsamixer_SOURCES = card.c main.c mixer.c g_control.c g_card.c g_mixer.c
gmerlin_alsamixer_LDADD = @top_srcdir@/lib/libgmerlin.la @top_srcdir@/lib/gtk/libgmerlin_gtk.la @ALSA_LIBS@ -lpthread

noinst_HEADERS = alsamixer.h
