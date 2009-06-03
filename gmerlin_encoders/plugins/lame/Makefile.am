libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = @OPT_CFLAGS@ @LAME_CFLAGS@

lib_LTLIBRARIES = \
e_lame.la

e_lame_la_SOURCES = e_lame.c
e_lame_la_LDFLAGS = -avoid-version -module
e_lame_la_LIBADD = @top_srcdir@/lib/libgmerlin_encoders.la @LAME_LIBS@
