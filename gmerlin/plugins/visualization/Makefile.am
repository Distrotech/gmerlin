INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"
LDADD = @MODULE_LIBADD@
lib_LTLIBRARIES = \
vis_scope.la

vis_scope_la_SOURCES  = vis_scope.c ../videofilters/colormatrix.c

