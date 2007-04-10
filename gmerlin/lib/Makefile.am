if HAVE_GTK
gtk_subdirs = gtk
else
gtk_subdirs =
endif

SUBDIRS = $(gtk_subdirs)

AM_CFLAGS = @FONTCONFIG_CFLAGS@ @FREETYPE_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = libgmerlin.la
libgmerlin_la_SOURCES = \
album.c \
album_xml.c \
bggavl.c \
bgladspa.c \
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
utf8.c

libgmerlin_la_LIBADD = @FONTCONFIG_LIBS@ @FREETYPE_LIBS@ @XML2_LIBS@ @ICONV_LIBS@ @LIBINTL@ -ldl -lpthread

