plugindir=$(pkglibdir)/plugins

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@XML2_CFLAGS@ \
@GTK_CFLAGS@ \
@ALSA_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-alsamixer.desktop

bin_PROGRAMS = gmerlin_alsamixer

gmerlin_alsamixer_SOURCES = card.c main.c mixer.c g_control.c g_card.c g_mixer.c
gmerlin_alsamixer_LDADD = $(top_builddir)/lib/libgmerlin.la $(top_builddir)/lib/gtk/libgmerlin_gtk.la @ALSA_LIBS@ -lpthread

noinst_HEADERS = alsamixer.h gui.h

EXTRA_DIST = gmerlin-alsamixer.desktop
