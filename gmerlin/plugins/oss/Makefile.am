libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = oa_oss.la i_oss.la

oa_oss_la_SOURCES = oa_oss.c oss_common.c
i_oss_la_SOURCES  = i_oss.c oss_common.c

noinst_HEADERS = oss_common.h
