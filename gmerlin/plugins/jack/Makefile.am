plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = @JACK_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)/plugins\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module


plugin_LTLIBRARIES = oa_jack.la i_jack.la

oa_jack_la_SOURCES = oa_jack.c jack_common.c
i_jack_la_SOURCES  = i_jack.c  jack_common.c

oa_jack_la_LIBADD = @GMERLIN_DEP_LIBS@ @JACK_LIBS@ @MODULE_LIBADD@
i_jack_la_LIBADD = @GMERLIN_DEP_LIBS@ @JACK_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = jack_common.h
