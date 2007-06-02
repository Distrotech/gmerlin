
lib_LTLIBRARIES = libgmerlin_avdec.la

noinst_HEADERS = pnm.h targa.h

if HAVE_VORBIS
vorbis_cflags = @VORBIS_CFLAGS@
vorbis_libs =   @VORBIS_LIBS@ @VORBISENC_LIBS@
vorbis_sources = audio_vorbis.c demux_ogg.c
else
vorbis_cflags =
vorbis_libs =
vorbis_sources =
endif

if HAVE_LIBA52
a52_cflags = @LIBA52_CFLAGS@
a52_libs =   @LIBA52_LIBS@
a52_sources = audio_a52.c demux_a52.c
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
png_sources = video_png.c pngreader.c
else
png_cflags =
png_libs =
png_sources =
endif

if HAVE_LIBTIFF
tiff_cflags =  @TIFF_CFLAGS@
tiff_libs =    @TIFF_LIBS@
tiff_sources = video_tiff.c
else
tiff_cflags =
tiff_libs =
tiff_sources =
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

if HAVE_DCA
dca_cflags =  @DCA_CFLAGS@
dca_libs =    @DCA_LIBS@
dca_sources = audio_dca.c
else
dca_cflags =
dca_libs =
dca_sources =
endif


if HAVE_MUSEPACK
musepack_cflags =  @MUSEPACK_CFLAGS@
musepack_libs =    @MUSEPACK_LIBS@
musepack_sources = demux_musepack.c
else
musepack_cflags =
musepack_libs =
musepack_sources =
endif


if HAVE_LIBAVCODEC
avcodec_cflags =  @AVCODEC_CFLAGS@
avcodec_libs =    @AVCODEC_LIBS@
avcodec_sources = audio_ffmpeg.c video_ffmpeg.c
else
avcodec_cflags =
avcodec_libs =
avcodec_sources =
endif

if HAVE_LIBPOSTPROC
libpostproc_cflags =  @LIBPOSTPROC_CFLAGS@
libpostproc_libs =    @LIBPOSTPROC_LIBS@
else
libpostproc_cflags =
libpostproc_libs =
endif


if HAVE_CDIO
cdio_cflags =  @CDIO_CFLAGS@
cdio_libs =    @CDIO_LIBS@

if HAVE_DVDREAD
dvdread_cflags  = @DVDREAD_CFLAGS@
dvdread_libs    = @DVDREAD_LIBS@
else
dvdread_cflags  =
dvdread_libs    =
endif

else
cdio_cflags =
cdio_libs =
dvdread_cflags  =
dvdread_libs    =
endif


if HAVE_MJPEGTOOLS
mjpegtools_cflags =  @MJPEGTOOLS_CFLAGS@
mjpegtools_libs =    @MJPEGTOOLS_LIBS@
mjpegtools_sources = demux_y4m.c
else
mjpegtools_cflags =
mjpegtools_libs =
mjpegtools_sources =
endif

if HAVE_SAMBA
samba_cflags =  @SAMBA_CFLAGS@
samba_libs =    @SAMBA_LIBS@
samba_sources = in_smb.c
else
samba_cflags =
samba_libs =
samba_sources =
endif

if HAVE_THEORA
theora_cflags  = @THEORA_CFLAGS@
theora_libs    = @THEORA_LIBS@
theora_sources = video_theora.c
else
theora_cflags  =
theora_libs    =
theora_sources =
endif

if HAVE_SPEEX
speex_cflags  = @SPEEX_CFLAGS@
speex_libs    = @SPEEX_LIBS@
speex_sources = audio_speex.c
else
speex_cflags  =
speex_libs    =
speex_sources =
endif

if HAVE_W32DLL
win32_subdirs = libw32dll
win32_libs = \
libw32dll/DirectShow/libds_filter.la \
libw32dll/dmo/libdmo_filter.la \
libw32dll/wine/libwine.la \
-lpthread
win32_sources = video_win32.c audio_qtwin32.c audio_win32.c win32codec.c
else
win32_subdirs =
win32_libs =
win32_sources =
endif

if HAVE_REALDLL
real_sources = audio_real.c video_real.c
real_libs = @REALDLL_LIBS@
else
real_sources =
real_libs =
endif

if HAVE_XADLL
xadll_sources = video_xadll.c
xadll_libs = @XADLL_LIBS@
else
xadll_sources =
xadll_libs = 
endif

if HAVE_LINUXDVB
dvb_sources = dvb_channels.c
else 	 
dvb_sources = 	 
endif	

SUBDIRS = $(win32_subdirs) GSM610

libgmerlin_avdec_la_LDFLAGS=-export-dynamic
libgmerlin_avdec_la_LIBADD= \
@GMERLIN_DEP_LIBS@ \
$(vorbis_libs) \
$(libmpeg2_libs) \
$(win32_libs) \
$(png_libs) \
$(tiff_libs) \
$(dvdread_libs) \
$(faad2_libs) \
$(flac_libs) \
$(mad_libs) \
$(dca_libs) \
$(musepack_libs) \
$(a52_libs) \
$(avcodec_libs) \
$(real_libs) \
$(xadll_libs) \
$(cdio_libs) \
$(mjpegtools_libs) \
$(samba_libs) \
$(theora_libs) \
$(speex_libs) \
$(libpostproc_libs) \
GSM610/libgsm.la \
@ICONV_LIBS@ \
@LIBINTL@ \
-lm

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
@OPT_CFLAGS@ \
$(vorbis_cflags) \
$(mad_cflags) \
$(png_cflags) \
$(tiff_cflags) \
$(faad2_cflags) \
$(flac_cflags) \
$(libmpeg2_cflags) \
$(a52_cflags) \
$(dca_cflags) \
$(musepack_cflags) \
$(avcodec_cflags) \
$(cdio_cflags) \
$(mjpegtools_cflags) \
$(samba_cflags) \
$(theora_cflags) \
$(speex_cflags) \
$(libpostproc_cflags)

libgmerlin_avdec_la_SOURCES = \
$(libmpeg2_sources) \
$(vorbis_sources) \
$(faad2_sources) \
$(png_sources) \
$(tiff_sources) \
$(win32_sources) \
$(mad_sources) \
$(musepack_sources) \
$(a52_sources) \
$(dca_sources) \
$(flac_sources) \
$(avcodec_sources) \
$(real_sources) \
$(xadll_sources) \
$(mjpegtools_sources) \
$(samba_sources) \
$(theora_sources) \
$(speex_sources) \
$(dvb_sources) \
RTjpeg.c \
a52_header.c \
apetag.c \
audio.c \
audio_gavl.c \
audio_gsm.c \
audio_pcm.c \
asmrp.c \
base64.c \
bgav.c \
bytebuffer.c \
chapterlist.c \
charset.c \
codecs.c \
demuxer.c \
demux_4xm.c \
demux_8svx.c \
demux_aac.c \
demux_aiff.c \
demux_asf.c \
demux_avi.c \
demux_avs.c \
demux_au.c \
demux_daud.c \
demux_dsicin.c \
demux_dv.c \
demux_dxa.c \
demux_flac.c \
demux_fli.c \
demux_flv.c \
demux_gif.c \
demux_gsm.c \
demux_gxf.c \
demux_ircam.c \
demux_sphere.c \
demux_mpegaudio.c \
demux_mpegps.c \
demux_mpegts.c \
demux_mpegvideo.c \
demux_mtv.c \
demux_nsv.c \
demux_nuv.c \
demux_psxstr.c \
demux_quicktime.c \
demux_ra.c \
demux_rm.c \
demux_roq.c \
demux_shorten.c \
demux_smacker.c \
demux_smaf.c \
demux_smjpeg.c \
demux_sol.c \
demux_tiertex.c \
demux_tta.c \
demux_vivo.c \
demux_voc.c \
demux_vmd.c \
demux_vqa.c \
demux_wav.c \
demux_wavpack.c \
demux_wve.c \
device.c \
dvframe.c \
http.c \
id3v1.c \
id3v2.c \
in_dvb_linux.c \
in_dvd.c \
in_fd.c \
in_file.c \
in_ftp.c \
in_http.c \
in_memory.c \
in_mms.c \
in_pnm.c \
in_rtsp.c \
in_vcd.c \
input.c \
languages.c \
log.c \
metadata.c \
mpegts_common.c \
mms.c \
nanosoft.c \
options.c \
packetbuffer.c \
packet.c \
pes_header.c \
pnm.c \
qt_atom.c \
qt_chan.c \
qt_cmov.c \
qt_dinf.c \
qt_dref.c \
qt_edts.c \
qt_elst.c \
qt_enda.c \
qt_esds.c \
qt_fiel.c \
qt_frma.c \
qt_ftab.c \
qt_gmhd.c \
qt_gmin.c \
qt_hdlr.c \
qt_language.c \
qt_mdhd.c \
qt_mdia.c \
qt_minf.c \
qt_moov.c \
qt_mvhd.c \
qt_nmhd.c \
qt_palette.c \
qt_pasp.c \
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
qt_tref.c \
qt_udta.c \
qt_utils.c \
qt_wave.c \
redirect.c \
rmff.c \
r_asx.c \
r_m3u.c \
r_pls.c \
r_ref.c \
r_smil.c \
rtsp.c \
sdp.c \
stream.c \
subovl_dvd.c \
subread.c \
subtitle.c \
superindex.c \
targa.c \
tcp.c \
track.c \
tracktable.c \
translation.c \
utils.c \
video.c \
video_gavl.c \
video_qtraw.c \
video_aviraw.c \
video_tga.c \
video_yuv.c \
video_rtjpeg.c \
vorbis_comment.c \
xing.c \
yml.c

