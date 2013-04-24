plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = gmerlin-dbedit
# dist_man_MANS = gmerlin-dbedit.1

gmerlin_dbedit_LDADD = $(top_builddir)/lib/libgmerlin.la -ldl
