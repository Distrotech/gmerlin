libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @VORBIS_CFLAGS@

lib_LTLIBRARIES = i_vorbis.la

i_vorbis_la_SOURCES = i_vorbis.c
i_vorbis_la_LDFLAGS  = -avoid-version -module
i_vorbis_la_LIBADD = @VORBIS_LIBS@ @VORBISFILE_LIBS@
