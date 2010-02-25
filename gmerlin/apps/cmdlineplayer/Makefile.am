plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = gmerlin_play
dist_man_MANS = gmerlin_play.1

gmerlin_play_LDADD = $(top_builddir)/lib/libgmerlin.la -ldl


