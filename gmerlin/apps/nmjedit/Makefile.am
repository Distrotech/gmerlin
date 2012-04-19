plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = gmerlin-nmjedit
# dist_man_MANS = gmerlin-record.1

gmerlin_nmjedit_LDADD = $(top_builddir)/lib/libgmerlin.la @SQLITE3_LIBS@ -ldl
gmerlin_nmjedit_SOURCES= \
album.c \
bgalbum.c \
create.c \
dir.c \
gmerlin-nmjedit.c \
file.c \
nmjedit.c \
song.c

noinst_HEADERS = nmjedit.h
