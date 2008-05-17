INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = @LQT_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = e_lqt.la i_lqt.la

e_lqt_la_SOURCES  = e_lqt.c lqt_common.c lqtgavl.c
i_lqt_la_SOURCES  = i_lqt.c lqt_common.c lqtgavl.c

e_lqt_la_LIBADD = @GMERLIN_DEP_LIBS@ @LQT_LIBS@
i_lqt_la_LIBADD = @GMERLIN_DEP_LIBS@ @LQT_LIBS@


noinst_HEADERS = lqt_common.h lqtgavl.h
