libdir=@GMERLIN_LIB_DIR@

lib_LTLIBRARIES = libgmerlin_avdec.la

if HAVE_VORBIS
vorbis_cflags = @VORBIS_CFLAGS@
vorbis_libs =   @VORBIS_LIBS@
vorbis_sources = audio_vorbis.c
else
vorbis_cflags =
vorbis_libs =
vorbis_sources =
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

if HAVE_W32DLL
win32_subdirs = libw32dll
win32_libs = \
libw32dll/DirectShow/libds_filter.la \
libw32dll/dmo/libdmo_filter.la \
libw32dll/wine/libwine.la \
-lpthread
win32_sources = video_win32.c audio_qtwin32.c
else
win32_subdirs =
win32_libs =
win32_sources =
endif

SUBDIRS = $(win32_subdirs) real_rtsp

libgmerlin_avdec_la_LDFLAGS=-export-dynamic
libgmerlin_avdec_la_LIBADD=$(vorbis_libs) $(win32_libs) $(png_libs) $(faad2_libs) real_rtsp/librealrtsp.la
CFLAGS = @CFLAGS@ $(vorbis_cflags) $(png_cflags) $(faad2_cflags)

libgmerlin_avdec_la_SOURCES = \
$(vorbis_sources) \
$(faad2_sources) \
$(png_sources) \
$(win32_sources) \
audio.c \
audio_ffmpeg.c \
audio_real.c \
audio_aiff.c \
bgav.c \
charset.c \
codecs.c \
demuxer.c \
demux_aiff.c \
demux_asf.c \
demux_avi.c \
demux_au.c \
demux_quicktime.c \
demux_ra.c \
demux_rm.c \
demux_wav.c \
in_file.c \
in_memory.c \
in_realrtsp.c \
input.c \
nanosoft.c \
packetbuffer.c \
packet.c \
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
tcp.c \
utils.c \
video.c \
video_ffmpeg.c \
video_xadll.c \
video_qtraw.c \
video_real.c

