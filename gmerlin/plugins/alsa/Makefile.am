INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = @ALSA_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = oa_alsa.la i_alsa.la

oa_alsa_la_SOURCES = oa_alsa.c alsa_common.c
i_alsa_la_SOURCES  = i_alsa.c alsa_common.c

oa_alsa_la_LIBADD = @GMERLIN_DEP_LIBS@ @ALSA_LIBS@
i_alsa_la_LIBADD = @GMERLIN_DEP_LIBS@ @ALSA_LIBS@

noinst_HEADERS = alsa_common.h
