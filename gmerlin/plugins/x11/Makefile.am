libdir=@GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = \
ov_x11.la

ov_x11_la_SOURCES = ov_x11.c
ov_x11_la_LDFLAGS = -avoid-version -module
ov_x11_la_LIBADD =  -L/usr/X11R6/lib -lXv -lXext -lX11

