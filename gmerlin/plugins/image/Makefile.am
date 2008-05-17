INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = ir_tga.la iw_tga.la ir_bmp.la iw_bmp.la ir_pnm.la iw_pnm.la

ir_tga_la_SOURCES = ir_tga.c targa.c

iw_tga_la_SOURCES = iw_tga.c targa.c

ir_bmp_la_SOURCES = ir_bmp.c

iw_bmp_la_SOURCES = iw_bmp.c

ir_pnm_la_SOURCES = ir_pnm.c

iw_pnm_la_SOURCES = iw_pnm.c


noinst_HEADERS = targa.h
