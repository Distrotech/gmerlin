libdir = @LV_PLUGIN_DIR@
AM_LDFLAGS = -avoid-version -module

AM_CFLAGS = @LV_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

lib_LTLIBRARIES = \
actor_lemuria.la

actor_lemuria_la_SOURCES  = actor_lemuria.c
actor_lemuria_la_LIBADD  = @LV_LIBS@ $(top_srcdir)/lib/liblemuria.la
