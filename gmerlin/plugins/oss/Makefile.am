INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = oa_oss.la i_oss.la

oa_oss_la_SOURCES = oa_oss.c oss_common.c
oa_oss_la_LIBADD = @MODULE_LIBADD@

i_oss_la_SOURCES  = i_oss.c oss_common.c
i_oss_la_LIBADD  = @MODULE_LIBADD@

noinst_HEADERS = oss_common.h
