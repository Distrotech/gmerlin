AM_CFLAGS = @GTK_CFLAGS@

bin_PROGRAMS = gmerlin_plugincfg

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-plugincfg.desktop

gmerlin_plugincfg_SOURCES = gmerlin_plugincfg.c
gmerlin_plugincfg_LDADD = \
@top_srcdir@/lib/gtk/libgmerlin_gtk.la \
@top_srcdir@/lib/libgmerlin.la
