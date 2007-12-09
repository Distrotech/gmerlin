if HAVE_X11
x11_subdirs = x11
x11_libadd = x11/libx11.la
else
x11_subdirs =
x11_libadd =
endif

if HAVE_GTK
gtk_subdirs = gtk
else
gtk_subdirs =
endif

if HAVE_LV
lv_sources = bglv.c
lv_cflags  = @LV_CFLAGS@
lv_libs  = @LV_LIBS@
else
lv_sources =
lv_cflags  =
lv_libs  =
endif

SUBDIRS = $(gtk_subdirs) $(x11_subdirs)

AM_CFLAGS = @FONTCONFIG_CFLAGS@ @FREETYPE_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\" $(lv_cflags)

libgmerlin_la_LIBADD = $(x11_libadd) @FONTCONFIG_LIBS@ @FREETYPE_LIBS@ @XML2_LIBS@ @ICONV_LIBS@ @LIBINTL@ $(lv_libs) -ldl -lpthread

bin_PROGRAMS = gmerlin_visualizer_slave

gmerlin_visualizer_slave_SOURCES = visualize_slave.c
gmerlin_visualizer_slave_LDADD = libgmerlin.la

lib_LTLIBRARIES = libgmerlin.la
libgmerlin_la_SOURCES = \
$(lv_sources) \
accelerator.c \
album.c \
album_xml.c \
bggavl.c \
bgladspa.c \
bgxml.c \
cfg_item.c \
cfg_registry.c \
cfg_section.c \
cfg_xml.c \
chapterlist.c \
chapterlist_xml.c \
charset.c \
cmdline.c \
converters.c \
device.c \
fifo.c \
filters.c \
formats.c \
hexdump.c \
lcdproc.c \
log.c \
md5.c \
metadata.c \
metadata_xml.c \
msgqueue.c \
osd.c \
parameter.c \
parameter_xml.c \
player.c \
player_input.c \
player_oa.c \
player_ov.c \
player_audio.c \
player_subtitle.c \
player_thread.c \
player_video.c \
playercmd.c \
pluginregistry.c \
pluginreg_xml.c \
remote.c \
searchpath.c \
singlepic.c \
socket.c \
streaminfo.c \
stringutils.c \
subprocess.c \
textrenderer.c \
transcoder.c \
transcoder_enc.c \
transcoder_pp.c \
transcoder_track.c \
transcoder_track_xml.c \
translation.c \
tree.c \
tree_xml.c \
urilist.c \
utf8.c \
visualize.c

