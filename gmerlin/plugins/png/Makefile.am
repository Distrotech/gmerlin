libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @PNG_CFLAGS@

lib_LTLIBRARIES = ir_png.la iw_png.la

ir_png_la_SOURCES = ir_png.c
ir_png_la_LDFLAGS  = -avoid-version -module
ir_png_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@

iw_png_la_SOURCES = iw_png.c
iw_png_la_LDFLAGS  = -avoid-version -module
iw_png_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@
