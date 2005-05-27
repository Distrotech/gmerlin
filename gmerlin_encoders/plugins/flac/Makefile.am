libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @FLAC_CFLAGS@

lib_LTLIBRARIES = \
e_flac.la

e_flac_la_SOURCES = e_flac.c
e_flac_la_LDFLAGS = -avoid-version -module
e_flac_la_LIBADD = @top_srcdir@/lib/libgmerlin_encoders.la @FLAC_LIBS@
