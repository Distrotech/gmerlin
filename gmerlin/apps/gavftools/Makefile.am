plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

noinst_LTLIBRARIES = \
libgavftools.la

libgavftools_la_SOURCES = \
gavftools.c

noinst_HEADERS = gavftools.h gavf-decode.h

INCLUDES = -I$(top_srcdir)/include

LDADD   = libgavftools.la $(top_builddir)/lib/libgmerlin.la -ldl
AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

# dist_man_MANS = gmerlin-record.1

bin_PROGRAMS = \
gavf-decode \
gavf-demux \
gavf-encode \
gavf-filter \
gavf-info \
gavf-mux \
gavf-play \
gavf-recompress \
gavf-record \
gavf-tee

gavf_decode_SOURCES = gavf-decode.c \
decode_album.c \
decode_albumstream.c \
decode_load.c

