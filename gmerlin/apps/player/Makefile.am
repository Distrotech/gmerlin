AM_CFLAGS = @XML2_CFLAGS@ @GTK_CFLAGS@

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-player.desktop

bin_PROGRAMS = gmerlin gmerlin_remote

bin_SCRIPTS = gmerlin_launcher

gmerlin_SOURCES = \
cfgdialog.c \
display.c \
gmerlin.c \
main.c \
mainmenu.c \
playerwindow.c \
pluginwindow.c \
remote.c \
skin.c \
skinbrowser.c

gmerlin_remote_SOURCES = \
gmerlin_remote.c

noinst_HEADERS = \
display.h \
gmerlin.h \
player_remote.h \
playerwindow.h \
pluginwindow.h

gmerlin_LDADD = ../../lib/libgmerlin.la ../../lib/gtk/libgmerlin_gtk.la
gmerlin_remote_LDADD = ../../lib/libgmerlin.la

EXTRA_DIST = gmerlin_laucher
