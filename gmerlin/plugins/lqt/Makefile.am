libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @LQT_CFLAGS@

lib_LTLIBRARIES = e_lqt.la i_lqt.la

e_lqt_la_SOURCES  = e_lqt.c lqt_common.c lqtgavl.c
i_lqt_la_SOURCES  = i_lqt.c lqt_common.c lqtgavl.c

e_lqt_la_LDFLAGS = -avoid-version -module
i_lqt_la_LDFLAGS  = -avoid-version -module

e_lqt_la_LIBADD = @LQT_LIBS@
i_lqt_la_LIBADD = @LQT_LIBS@


noinst_HEADERS = lqt_common.h
