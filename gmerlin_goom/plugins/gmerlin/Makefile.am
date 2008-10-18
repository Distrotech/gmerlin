libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @GMERLIN_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

AM_LDFLAGS = -avoid-version -module @GMERLIN_PLUGIN_LDFLAGS@

lib_LTLIBRARIES = \
vis_goom.la

vis_goom_la_SOURCES = vis_goom.c
vis_goom_la_LIBADD = @top_srcdir@/lib/libgoom2.la @GMERLIN_LIBS@

