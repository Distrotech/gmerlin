if HAVE_X11
x11_subdirs = x11
else
x11_subdirs =
endif

if HAVE_OSS
oss_subdirs = oss
else
oss_subdirs =
endif

if HAVE_LIBPNG
png_subdirs = png
else
png_subdirs =
endif

if HAVE_LIBTIFF
tiff_subdirs = tiff
else
tiff_subdirs =
endif

if HAVE_V4L2
v4l2_subdirs = v4l2
else
v4l2_subdirs =
endif

if HAVE_V4L
v4l_subdirs = v4l
else
v4l_subdirs =
endif

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

if HAVE_LQT
lqt_subdirs = lqt
else
lqt_subdirs = 
endif

if HAVE_ALSA
alsa_subdirs = alsa
else
alsa_subdirs = 
endif

if HAVE_JACK
jack_subdirs = jack
else
jack_subdirs = 
endif

if HAVE_PULSEAUDIO
pulseaudio_subdirs = pulseaudio
else
pulseaudio_subdirs = 
endif

if HAVE_CDIO
cdio_subdirs = cdaudio
else
cdio_subdirs = 
endif

SUBDIRS = \
audiofilters \
edl \
gavl \
videofilters \
visualization \
$(alsa_subdirs) \
$(jack_subdirs) \
$(lqt_subdirs) \
$(jpeg_subdirs) \
$(esd_subdirs) \
$(v4l_subdirs) \
$(v4l2_subdirs) \
$(oss_subdirs) \
$(png_subdirs) \
$(tiff_subdirs) \
$(x11_subdirs) \
$(cdio_subdirs) \
$(pulseaudio_subdirs) \
mikmod \
wavwrite \
image \
postprocessors \
subwriters


