
if HAVE_PLAYER
player_subdirs = player
else
player_subdirs =
endif

if HAVE_PLUGINCFG
plugincfg_subdirs = plugincfg
else
plugincfg_subdirs =
endif

if HAVE_ALSAMIXER
alsamixer_subdirs = alsamixer
else
alsamixer_subdirs =
endif

#if HAVE_CAMELOT
#camelot_subdirs = camelot
#else
#camelot_subdirs =
#endif

if HAVE_TRANSCODER
transcoder_subdirs = transcoder
else
transcoder_subdirs =
endif

if HAVE_RECORDER
recorder_subdirs = recorder
else
recorder_subdirs =
endif

if HAVE_KBD
kbd_subdirs = kbd
else
kbd_subdirs =
endif

if HAVE_VISUALIZER
visualizer_subdirs = visualizer
else
visualizer_subdirs =
endif

if HAVE_THUMBNAILER
thumbnailer_subdirs = thumbnailer
else
thumbnailer_subdirs =
endif

if HAVE_NMJEDIT
nmjedit_subdirs = nmjedit
else
nmjedit_subdirs =
endif


SUBDIRS = $(player_subdirs) \
cmdlineplayer \
cmdlinerecorder \
dbedit \
gavftools \
mediaserver \
$(plugincfg_subdirs) \
$(alsamixer_subdirs) \
$(nmjedit_subdirs) \
$(camelot_subdirs) \
$(recorder_subdirs) \
$(transcoder_subdirs) \
$(thumbnailer_subdirs) \
$(kbd_subdirs) \
$(visualizer_subdirs)

