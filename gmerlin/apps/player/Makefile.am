CFLAGS = @CFLAGS@ @XML2_CFLAGS@ @GTK_CFLAGS@
LIBS = @LIBS@

bin_PROGRAMS = gmerlin
gmerlin_SOURCES = \
cfgdialog.c \
display.c \
gmerlin.c \
main.c \
mainmenu.c \
playerwindow.c \
pluginwindow.c \
skin.c

noinst_HEADERS = \
display.h \
gmerlin.h \
playerwindow.h \
pluginwindow.h


gmerlin_LDADD = ../../lib/libgmerlin.la ../../lib/gtk/libgmerlin_gtk.la
