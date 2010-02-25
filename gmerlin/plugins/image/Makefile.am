INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = ir_tga.la iw_tga.la ir_bmp.la iw_bmp.la ir_pnm.la iw_pnm.la

ir_tga_la_SOURCES = ir_tga.c targa.c
ir_tga_la_LIBADD = @MODULE_LIBADD@

iw_tga_la_SOURCES = iw_tga.c targa.c
iw_tga_la_LIBADD = @MODULE_LIBADD@

ir_bmp_la_SOURCES = ir_bmp.c
ir_bmp_la_LIBADD = @MODULE_LIBADD@

iw_bmp_la_SOURCES = iw_bmp.c
iw_bmp_la_LIBADD = @MODULE_LIBADD@

ir_pnm_la_SOURCES = ir_pnm.c
ir_pnm_la_LIBADD = @MODULE_LIBADD@

iw_pnm_la_SOURCES = iw_pnm.c
iw_pnm_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = targa.h
