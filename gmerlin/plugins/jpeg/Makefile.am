libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @JPEG_CFLAGS@

lib_LTLIBRARIES = ir_jpeg.la iw_jpeg.la

ir_jpeg_la_SOURCES = ir_jpeg.c
ir_jpeg_la_LDFLAGS  = -avoid-version -module
ir_jpeg_la_LIBADD = @JPEG_LIBS@

iw_jpeg_la_SOURCES = iw_jpeg.c
iw_jpeg_la_LDFLAGS  = -avoid-version -module
iw_jpeg_la_LIBADD = @JPEG_LIBS@
