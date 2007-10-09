libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = e_pp_vcdimager.la e_pp_cdrdao.la

e_pp_vcdimager_la_SOURCES  = e_pp_vcdimager.c cdrdao_common.c

e_pp_cdrdao_la_SOURCES  = e_pp_cdrdao.c cdrdao_common.c
e_pp_cdrdao_la_LDFLAGS = -avoid-version -module

noinst_HEADERS = cdrdao_common.h
