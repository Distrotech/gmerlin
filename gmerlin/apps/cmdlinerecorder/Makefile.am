plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = gmerlin-record
dist_man_MANS = gmerlin-record.1

gmerlin_record_LDADD = $(top_builddir)/lib/libgmerlin.la -ldl
gmerlin_record_SOURCES=gmerlin-record.c
