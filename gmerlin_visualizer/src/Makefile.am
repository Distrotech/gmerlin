AM_CFLAGS=@GTK_CFLAGS@ @XMMS_CFLAGS@ @GMERLIN_CFLAGS@

bin_PROGRAMS = gmerlin_visualizer

gmerlin_visualizer_SOURCES = \
input.c \
main.c \
mainwindow.c \
fft.c \
plugin.c \
static_xfree_libs_wrapper.c

gmerlin_visualizer_LDADD = @XMMS_LIBS@ @GMERLIN_LIBS@ -lXxf86vm

noinst_HEADERS = \
mainwindow.h \
input.h \
fft.h

