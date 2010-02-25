INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
@JPEG_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = ir_jpeg.la iw_jpeg.la

ir_jpeg_la_SOURCES = ir_jpeg.c
ir_jpeg_la_LIBADD = @GMERLIN_DEP_LIBS@ @JPEG_LIBS@ @MODULE_LIBADD@

iw_jpeg_la_SOURCES = iw_jpeg.c
iw_jpeg_la_LIBADD = @GMERLIN_DEP_LIBS@ @JPEG_LIBS@ @MODULE_LIBADD@
