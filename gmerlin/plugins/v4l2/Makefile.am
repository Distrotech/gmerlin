INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = i_v4l2.la ov_v4l2.la

if HAVE_V4LVCONVERT
convert_sources = convert.c
convert_libs = @V4LCONVERT_LIBS@
else
convert_sources =
convert_libs =
endif

i_v4l2_la_SOURCES  = i_v4l2.c v4l2_common.c $(convert_sources)
i_v4l2_la_LIBADD  = @MODULE_LIBADD@ $(convert_libs)

ov_v4l2_la_SOURCES  = ov_v4l2.c v4l2_common.c $(convert_sources)
ov_v4l2_la_LIBADD  = @MODULE_LIBADD@ $(convert_libs)


noinst_HEADERS = v4l2_common.h convert.h

