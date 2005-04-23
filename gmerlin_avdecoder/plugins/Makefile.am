libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @OPT_CFLAGS@ @GMERLIN_CFLAGS@

if HAVE_CDIO
cdio_modules = i_vcd.la
else
cdio_modules =
endif

lib_LTLIBRARIES = \
i_avdec.la \
$(cdio_modules)

noinst_HEADERS = avdec_common.h

i_avdec_la_SOURCES = i_avdec.c avdec_common.c
i_avdec_la_LDFLAGS = -avoid-version -module
i_avdec_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la

i_vcd_la_SOURCES = i_vcd.c avdec_common.c
i_vcd_la_LDFLAGS = -avoid-version -module
i_vcd_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la
