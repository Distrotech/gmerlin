plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@XML2_CFLAGS@ \
@GTK_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\"  \
-DDATA_DIR=\"$(pkgdatadir)\"


Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-camelot.desktop

if HAVE_V4L
vloopback_sources = vloopback.c
else
vloopback_sources = 
endif


bin_PROGRAMS = camelot

camelot_SOURCES = webcam.c webcam_window.c main.c $(vloopback_sources)
camelot_LDADD = \
$(top_builddir)/lib/gtk/libgmerlin_gtk.la \
$(top_builddir)/lib/libgmerlin.la

noinst_HEADERS = webcam.h webcam_window.h webcam_priv.h vloopback.h
