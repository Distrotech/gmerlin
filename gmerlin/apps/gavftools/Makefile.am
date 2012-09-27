plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

LDADD = $(top_builddir)/lib/libgmerlin.la -ldl

INCLUDES = -I$(top_srcdir)/include

# dist_man_MANS = gmerlin-record.1

bin_PROGRAMS = gavf-record

