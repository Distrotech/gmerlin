INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
@TIFF_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

plugin_LTLIBRARIES = ir_tiff.la iw_tiff.la c_tiffenc.la

ir_tiff_la_SOURCES = ir_tiff.c
ir_tiff_la_LIBADD = @GMERLIN_DEP_LIBS@ @TIFF_LIBS@ @MODULE_LIBADD@

c_tiffenc_la_SOURCES = c_tiffenc.c tiffwriter.c
c_tiffenc_la_LIBADD = @GMERLIN_DEP_LIBS@ @TIFF_LIBS@ @MODULE_LIBADD@

iw_tiff_la_SOURCES = iw_tiff.c tiffwriter.c
iw_tiff_la_LIBADD = @GMERLIN_DEP_LIBS@ @TIFF_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = tiffwriter.h