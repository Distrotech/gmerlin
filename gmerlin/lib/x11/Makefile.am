INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = @X_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

if HAVE_XV
xv_sources = xvideo.c
else
xv_sources =
endif

noinst_LTLIBRARIES = \
libx11.la

libx11_la_SOURCES = \
shm.c \
x11_event.c \
x11_gl.c \
x11_video.c \
x11_window.c \
glvideo.c \
overlay.c \
ximage.c \
$(xv_sources)

libx11_la_LIBADD =  @GMERLIN_DEP_LIBS@ @X_LIBS@ @XINERAMA_LIBS@ @XV_LIBS@ @GL_LIBS@


