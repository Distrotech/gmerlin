AM_CFLAGS = @XML2_CFLAGS@ @GTK_CFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-kbd.desktop

bin_PROGRAMS = gmerlin_kbd gmerlin_kbd_config

gmerlin_kbd_config_SOURCES = \
kbd.c \
kbd_config.c \
kbd_remote.c \
keys_xml.c


gmerlin_kbd_SOURCES = \
kbd_daemon.c \
kbd.c \
keys_xml.c

noinst_HEADERS = \
kbd.h \
kbd_remote.h

gmerlin_kbd_LDADD = ../../lib/libgmerlin.la @X_LIBS@
gmerlin_kbd_config_LDADD = ../../lib/libgmerlin.la ../../lib/gtk/libgmerlin_gtk.la


