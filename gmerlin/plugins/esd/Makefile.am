libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @ESD_CFLAGS@

lib_LTLIBRARIES = oa_esd.la i_esd.la

oa_esd_la_SOURCES = oa_esd.c
i_esd_la_SOURCES  = i_esd.c

oa_esd_la_LDFLAGS = -avoid-version -module
i_esd_la_LDFLAGS  = -avoid-version -module

oa_esd_la_LIBADD = @ESD_LIBS@
i_esd_la_LIBADD = @ESD_LIBS@

