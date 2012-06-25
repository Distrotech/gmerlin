INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"



plugin_LTLIBRARIES = \
fv_bitshift.la \
fv_blur.la \
fv_cropscale.la \
fv_colorbalance.la \
fv_colormatrix_rgb.la \
fv_colormatrix_yuv.la \
fv_decimate.la \
fv_deinterlace.la \
fv_equalizer.la \
fv_flip.la \
fv_framerate.la \
fv_interlace.la \
fv_invert_rgb.la \
fv_oldcolor.la \
fv_pixelformat.la \
fv_swapfields.la \
fv_tcdisplay.la \
fv_tctweak.la \
fv_textlogo.la \
fv_tlp.la \
fv_transform.la \
fv_zoom.la

fv_blur_la_SOURCES  = fv_blur.c
fv_blur_la_LIBADD = @MODULE_LIBADD@ @LIBM@

fv_colormatrix_rgb_la_SOURCES  = fv_colormatrix_rgb.c colormatrix.c
fv_colormatrix_rgb_la_LIBADD = @MODULE_LIBADD@

fv_colormatrix_yuv_la_SOURCES  = fv_colormatrix_yuv.c colormatrix.c
fv_colormatrix_yuv_la_LIBADD = @MODULE_LIBADD@

fv_invert_rgb_la_SOURCES  = fv_invert_rgb.c colormatrix.c
fv_invert_rgb_la_LIBADD = @MODULE_LIBADD@

fv_colorbalance_la_SOURCES  = fv_colorbalance.c colormatrix.c
fv_colorbalance_la_LIBADD = @MODULE_LIBADD@

fv_oldcolor_la_SOURCES  = fv_oldcolor.c colormatrix.c
fv_oldcolor_la_LIBADD = @MODULE_LIBADD@

fv_pixelformat_la_SOURCES  = fv_pixelformat.c
fv_pixelformat_la_LIBADD = @MODULE_LIBADD@

fv_tlp_la_SOURCES  = fv_tlp.c
fv_tlp_la_LIBADD = @MODULE_LIBADD@

fv_tcdisplay_la_SOURCES  = fv_tcdisplay.c
fv_tcdisplay_la_CFLAGS   = @FREETYPE_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"
fv_tcdisplay_la_LIBADD = @MODULE_LIBADD@

fv_textlogo_la_SOURCES  = fv_textlogo.c
fv_textlogo_la_CFLAGS   = @FREETYPE_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"
fv_textlogo_la_LIBADD = @MODULE_LIBADD@

fv_tctweak_la_SOURCES  = fv_tctweak.c
fv_tctweak_la_LIBADD = @MODULE_LIBADD@

fv_transform_la_SOURCES  = fv_transform.c
fv_transform_la_LIBADD = @MODULE_LIBADD@ @LIBM@

fv_cropscale_la_SOURCES  = fv_cropscale.c
fv_cropscale_la_LIBADD = @MODULE_LIBADD@

fv_framerate_la_SOURCES  = fv_framerate.c
fv_framerate_la_LIBADD = @MODULE_LIBADD@


fv_flip_la_SOURCES  = fv_flip.c
fv_flip_la_LIBADD = @MODULE_LIBADD@

fv_decimate_la_SOURCES  = fv_decimate.c
fv_decimate_la_LIBADD = @MODULE_LIBADD@

fv_deinterlace_la_CFLAGS   =  -DLOCALE_DIR=\"$(localedir)\"
fv_deinterlace_la_LIBADD   = @MODULE_LIBADD@
fv_deinterlace_la_SOURCES  = fv_deinterlace.c bgyadif.c

fv_equalizer_la_SOURCES  = fv_equalizer.c colormatrix.c
fv_equalizer_la_LIBADD = @MODULE_LIBADD@ @LIBM@

fv_zoom_la_SOURCES  = fv_zoom.c
fv_zoom_la_LIBADD = @MODULE_LIBADD@

fv_swapfields_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = bgyadif.h colormatrix.h deinterlace.h
