
if HAVE_GTK
gtk_subdirs = plugincfg player camelot

if HAVE_ALSA
alsa_subdirs = alsamixer
else
alsa_subdirs =
endif

else
gtk_subdirs =
endif


SUBDIRS = $(alsa_subdirs) $(gtk_subdirs)
