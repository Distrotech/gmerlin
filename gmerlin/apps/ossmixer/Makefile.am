AM_CFLAGS = @GTK_CFLAGS@

bin_PROGRAMS = gmerlin_ossmixer

gmerlin_ossmixer_SOURCES = ossmixer.c monitor.c
gmerlin_ossmixer_LDADD = @top_srcdir@/lib/libgmerlin.la @top_srcdir@/lib/gtk/libgmerlin_gtk.la -lpthread

noinst_HEADERS = monitor.h
