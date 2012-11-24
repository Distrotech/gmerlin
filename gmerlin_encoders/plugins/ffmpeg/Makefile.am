gmerlin_plugindir = @gmerlin_plugindir@

INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS = @GMERLIN_DEP_CFLAGS@ @AVFORMAT_CFLAGS@ @AVCODEC_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

gmerlin_plugin_LTLIBRARIES = \
e_ffmpeg_video.la \
e_ffmpeg_audio.la \
e_ffmpeg.la \
c_ffmpeg_mpeg4.la \
c_ffmpeg_x264.la \
c_ffmpeg_mp2.la

common_sources = ffmpeg_common.c codecs.c codec.c

e_ffmpeg_video_la_SOURCES = e_ffmpeg_video.c $(common_sources)
e_ffmpeg_video_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

e_ffmpeg_audio_la_SOURCES = e_ffmpeg_audio.c $(common_sources)
e_ffmpeg_audio_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

e_ffmpeg_la_SOURCES = e_ffmpeg.c $(common_sources)
e_ffmpeg_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_mpeg4_la_SOURCES = c_ffmpeg_mpeg4.c codecs.c codec.c ffmpeg_common.c
c_ffmpeg_mpeg4_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_x264_la_SOURCES = c_ffmpeg_x264.c codecs.c codec.c ffmpeg_common.c
c_ffmpeg_x264_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_mp2_la_SOURCES = c_ffmpeg_mp2.c codecs.c codec.c ffmpeg_common.c
c_ffmpeg_mp2_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

noinst_HEADERS = ffmpeg_common.h params.h
