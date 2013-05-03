plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@UUID_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

noinst_LTLIBRARIES = \
libupnp.la

libupnp_la_SOURCES = \
device.c \
devicedesc.c \
mediaserver.c \
servicedesc.c \
servicedesc_xml.c \
soap.c \
ssdp.c \
ssdp_device.c


libupnp_la_LIBADD =  @GMERLIN_DEP_LIBS@ @UUID_LIBS@

