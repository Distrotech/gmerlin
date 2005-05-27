libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @VORBIS_CFLAGS@

lib_LTLIBRARIES = \
e_vorbis.la

e_vorbis_la_SOURCES = e_vorbis.c
e_vorbis_la_LDFLAGS = -avoid-version -module
e_vorbis_la_LIBADD = @top_srcdir@/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ 
