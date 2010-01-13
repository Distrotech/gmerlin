## Process this file with automake to produce Makefile.in.

gmerlin_plugindir = @GMERLIN_PLUGIN_DIR@

INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = @GMERLIN_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

AM_LDFLAGS = -avoid-version -module @GMERLIN_PLUGIN_LDFLAGS@
# AM_LDFLAGS = -avoid-version -module
gmerlin_plugin_LTLIBRARIES = \
i_gmerlerra.la

i_gmerlerra_la_SOURCES = i_gmerlerra.c
i_gmerlerra_la_LIBADD = $(top_builddir)/lib/libgmerlerra.la @GMERLIN_LIBS@
