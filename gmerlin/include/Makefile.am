if HAVE_X11
x11_subdirs = x11
else
x11_subdirs =
endif

SUBDIRS=gui_gtk $(x11_subdirs)

pkginclude_HEADERS = \
accelerator.h \
bg_version.h \
bgsocket.h \
cfg_dialog.h \
cfg_registry.h \
charset.h \
cmdline.h \
config.h \
converters.h \
fifo.h \
filters.h \
float_cast.h \
keycodes.h \
lcdproc.h \
log.h \
msgqueue.h \
osd.h \
parameter.h \
player.h \
playermsg.h \
plugin.h \
pluginregistry.h \
remote.h \
singlepic.h \
streaminfo.h \
subprocess.h \
textrenderer.h \
transcoder.h \
transcodermsg.h \
transcoder_track.h \
translation.h \
tree.h \
utils.h \
visualize.h



noinst_HEADERS = \
bgfreetype.h \
bggavl.h \
bglv.h \
bgladspa.h \
ladspa.h \
language_table.h \
md5.h \
playerprivate.h \
registry_priv.h \
treeprivate.h \
pluginreg_priv.h \
visualize_priv.h \
xmlutils.h

EXTRA_DIST = bg_version.h.in
