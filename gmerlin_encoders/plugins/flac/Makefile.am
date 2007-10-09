libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS = @FLAC_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = \
e_flac.la

e_flac_la_SOURCES = e_flac.c
e_flac_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @top_srcdir@/lib/libbgflac.la @FLAC_LIBS@
