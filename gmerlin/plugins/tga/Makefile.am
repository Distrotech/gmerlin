libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = ir_tga.la iw_tga.la

ir_tga_la_SOURCES = ir_tga.c targa.c
ir_tga_la_LDFLAGS  = -avoid-version -module
ir_tga_la_LIBADD = @TGA_LIBS@

iw_tga_la_SOURCES = iw_tga.c targa.c
iw_tga_la_LDFLAGS  = -avoid-version -module

noinst_HEADERS = targa.h
