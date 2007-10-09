libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = @TIFF_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = ir_tiff.la iw_tiff.la

ir_tiff_la_SOURCES = ir_tiff.c
ir_tiff_la_LIBADD = @GMERLIN_DEP_LIBS@ @TIFF_LIBS@

iw_tiff_la_SOURCES = iw_tiff.c
iw_tiff_la_LIBADD = @GMERLIN_DEP_LIBS@ @TIFF_LIBS@
