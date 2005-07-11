libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = ir_tga.la iw_tga.la ir_bmp.la iw_bmp.la ir_pnm.la iw_pnm.la

ir_tga_la_SOURCES = ir_tga.c targa.c
ir_tga_la_LDFLAGS  = -avoid-version -module

iw_tga_la_SOURCES = iw_tga.c targa.c
iw_tga_la_LDFLAGS  = -avoid-version -module

ir_bmp_la_SOURCES = ir_bmp.c
ir_bmp_la_LDFLAGS  = -avoid-version -module

iw_bmp_la_SOURCES = iw_bmp.c
iw_bmp_la_LDFLAGS  = -avoid-version -module

ir_pnm_la_SOURCES = ir_pnm.c
ir_pnm_la_LDFLAGS  = -avoid-version -module

iw_pnm_la_SOURCES = iw_pnm.c
iw_pnm_la_LDFLAGS  = -avoid-version -module


noinst_HEADERS = targa.h
