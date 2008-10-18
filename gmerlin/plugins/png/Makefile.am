INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = @PNG_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = ir_png.la iw_png.la e_spumux.la

ir_png_la_SOURCES = ir_png.c
ir_png_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

iw_png_la_SOURCES = iw_png.c pngwriter.c
iw_png_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

e_spumux_la_SOURCES = e_spumux.c pngwriter.c
e_spumux_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = pngwriter.h
