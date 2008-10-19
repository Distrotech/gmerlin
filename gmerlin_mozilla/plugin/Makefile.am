common_sources =

lib_LTLIBRARIES = gmerlin-mozilla.la

AM_LDFLAGS = -avoid-version -module -export-dynamic
AM_CFLAGS = @MOZPLUG_CFLAGS@ @GMERLIN_CFLAGS@ @GMERLIN_GTK_CFLAGS@

gmerlin_mozilla_la_SOURCES = plugin.c gmerlin_mozilla.c widget.c menu.c pluginwindow.c
gmerlin_mozilla_la_LIBADD = @GMERLIN_LIBS@ @GMERLIN_GTK_LIBS@
