if HAVE_GTK
gtk_subdirs = gtk
else
gtk_subdirs =
endif

SUBDIRS = $(gtk_subdirs)

AM_CFLAGS = -I@top_srcdir@/gavl/include @XML2_CFLAGS@

lib_LTLIBRARIES = libgmerlin.la
libgmerlin_la_SOURCES = \
album.c \
album_xml.c \
cfg_item.c \
cfg_registry.c \
cfg_section.c \
cfg_xml.c \
fifo.c \
formats.c \
hexdump.c \
http.c \
loader.c \
msgqueue.c \
parameter.c \
player.c \
player_input.c \
player_oa.c \
player_ov.c \
player_audio.c \
player_thread.c \
player_video.c \
playercmd.c \
pluginregistry.c \
pluginreg_xml.c \
searchpath.c \
socket.c \
streaminfo.c \
stringutils.c \
tree.c \
tree_xml.c \
urilist.c \
utf8.c

libgmerlin_la_LIBADD = @XML2_LIBS@ -ldl -lpthread

