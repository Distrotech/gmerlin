INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS =  \
@PULSEAUDIO_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = \
oa_pulse.la \
i_pulse.la

oa_pulse_la_SOURCES = oa_pulse.c pulseaudio_common.c
oa_pulse_la_LIBADD = @GMERLIN_DEP_LIBS@ @PULSEAUDIO_LIBS@ @MODULE_LIBADD@

i_pulse_la_SOURCES = i_pulse.c pulseaudio_common.c
i_pulse_la_LIBADD = @GMERLIN_DEP_LIBS@ @PULSEAUDIO_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = pulseaudio_common.h
