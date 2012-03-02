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
Desktop_DATA = gmerlin-player.desktop

bin_PROGRAMS = gmerlin gmerlin_remote

dist_man_MANS = gmerlin.1 gmerlin_remote.1

bin_SCRIPTS = gmerlin_launcher

gmerlin_SOURCES = \
cfgdialog.c \
display.c \
gmerlin.c \
main.c \
mainmenu.c \
playerwindow.c \
remote.c \
skin.c \
skinbrowser.c

gmerlin_remote_SOURCES = \
gmerlin_remote.c

noinst_HEADERS = \
display.h \
gmerlin.h \
player_remote.h \
playerwindow.h

gmerlin_LDADD = $(top_builddir)/lib/libgmerlin.la $(top_builddir)/lib/gtk/libgmerlin_gtk.la @XML2_LIBS@
gmerlin_remote_LDADD = $(top_builddir)/lib/libgmerlin.la

EXTRA_DIST = gmerlin_launcher gmerlin-player.desktop
