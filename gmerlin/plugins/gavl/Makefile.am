INCLUDES = -I$(top_srcdir)/include

libdir=@GMERLIN_PLUGIN_DIR@
AM_LDFLAGS = @GMERLIN_PLUGIN_LDFLAGS@ -avoid-version -module

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = ir_gavl.la iw_gavl.la

ir_gavl_la_SOURCES = ir_gavl.c
ir_gavl_la_LIBADD = @GMERLIN_DEP_LIBS@ @MODULE_LIBADD@

iw_gavl_la_SOURCES = iw_gavl.c
iw_gavl_la_LIBADD = @GMERLIN_DEP_LIBS@ @MODULE_LIBADD@
