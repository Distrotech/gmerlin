libdir=@GMERLIN_PLUGIN_DIR@

CFLAGS = @CFLAGS@ @VORBIS_CFLAGS@
LIBS = @LIBS@ @VORBIS_LIBS@ @VORBISFILE_LIBS@

lib_LTLIBRARIES = i_vorbis.la

i_vorbis_la_SOURCES = i_vorbis.c
i_vorbis_la_LDFLAGS  = -avoid-version -module
