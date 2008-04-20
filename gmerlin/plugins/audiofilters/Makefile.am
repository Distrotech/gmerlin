libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = fa_volume.la fa_sampleformat.la

fa_volume_la_SOURCES  = fa_volume.c 

fa_sampleformat_la_SOURCES  = fa_sampleformat.c
