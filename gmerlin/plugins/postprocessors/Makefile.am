INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = e_pp_vcdimager.la e_pp_cdrdao.la

e_pp_vcdimager_la_SOURCES  = e_pp_vcdimager.c cdrdao_common.c
e_pp_vcdimager_la_LIBADD = @MODULE_LIBADD@

e_pp_cdrdao_la_SOURCES  = e_pp_cdrdao.c cdrdao_common.c
e_pp_cdrdao_la_LDFLAGS = -avoid-version -module
e_pp_cdrdao_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = cdrdao_common.h
