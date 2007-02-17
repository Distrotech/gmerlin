libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @LAME_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = \
e_lame.la

e_lame_la_SOURCES = e_lame.c
e_lame_la_LDFLAGS = -avoid-version -module
e_lame_la_LIBADD = @GMERLIN_DEP_LIBS@ @top_srcdir@/lib/libgmerlin_encoders.la @LAME_LIBS@
