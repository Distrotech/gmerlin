
if HAVE_ESD
esd_subdirs = esd
else
esd_subdirs =
endif

if HAVE_LIBJPEG
jpeg_subdirs = jpeg
else
jpeg_subdirs =
endif

if HAVE_VORBIS
vorbis_subdirs = vorbis
else
vorbis_subdirs =
endif

if HAVE_LQT
lqt_subdirs = lqt
else
lqt_subdirs = 
endif

SUBDIRS = \
$(lqt_subdirs) \
$(jpeg_subdirs) \
$(vorbis_subdirs) \
$(esd_subdirs) \
oss \
x11

