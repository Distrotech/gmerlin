INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = @X_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

if HAVE_XV
xv_sources = xvideo.c
xv_libs = @XV_LIBS@
else
xv_sources =
xv_libs =
endif

if HAVE_GLX
glx_sources = glvideo.c
glx_libs = @GL_LIBS@
else
glx_sources =
glx_libs =
endif

noinst_LTLIBRARIES = \
libx11.la

libx11_la_SOURCES = \
grab.c \
icon.c \
shm.c \
x11_event.c \
x11_gl.c \
x11_video.c \
x11_window.c \
overlay.c \
ximage.c \
$(glx_sources) \
$(xv_sources)

libx11_la_LIBADD =  @GMERLIN_DEP_LIBS@ @X_LIBS@ @XINERAMA_LIBS@ $(xv_libs) $(glx_libs)
