
if HAVE_GMERLIN
gmerlin_subdirs = plugins
else
gmerlin_subdirs =
endif

SUBDIRS = lib $(gmerlin_subdirs) tests include

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gmerlin_avdec.pc

EXTRA_DIST = autogen.sh cvs_clean.sh gmerlin_avdec.pc.in gmerlin_avdec.spec.in lqt_avcodec.m4
