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
Desktop_DATA = gmerlin-kbd.desktop

bin_PROGRAMS = gmerlin_kbd gmerlin_kbd_config

gmerlin_kbd_config_SOURCES = \
kbd.c \
keys.c \
kbd_config.c \
kbd_remote.c \
keys_xml.c


gmerlin_kbd_SOURCES = \
kbd_daemon.c \
keys.c \
kbd.c \
keys_xml.c

noinst_HEADERS = \
kbd.h \
kbd_remote.h

gmerlin_kbd_LDADD = $(top_builddir)/lib/libgmerlin.la @X_LIBS@
gmerlin_kbd_config_LDADD = $(top_builddir)/lib/libgmerlin.la $(top_builddir)/lib/gtk/libgmerlin_gtk.la

EXTRA_DIST = gmerlin-kbd.desktop
