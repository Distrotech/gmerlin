gmerlin_plugindir = @gmerlin_plugindir@

INCLUDES = -I$(top_srcdir)/include

AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module
AM_CFLAGS = @LAME_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

gmerlin_plugin_LTLIBRARIES = \
e_lame.la

e_lame_la_SOURCES = e_lame.c
e_lame_la_LIBADD = @GMERLIN_DEP_LIBS@ $(top_builddir)/lib/libgmerlin_encoders.la @LAME_LIBS@
