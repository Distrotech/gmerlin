AM_CFLAGS = @ALSA_CFLAGS@

libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = oa_alsa.la i_alsa.la

oa_alsa_la_SOURCES = oa_alsa.c alsa_common.c
i_alsa_la_SOURCES  = i_alsa.c alsa_common.c

oa_alsa_la_LDFLAGS = -avoid-version -module
i_alsa_la_LDFLAGS  = -avoid-version -module

oa_alsa_la_LIBADD = @ALSA_LIBS@
i_alsa_la_LIBADD = @ALSA_LIBS@

noinst_HEADERS = alsa_common.h
