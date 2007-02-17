libdir=@GMERLIN_PLUGIN_DIR@

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = i_mikmod.la

i_mikmod_la_SOURCES  = i_mikmod.c 

i_mikmod_la_LDFLAGS  = -avoid-version -module

i_mikmod_la_LIBADD = @GMERLIN_DEP_LIBS@

