plugindir=$(pkglibdir)/plugins
pkgbindir=$(pkglibdir)/bin
INCLUDES = -I$(top_srcdir)/include

if HAVE_X11
x11_subdirs = x11
x11_libadd = x11/libx11.la
else
x11_subdirs =
x11_libadd =
endif

if HAVE_UUID
upnp_subdirs = upnp
upnp_libadd = upnp/libupnp.la
else
upnp_subdirs =
upnp_libadd =
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

if HAVE_POSIX_SEMAPHORES
sem_sources =
else
sem_sources = uthread_sem.c
endif

SUBDIRS = $(x11_subdirs) $(upnp_subdirs) . $(gtk_subdirs)

AM_CFLAGS = \
@FONTCONFIG_CFLAGS@ \
@FREETYPE_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\"  \
-DDATA_DIR=\"$(pkgdatadir)\" \
-DPKGBIN_DIR=\"$(pkgbindir)\" \
$(lv_cflags)

libgmerlin_la_LDFLAGS = -version-info @LTVERSION_CURRENT@:@LTVERSION_REVISION@:@LTVERSION_AGE@ -export-symbols-regex ^bg_ @GMERLIN_LIB_LDFLAGS@

libgmerlin_la_LIBADD = $(x11_libadd) $(upnp_libadd) @SQLITE3_LIBS@ @FONTCONFIG_LIBS@ @FREETYPE_LIBS@ @XML2_LIBS@ @ICONV_LIBS@ @MQ_LIBS@ @LIBINTL@ $(lv_libs) -ldl -lpthread

pkgbin_PROGRAMS = gmerlin_visualizer_slave

gmerlin_visualizer_slave_SOURCES = visualize_slave.c
gmerlin_visualizer_slave_LDADD = libgmerlin.la -ldl -lpthread

lib_LTLIBRARIES = libgmerlin.la
libgmerlin_la_SOURCES = \
$(lv_sources) \
$(sem_sources) \
accelerator.c \
album.c \
album_xml.c \
audiofilters.c \
bggavl.c \
bgladspa.c \
bgfrei0r.c \
bgplug.c \
bgplug_io.c \
bgshm.c \
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
db.c \
db_audio.c \
db_audioalbum.c \
db_dir.c \
db_file.c \
db_image.c \
db_object.c \
db_scan.c \
db_vfolder.c \
device.c \
edl.c \
edldec.c \
edl_xml.c \
encoder.c \
fileformat.c \
fileutils.c \
formats.c \
frametimer.c \
gavfenc.c \
hexdump.c \
http.c \
language_table.c \
lcdproc.c \
log.c \
md5.c \
mediaconnector.c \
metadata.c \
metadata_xml.c \
msgqueue.c \
ocr.c \
osd.c \
ov.c \
ovl2text.c \
parameter.c \
parameter_xml.c \
player.c \
player_input.c \
player_oa.c \
player_ov.c \
player_audio.c \
player_loop.c \
player_subtitle.c \
player_thread.c \
player_time.c \
player_video.c \
playercmd.c \
pluginfuncs.c \
pluginregistry.c \
pluginreg_xml.c \
preset.c \
preset_xml.c \
recorder.c \
recorder_audio.c \
recorder_video.c \
remote.c \
searchpath.c \
serialize.c \
singlepic.c \
socket.c \
sqlite.c \
streaminfo.c \
stringutils.c \
subprocess.c \
subtitle.c \
textrenderer.c \
threadpool.c \
thumbnail.c \
transcoder.c \
transcoder_pp.c \
transcoder_track.c \
transcoder_track_xml.c \
translation.c \
tree.c \
tree_xml.c \
urilist.c \
utf8.c \
videofilters.c \
visualize.c

