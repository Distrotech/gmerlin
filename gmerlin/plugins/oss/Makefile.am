libdir=@GMERLIN_PLUGIN_DIR@

CFLAGS = @CFLAGS@
LIBS = @LIBS@

lib_LTLIBRARIES = oa_oss.la i_oss.la

oa_oss_la_SOURCES = oa_oss.c oss_common.c
i_oss_la_SOURCES  = i_oss.c oss_common.c

oa_oss_la_LDFLAGS = -avoid-version -module
i_oss_la_LDFLAGS  = -avoid-version -module

noinst_HEADERS = oss_common.h
