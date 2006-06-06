libdir=@GMERLIN_PLUGIN_DIR@

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


lib_LTLIBRARIES = \
e_vorbis.la \
$(theora_plugins) \
$(speex_plugins)

e_vorbis_la_CFLAGS = @VORBIS_CFLAGS@
e_vorbis_la_SOURCES = e_vorbis.c vorbis.c ogg_common.c
e_vorbis_la_LDFLAGS = -avoid-version -module
e_vorbis_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ 

e_theora_la_CFLAGS = @THEORA_CFLAGS@ @VORBIS_CFLAGS@ @SPEEX_CFLAGS@
e_theora_la_SOURCES = e_theora.c vorbis.c $(SPEEX_SOURCES) theora.c ogg_common.c
e_theora_la_LDFLAGS = -avoid-version -module
e_theora_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ @THEORA_LIBS@ @SPEEX_LIBS@

e_speex_la_CFLAGS = @SPEEX_CFLAGS@
e_speex_la_SOURCES = e_speex.c speex.c ogg_common.c
e_speex_la_LDFLAGS = -avoid-version -module
e_speex_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @SPEEX_LIBS@

noinst_HEADERS = ogg_common.h