libdir=@GMERLIN_PLUGIN_DIR@

CFLAGS = @CFLAGS@ @GMERLIN_CFLAGS@
LIBS = @LIBS@

lib_LTLIBRARIES = \
i_avdec.la 

i_avdec_la_SOURCES = i_avdec.c
i_avdec_la_LDFLAGS = -avoid-version -module
i_avdec_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la
