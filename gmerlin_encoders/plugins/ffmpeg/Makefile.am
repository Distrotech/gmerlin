gmerlin_plugindir = @gmerlin_plugindir@

INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS = @GMERLIN_DEP_CFLAGS@ @AVFORMAT_CFLAGS@ @AVCODEC_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

gmerlin_plugin_LTLIBRARIES = \
e_ffmpeg_video.la \
e_ffmpeg_audio.la \
e_ffmpeg.la \
c_ffmpeg_ac3.la \
c_ffmpeg_alaw.la \
c_ffmpeg_ulaw.la \
c_ffmpeg_mpeg4.la \
c_ffmpeg_x264.la \
c_ffmpeg_mp2.la \
c_ffmpeg_jpeg.la \
c_ffmpeg_mpeg1.la \
c_ffmpeg_mpeg2.la \
c_ffmpeg_tga.la

common_sources = ffmpeg_common.c codecs.c codec.c

codec_sources = codecs.c codec.c

e_ffmpeg_video_la_SOURCES = e_ffmpeg_video.c $(common_sources)
e_ffmpeg_video_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

e_ffmpeg_audio_la_SOURCES = e_ffmpeg_audio.c $(common_sources)
e_ffmpeg_audio_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

e_ffmpeg_la_SOURCES = e_ffmpeg.c $(common_sources)
e_ffmpeg_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_mpeg4_la_SOURCES = c_ffmpeg_mpeg4.c $(codec_sources)
c_ffmpeg_mpeg4_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_x264_la_SOURCES = c_ffmpeg_x264.c $(codec_sources)
c_ffmpeg_x264_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_mp2_la_SOURCES = c_ffmpeg_mp2.c $(codec_sources)
c_ffmpeg_mp2_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_ac3_la_SOURCES = c_ffmpeg_ac3.c $(codec_sources)
c_ffmpeg_ac3_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_alaw_la_SOURCES = c_ffmpeg_alaw.c $(codec_sources)
c_ffmpeg_alaw_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_ulaw_la_SOURCES = c_ffmpeg_ulaw.c $(codec_sources)
c_ffmpeg_ulaw_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_jpeg_la_SOURCES = c_ffmpeg_jpeg.c $(codec_sources)
c_ffmpeg_jpeg_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_mpeg1_la_SOURCES = c_ffmpeg_mpeg1.c $(codec_sources)
c_ffmpeg_mpeg1_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_mpeg2_la_SOURCES = c_ffmpeg_mpeg2.c $(codec_sources)
c_ffmpeg_mpeg2_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

c_ffmpeg_tga_la_SOURCES = c_ffmpeg_tga.c $(codec_sources)
c_ffmpeg_tga_la_LIBADD  = @GMERLIN_DEP_LIBS@ @AVFORMAT_LIBS@

noinst_HEADERS = ffmpeg_common.h params.h
