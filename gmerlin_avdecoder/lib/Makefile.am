libdir=@GMERLIN_LIB_DIR@

lib_LTLIBRARIES = libgmerlin_avdec.la

noinst_HEADERS = pnm.h

if HAVE_VORBIS
vorbis_cflags = @VORBIS_CFLAGS@
vorbis_libs =   @VORBIS_LIBS@
vorbis_sources = audio_vorbis.c demux_ogg.c
else
vorbis_cflags =
vorbis_libs =
vorbis_sources =
endif

if HAVE_LIBA52
a52_cflags = @LIBA52_CFLAGS@
a52_libs =   @LIBA52_LIBS@
a52_sources = audio_a52.c
else
a52_cflags =
a52_libs =
a52_sources =
endif

if HAVE_FAAD2
faad2_cflags = @FAAD2_CFLAGS@
faad2_libs =   @FAAD2_LIBS@
faad2_sources = audio_faad2.c
else
faad2_cflags =
faad2_libs = 
faad2_sources = 
endif

if HAVE_LIBPNG
png_cflags =  @PNG_CFLAGS@
png_libs =    @PNG_LIBS@
png_sources = video_png.c
else
png_cflags =
png_libs =
png_sources =
endif

if HAVE_LIBMPEG2
libmpeg2_cflags =  @LIBMPEG2_CFLAGS@
libmpeg2_libs =    @LIBMPEG2_LIBS@
libmpeg2_sources = video_libmpeg2.c
else
libmpeg2_cflags =
libmpeg2_libs =
libmpeg2_sources =
endif

if HAVE_FLAC
flac_cflags =  @FLAC_CFLAGS@
flac_libs =    @FLAC_LIBS@
flac_sources = audio_flac.c
else
flac_cflags =
flac_libs =
flac_sources =
endif

if HAVE_MAD
mad_cflags =  @MAD_CFLAGS@
mad_libs =    @MAD_LIBS@
mad_sources = audio_mad.c
else
mad_cflags =
mad_libs =
mad_sources =
endif


if HAVE_W32DLL
win32_subdirs = libw32dll
win32_libs = \
libw32dll/DirectShow/libds_filter.la \
libw32dll/dmo/libdmo_filter.la \
libw32dll/wine/libwine.la \
-lpthread
win32_sources = video_win32.c audio_qtwin32.c audio_win32.c
else
win32_subdirs =
win32_libs =
win32_sources =
endif


SUBDIRS = $(win32_subdirs) real_rtsp

libgmerlin_avdec_la_LDFLAGS=-export-dynamic
libgmerlin_avdec_la_LIBADD= \
$(vorbis_libs) \
$(libmpeg2_libs) \
$(win32_libs) \
$(png_libs) \
$(faad2_libs) \
$(flac_libs) \
$(mad_libs) \
$(a52_libs) \
real_rtsp/librealrtsp.la \
-lm

AM_CFLAGS = \
$(vorbis_cflags) \
$(mad_cflags) \
$(png_cflags) \
$(faad2_cflags) \
$(flac_cflags) \
$(libmpeg2_cflags) \
$(a52_cflags)

libgmerlin_avdec_la_SOURCES = \
$(libmpeg2_sources) \
$(vorbis_sources) \
$(faad2_sources) \
$(png_sources) \
$(win32_sources) \
$(mad_sources) \
$(a52_sources) \
$(flac_sources) \
audio.c \
audio_ffmpeg.c \
audio_real.c \
audio_aiff.c \
base64.c \
bgav.c \
charset.c \
codecs.c \
demuxer.c \
demux_aiff.c \
demux_asf.c \
demux_avi.c \
demux_au.c \
demux_flac.c \
demux_mpegaudio.c \
demux_mpegps.c \
demux_quicktime.c \
demux_ra.c \
demux_rm.c \
demux_wav.c \
http.c \
id3v1.c \
id3v2.c \
in_fd.c \
in_file.c \
in_http.c \
in_memory.c \
in_mms.c \
in_pnm.c \
in_realrtsp.c \
in_vcd.c \
input.c \
metadata.c \
mms.c \
nanosoft.c \
packetbuffer.c \
packet.c \
pes_header.c \
pnm.c \
qt_atom.c \
qt_cmov.c \
qt_esds.c \
qt_hdlr.c \
qt_mdhd.c \
qt_mdia.c \
qt_minf.c \
qt_moov.c \
qt_mvhd.c \
qt_palette.c \
qt_rdrf.c \
qt_rmda.c \
qt_rmra.c \
qt_stbl.c \
qt_stco.c \
qt_stsc.c \
qt_stsd.c \
qt_stss.c \
qt_stsz.c \
qt_stts.c \
qt_tkhd.c \
qt_trak.c \
qt_udta.c \
qt_utils.c \
redirect.c \
r_asx.c \
r_m3u.c \
r_pls.c \
r_ref.c \
sdp.c \
stream.c \
rtsp.c \
tcp.c \
track.c \
tracktable.c \
utils.c \
video.c \
video_ffmpeg.c \
video_xadll.c \
video_qtraw.c \
video_qtrle.c \
video_real.c \
xing.c \
yml.c

