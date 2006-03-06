
if HAVE_XMMS1
xmms1_cflags = @XMMS_CFLAGS@
xmms1_libs = @XMMS_LIBS@
xmms1_sources = vis_plugin_xmms1.c
else
xmms1_cflags =
xmms1_libs =
xmms1_sources =
endif

if HAVE_LIBVISUAL
libvisual_cflags = @LIBVISUAL_CFLAGS@
libvisual_libs = @LIBVISUAL_LIBS@
libvisual_sources = vis_plugin_lv.c
else
libvisual_cflags =
libvisual_libs =
libvisual_sources =
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
$(xmms1_sources) \
$(libvisual_sources)

gmerlin_visualizer_LDADD = $(xmms1_libs) $(libvisual_libs) @GMERLIN_LIBS@ 

noinst_HEADERS = \
mainwindow.h \
input.h \
fft.h \
vis_plugin.h

