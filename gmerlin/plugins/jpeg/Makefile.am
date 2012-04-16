INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
@JPEG_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

if HAVE_LIBEXIF
exif_sources = exif.c
exif_libs = @LIBEXIF_LIBS@
exif_cflags = @LIBEXIF_CFLAGS@
else
exif_sources = 
exif_libs = 
exif_cflags = 
endif

plugin_LTLIBRARIES = ir_jpeg.la iw_jpeg.la

noinst_HEADERS = exif.h

ir_jpeg_la_SOURCES = ir_jpeg.c $(exif_sources)
ir_jpeg_la_LIBADD = @GMERLIN_DEP_LIBS@ @JPEG_LIBS@ @MODULE_LIBADD@ $(exif_libs)

iw_jpeg_la_SOURCES = iw_jpeg.c
iw_jpeg_la_LIBADD = @GMERLIN_DEP_LIBS@ @JPEG_LIBS@ @MODULE_LIBADD@
