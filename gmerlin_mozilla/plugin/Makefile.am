common_sources =

pkglib_LTLIBRARIES = libgmerlin-mozilla.la libgmerlin-mozilla-divx.la

noinst_LTLIBRARIES = common.la

common_la_SOURCES = \
gmerlin_mozilla.c \
widget.c \
menu.c \
pluginwindow.c \
buffer.c \
skin.c \
embedinfo.c \
cfgdialog.c \
browserfuncs.c \
scriptable.c


AM_LDFLAGS = -avoid-version -module -export-dynamic
AM_CFLAGS = @MOZPLUG_CFLAGS@ @GMERLIN_CFLAGS@ @GMERLIN_GTK_CFLAGS@

libgmerlin_mozilla_la_SOURCES = \
plugin-gmerlin.c
libgmerlin_mozilla_la_LIBADD = common.la @GMERLIN_LIBS@ @GMERLIN_GTK_LIBS@ 

libgmerlin_mozilla_divx_la_SOURCES = \
plugin-divx.c
libgmerlin_mozilla_divx_la_LIBADD = common.la @GMERLIN_LIBS@ @GMERLIN_GTK_LIBS@ 


EXTRA_DIST = plugin.c
