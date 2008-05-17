INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = @ESD_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = oa_esd.la i_esd.la

oa_esd_la_SOURCES = oa_esd.c
i_esd_la_SOURCES  = i_esd.c

oa_esd_la_LIBADD = @GMERLIN_DEP_LIBS@ @ESD_LIBS@
i_esd_la_LIBADD = @GMERLIN_DEP_LIBS@ @ESD_LIBS@

