INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS =  @PULSEAUDIO_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"
LIBS = @GMERLIN_DEP_LIBS@ @PULSEAUDIO_LIBS@

lib_LTLIBRARIES = \
oa_pulse.la \
i_pulse.la

oa_pulse_la_SOURCES = oa_pulse.c pulseaudio_common.c
i_pulse_la_SOURCES = i_pulse.c pulseaudio_common.c

noinst_HEADERS = pulseaudio_common.h
