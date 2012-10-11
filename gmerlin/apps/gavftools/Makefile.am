plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

noinst_LTLIBRARIES = \
libgavftools.la

libgavftools_la_SOURCES = \
gavftools.c \
program.c

noinst_HEADERS = gavftools.h

INCLUDES = -I$(top_srcdir)/include

# dist_man_MANS = gmerlin-record.1

bin_PROGRAMS = gavf-record
gavf_record_LDFLAGS = @GMERLIN_EXE_LDFLAGS@
gavf_record_LDADD = libgavftools.la $(top_builddir)/lib/libgmerlin.la -ldl 
