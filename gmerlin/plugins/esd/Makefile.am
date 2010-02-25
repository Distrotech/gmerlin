INCLUDES = -I$(top_srcdir)/include

plugindir=$(pkglibdir)/plugins
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = \
@ESD_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


plugin_LTLIBRARIES = oa_esd.la i_esd.la

oa_esd_la_SOURCES = oa_esd.c
i_esd_la_SOURCES  = i_esd.c

oa_esd_la_LIBADD = @GMERLIN_DEP_LIBS@ @ESD_LIBS@ @MODULE_LIBADD@
i_esd_la_LIBADD = @GMERLIN_DEP_LIBS@ @ESD_LIBS@ @MODULE_LIBADD@

