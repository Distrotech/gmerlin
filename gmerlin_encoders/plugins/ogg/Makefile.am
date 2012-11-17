gmerlin_plugindir = @gmerlin_plugindir@

INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

if HAVE_THEORAENC
theoraenc_plugins = e_theora.la c_theoraenc.la
theora_sources = theora.c

if HAVE_SHOUT
shout_plugins = b_ogg.la
bgshout_libs    = $(top_builddir)/lib/libbgshout.la @SHOUT_LIBS@
else
shout_plugins =
endif

else
theoraenc_plugins =
theora_sources = 
endif

if HAVE_SPEEX
speex_plugins = e_speex.la c_speexenc.la
speex_sources = speex.c
else
speex_plugins =
speex_sources =
endif

if HAVE_SCHROEDINGER
schroedinger_sources = schroedinger.c
schroedinger_plugins = c_schroedingerenc.la
else
schroedinger_sources =
schroedinger_plugins =
endif


if HAVE_OPUS
opus_plugins = e_opus.la c_opusenc.la
opus_sources = opus.c
else
opus_plugins =
opus_sources =
endif

if HAVE_FLAC
flac_plugins = e_flacogg.la c_flacenc.la
flac_sources = flac.c
bgflac_libs    = $(top_builddir)/lib/libbgflac.la @FLAC_LIBS@
else
flac_plugins =
flac_sources =
endif

if HAVE_OGG_VIDEO
oggvideo_plugins = e_oggvideo.la
else
oggvideo_plugins =
endif

gmerlin_plugin_LTLIBRARIES = \
e_vorbis.la \
c_vorbisenc.la \
$(theoraenc_plugins) \
$(speex_plugins) \
$(opus_plugins) \
$(flac_plugins) \
$(shout_plugins) \
$(oggvideo_plugins) \
$(schroedinger_plugins)


AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

e_vorbis_la_CFLAGS = @VORBIS_CFLAGS@ $(AM_CFLAGS)
e_vorbis_la_SOURCES = e_vorbis.c vorbis.c ogg_common.c
e_vorbis_la_LIBADD = @GMERLIN_DEP_LIBS@ $(top_builddir)/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ 

e_flacogg_la_CFLAGS = @OGG_CFLAGS@ @FLAC_CFLAGS@ $(AM_CFLAGS)
e_flacogg_la_SOURCES = e_flacogg.c flac.c ogg_common.c
e_flacogg_la_LIBADD = @GMERLIN_DEP_LIBS@ $(top_builddir)/lib/libgmerlin_encoders.la $(bgflac_libs) @OGG_LIBS@

e_theora_la_CFLAGS = \
@THEORAENC_CFLAGS@ \
@VORBIS_CFLAGS@ \
@SPEEX_CFLAGS@ \
@FLAC_CFLAGS@ \
@OPUS_CFLAGS@ \
$(AM_CFLAGS)

e_theora_la_SOURCES = \
e_theora.c \
vorbis.c \
$(speex_sources) \
$(flac_sources) \
$(opus_sources) \
theora.c \
ogg_common.c

e_theora_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@VORBISENC_LIBS@ \
@VORBIS_LIBS@ \
@THEORAENC_LIBS@ \
@THEORADEC_LIBS@ \
@SPEEX_LIBS@ \
@OPUS_LIBS@ \
$(bgflac_libs)

e_oggvideo_la_CFLAGS = \
@THEORAENC_CFLAGS@ \
@VORBIS_CFLAGS@ \
@SPEEX_CFLAGS@ \
@FLAC_CFLAGS@ \
@OPUS_CFLAGS@ \
@SCHROEDINGER_CFLAGS@ \
$(AM_CFLAGS)

e_oggvideo_la_SOURCES = \
e_oggvideo.c \
vorbis.c \
$(speex_sources) \
$(flac_sources) \
$(opus_sources) \
$(theora_sources) \
$(schroedinger_sources) \
ogg_common.c

e_oggvideo_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@VORBISENC_LIBS@ \
@VORBIS_LIBS@ \
@THEORAENC_LIBS@ \
@THEORADEC_LIBS@ \
@SPEEX_LIBS@ \
@OPUS_LIBS@ \
@SCHROEDINGER_LIBS@ \
$(bgflac_libs)


b_ogg_la_CFLAGS = @THEORAENC_CFLAGS@ @THEORADEC_LIBS@ @VORBIS_CFLAGS@ @SPEEX_CFLAGS@  $(AM_CFLAGS)
b_ogg_la_SOURCES = b_ogg.c vorbis.c $(speex_sources) theora.c ogg_common.c
b_ogg_la_LIBADD = @GMERLIN_DEP_LIBS@ $(top_builddir)/lib/libgmerlin_encoders.la @VORBISENC_LIBS@ @VORBIS_LIBS@ @THEORAENC_LIBS@ @THEORADEC_LIBS@ @SPEEX_LIBS@ $(bgshout_libs)

e_speex_la_CFLAGS = @SPEEX_CFLAGS@ $(AM_CFLAGS)
e_speex_la_SOURCES = e_speex.c speex.c ogg_common.c
e_speex_la_LIBADD = @GMERLIN_DEP_LIBS@ $(top_builddir)/lib/libgmerlin_encoders.la @SPEEX_LIBS@ @OGG_LIBS@ 

e_opus_la_CFLAGS = @OPUS_CFLAGS@ $(AM_CFLAGS)
e_opus_la_SOURCES = e_opus.c opus.c ogg_common.c
e_opus_la_LIBADD = @GMERLIN_DEP_LIBS@ $(top_builddir)/lib/libgmerlin_encoders.la @OPUS_LIBS@ @OGG_LIBS@ 


noinst_HEADERS = ogg_common.h

c_theoraenc_la_CFLAGS = \
@THEORAENC_CFLAGS@ \
$(AM_CFLAGS)

c_theoraenc_la_SOURCES = \
theora.c \
c_theoraenc.c \
ogg_common.c

c_theoraenc_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@THEORAENC_LIBS@ \
@THEORADEC_LIBS@


c_schroedingerenc_la_CFLAGS = \
@SCHROEDINGER_CFLAGS@ \
$(AM_CFLAGS)

c_schroedingerenc_la_SOURCES = \
schroedinger.c \
c_schroedingerenc.c \
ogg_common.c

c_schroedingerenc_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@SCHROEDINGER_LIBS@ \
@OGG_LIBS@


c_vorbisenc_la_CFLAGS = \
@VORBIS_CFLAGS@ $(AM_CFLAGS)

c_vorbisenc_la_SOURCES = \
vorbis.c \
c_vorbisenc.c \
ogg_common.c

c_vorbisenc_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@VORBISENC_LIBS@ \
@VORBIS_LIBS@ 


c_speexenc_la_CFLAGS = \
@SPEEX_CFLAGS@ $(AM_CFLAGS)

c_speexenc_la_SOURCES = \
speex.c \
c_speexenc.c \
ogg_common.c

c_speexenc_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@SPEEX_LIBS@ \
@OGG_LIBS@


c_opusenc_la_CFLAGS = \
@OPUS_CFLAGS@ $(AM_CFLAGS)

c_opusenc_la_SOURCES = \
opus.c \
c_opusenc.c \
ogg_common.c

c_opusenc_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
@OPUS_LIBS@ \
@OGG_LIBS@

#

c_flacenc_la_CFLAGS = \
@FLAC_CFLAGS@ $(AM_CFLAGS)

c_flacenc_la_SOURCES = \
flac.c \
c_flacenc.c \
ogg_common.c

c_flacenc_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(top_builddir)/lib/libgmerlin_encoders.la \
$(bgflac_libs) \
@FLAC_LIBS@ \
@OGG_LIBS@


EXTRA_c_vorbisenc_la_SOURCES = \
_codec_plugin.c
