plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

INCLUDES = -I$(top_srcdir)/include

SUBDIRS = web

bin_PROGRAMS = gmerlin-server

gmerlin_server_SOURCES = \
buffer.c \
client.c \
filter.c \
gmerlin-server.c \
id3gen.c \
mediahandler.c \
ondemand.c \
server.c \
sink.c \
source.c \
statichandler.c \
transcode.c 

# dist_man_MANS = gmerlin-dbedit.1

gmerlin_server_LDADD = $(top_builddir)/lib/libgmerlin.la -ldl
