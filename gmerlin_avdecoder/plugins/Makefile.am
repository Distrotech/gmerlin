libdir=@GMERLIN_PLUGIN_DIR@

CFLAGS = @CFLAGS@ @GMERLIN_CFLAGS@
LIBS = @LIBS@

lib_LTLIBRARIES = \
r_asx.la \
i_avdec.la \
r_ram.la

i_avdec_la_SOURCES = i_avdec.c
i_avdec_la_LDFLAGS = -avoid-version -module
i_avdec_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la

r_asx_la_SOURCES = r_asx.c
r_asx_la_LDFLAGS = -avoid-version -module

r_ram_la_SOURCES = r_ram.c
r_ram_la_LDFLAGS = -avoid-version -module
