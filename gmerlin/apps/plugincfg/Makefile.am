CFLAGS = @CFLAGS@ @GTK_CFLAGS@
LIBS = @LIBS@

LDFLAGS = -export-dynamic

bin_PROGRAMS = gmerlin_plugincfg

gmerlin_plugincfg_SOURCES = gmerlin_plugincfg.c
gmerlin_plugincfg_LDADD = \
@top_srcdir@/lib/gtk/libgmerlin_gtk.la \
@top_srcdir@/lib/libgmerlin.la
