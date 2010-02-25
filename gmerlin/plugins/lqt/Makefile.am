INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
@LQT_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = e_lqt.la i_lqt.la

e_lqt_la_SOURCES  = e_lqt.c lqt_common.c lqtgavl.c
i_lqt_la_SOURCES  = i_lqt.c lqt_common.c lqtgavl.c

e_lqt_la_LIBADD = @GMERLIN_DEP_LIBS@ @LQT_LIBS@ @MODULE_LIBADD@
i_lqt_la_LIBADD = @GMERLIN_DEP_LIBS@ @LQT_LIBS@ @MODULE_LIBADD@


noinst_HEADERS = lqt_common.h lqtgavl.h
