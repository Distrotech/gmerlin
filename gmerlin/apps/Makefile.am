
if HAVE_GTK
gtk_subdirs = ossmixer plugincfg player camelot
endif

if HAVE_ALSA
alsa_subdirs = alsamixer
else
alsa_subdirs =
endif

SUBDIRS = $(alsa_subdirs) $(gtk_subdirs)
