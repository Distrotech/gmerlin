
if HAVE_XMMS1
xmms1_cflags = @XMMS_CFLAGS@
xmms1_libs = @XMMS_LIBS@
xmms1_sources = vis_plugin_xmms1.c
else
xmms1_cflags =
xmms1_libs =
xmms1_sources =
endif

AM_CFLAGS=@GTK_CFLAGS@ @GMERLIN_CFLAGS@ $(xmms1_cflags)

bin_PROGRAMS = gmerlin_visualizer

gmerlin_visualizer_SOURCES = \
input.c \
main.c \
mainwindow.c \
fft.c \
static_xfree_libs_wrapper.c \
vis_plugin.c \
vis_plugin_xml.c \
$(xmms1_sources)

gmerlin_visualizer_LDADD = $(xmms1_libs) @GMERLIN_LIBS@ -lXxf86vm

noinst_HEADERS = \
mainwindow.h \
input.h \
fft.h \
vis_plugin.h

