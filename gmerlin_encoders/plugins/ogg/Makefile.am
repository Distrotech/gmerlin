libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

if HAVE_THEORA
theora_plugins = e_theora.la
else
theora_plugins =
endif

if HAVE_SPEEX
speex_plugins = e_speex.la
speex_sources = speex.c
else
speex_plugins =
speex_sources =
endif

if HAVE_FLAC
flac_plugins = e_flacogg.la
flac_sources = flac.c
bgflac_libs    = @top_srcdir@/lib/libbgflac.la @FLAC_LIBS@
else
flac_plugins =
flac_sources =
endif

lib_LTLIBRARIES = \
e_vorbis.la \
$(theora_plugins) \
$(speex_plugins) \
$(flac_plugins)

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

e_vorbis_la_CFLAGS = @VORBIS_CFLAGS@ $(AM_CFLAGS)
e_vorbis_la_SOURCES = e_vorbis.c vorbis.c ogg_common.c
e_vorbis_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ 

e_flacogg_la_CFLAGS = @OGG_CFLAGS@ @FLAC_CFLAGS@ $(AM_CFLAGS)
e_flacogg_la_SOURCES = e_flacogg.c flac.c ogg_common.c
e_flacogg_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la $(bgflac_libs) @OGG_LIBS@

e_theora_la_CFLAGS = @THEORA_CFLAGS@ @VORBIS_CFLAGS@ @SPEEX_CFLAGS@ @FLAC_CFLAGS@ $(AM_CFLAGS) $(AM_CFLAGS)
e_theora_la_SOURCES = e_theora.c vorbis.c $(speex_sources) $(flac_sources) theora.c ogg_common.c
e_theora_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ @THEORA_LIBS@ @SPEEX_LIBS@ $(bgflac_libs)

e_speex_la_CFLAGS = @SPEEX_CFLAGS@ $(AM_CFLAGS)
e_speex_la_SOURCES = e_speex.c speex.c ogg_common.c
e_speex_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @SPEEX_LIBS@ @OGG_LIBS@ 

noinst_HEADERS = ogg_common.h
