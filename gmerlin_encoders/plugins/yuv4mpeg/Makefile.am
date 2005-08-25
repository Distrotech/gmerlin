libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @MJPEGTOOLS_CFLAGS@

lib_LTLIBRARIES = e_yuv4mpeg.la e_mpegvideo.la

e_yuv4mpeg_la_SOURCES  = e_yuv4mpeg.c y4m_common.c
e_yuv4mpeg_la_LDFLAGS = -avoid-version -module
e_yuv4mpeg_la_LIBADD = @MJPEGTOOLS_LIBS@


e_mpegvideo_la_SOURCES  = e_mpegvideo.c y4m_common.c mpv_common.c
e_mpegvideo_la_LDFLAGS = -avoid-version -module
e_mpegvideo_la_LIBADD = @MJPEGTOOLS_LIBS@

noinst_HEADERS = y4m_common.h mpv_common.h

