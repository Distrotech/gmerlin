libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = e_pp_vcdimager.la e_pp_cdrdao.la

e_pp_vcdimager_la_SOURCES  = e_pp_vcdimager.c cdrdao_common.c

e_pp_vcdimager_la_LDFLAGS = -avoid-version -module

e_pp_cdrdao_la_SOURCES  = e_pp_cdrdao.c cdrdao_common.c
e_pp_cdrdao_la_LDFLAGS = -avoid-version -module

noinst_HEADERS = cdrdao_common.h
