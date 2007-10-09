libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS = @JPEG_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = ir_jpeg.la iw_jpeg.la

ir_jpeg_la_SOURCES = ir_jpeg.c
ir_jpeg_la_LIBADD = @GMERLIN_DEP_LIBS@ @JPEG_LIBS@

iw_jpeg_la_SOURCES = iw_jpeg.c
iw_jpeg_la_LIBADD = @GMERLIN_DEP_LIBS@ @JPEG_LIBS@
