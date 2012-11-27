INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
@PNG_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = ir_png.la iw_png.la e_spumux.la c_pngenc.la

ir_png_la_SOURCES = ir_png.c
ir_png_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

iw_png_la_SOURCES = iw_png.c pngwriter.c
iw_png_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

e_spumux_la_SOURCES = e_spumux.c pngwriter.c
e_spumux_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

c_pngenc_la_SOURCES = c_pngenc.c pngwriter.c
c_pngenc_la_LIBADD = @GMERLIN_DEP_LIBS@ @PNG_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = pngwriter.h
