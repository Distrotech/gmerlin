libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

if HAVE_MJPEGTOOLS
yuvdeinterlace_sources = yuvdeinterlace.c
mjpeg_cflags = @MJPEGTOOLS_CFLAGS@
mjpeg_libs   = @MJPEGTOOLS_LIBS@
else
yuvdeinterlace_sources =
mjpeg_cflags = 
mjpeg_libs   = 
endif

lib_LTLIBRARIES = \
fv_blur.la \
fv_cropscale.la \
fv_decimate.la \
fv_deinterlace.la \
fv_equalizer.la \
fv_flip.la \
fv_pixelformat.la \
fv_tlp.la \
fv_zoom.la

fv_blur_la_SOURCES  = fv_blur.c

fv_pixelformat_la_SOURCES  = fv_pixelformat.c

fv_tlp_la_SOURCES  = fv_tlp.c

fv_cropscale_la_SOURCES  = fv_cropscale.c


fv_flip_la_SOURCES  = fv_flip.c

fv_decimate_la_SOURCES  = fv_decimate.c

fv_deinterlace_la_CFLAGS   = $(mjpeg_cflags) -DLOCALE_DIR=\"$(localedir)\"
fv_deinterlace_la_LIBADD   = $(mjpeg_libs)
fv_deinterlace_la_SOURCES  = fv_deinterlace.c $(yuvdeinterlace_sources)

fv_equalizer_la_SOURCES  = fv_equalizer.c

fv_zoom_la_SOURCES  = fv_zoom.c

noinst_HEADERS = motionsearch.h
