libdir=@GMERLIN_PLUGIN_DIR@

CFLAGS = @CFLAGS@ @LQT_CFLAGS@
LIBS = @LIBS@ @LQT_LIBS@

lib_LTLIBRARIES = e_lqt.la i_lqt.la

e_lqt_la_SOURCES  = e_lqt.c lqt_common.c
i_lqt_la_SOURCES  = i_lqt.c lqt_common.c

e_lqt_la_LDFLAGS = -avoid-version -module
i_lqt_la_LDFLAGS  = -avoid-version -module

noinst_HEADERS = lqt_common.h
