libdir=@GMERLIN_PLUGIN_DIR@


if HAVE_CDIO
cdio_modules = i_vcd.la
cdio_cflags = @CDIO_CFLAGS@

if HAVE_DVDREAD
dvdread_modules = i_dvd.la
else
dvdread_modules =
endif

else
cdio_modules =
cdio_cflags =
endif

if HAVE_LINUXDVB
dvb_modules = i_dvb.la
else
dvb_modules =
endif

AM_CFLAGS = @OPT_CFLAGS@ @GMERLIN_CFLAGS@ $(cdio_cflags)

lib_LTLIBRARIES = \
i_avdec.la \
$(cdio_modules) \
$(dvdread_modules) \
$(dvb_modules)

noinst_HEADERS = avdec_common.h

i_avdec_la_SOURCES = i_avdec.c avdec_common.c
i_avdec_la_LDFLAGS = -avoid-version -module
i_avdec_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la

i_vcd_la_SOURCES = i_vcd.c avdec_common.c
i_vcd_la_LDFLAGS = -avoid-version -module
i_vcd_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la

i_dvd_la_SOURCES = i_dvd.c avdec_common.c
i_dvd_la_LDFLAGS = -avoid-version -module
i_dvd_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la

i_dvb_la_SOURCES = i_dvb.c avdec_common.c
i_dvb_la_LDFLAGS = -avoid-version -module
i_dvb_la_LIBADD = @top_srcdir@/lib/libgmerlin_avdec.la
