libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

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
fv_blur_la_LDFLAGS = -avoid-version -module

fv_pixelformat_la_SOURCES  = fv_pixelformat.c
fv_pixelformat_la_LDFLAGS = -avoid-version -module

fv_tlp_la_SOURCES  = fv_tlp.c
fv_tlp_la_LDFLAGS = -avoid-version -module

fv_cropscale_la_SOURCES  = fv_cropscale.c
fv_cropscale_la_LDFLAGS = -avoid-version -module


fv_flip_la_SOURCES  = fv_flip.c
fv_flip_la_LDFLAGS = -avoid-version -module

fv_decimate_la_SOURCES  = fv_decimate.c
fv_decimate_la_LDFLAGS = -avoid-version -module

fv_deinterlace_la_SOURCES  = fv_deinterlace.c
fv_deinterlace_la_LDFLAGS = -avoid-version -module

fv_equalizer_la_SOURCES  = fv_equalizer.c
fv_equalizer_la_LDFLAGS = -avoid-version -module

fv_zoom_la_SOURCES  = fv_zoom.c
fv_zoom_la_LDFLAGS = -avoid-version -module
