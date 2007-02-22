libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = \
fv_equalizer.la \
fv_flip.la

fv_flip_la_SOURCES  = fv_flip.c
fv_flip_la_LDFLAGS = -avoid-version -module

fv_equalizer_la_SOURCES  = fv_equalizer.c
fv_equalizer_la_LDFLAGS = -avoid-version -module
