plugindir=$(pkglibdir)/plugins

AM_CFLAGS = \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

INCLUDES = -I$(top_srcdir)/include

bin_PROGRAMS = gmerlin-server

gmerlin_server_SOURCES = \
buffer.c \
client.c \
gmerlin-server.c \
mediahandler.c \
server.c \
transcode.c 

# dist_man_MANS = gmerlin-dbedit.1

gmerlin_server_LDADD = $(top_builddir)/lib/libgmerlin.la -ldl
