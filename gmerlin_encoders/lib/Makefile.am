if HAVE_FLAC
flac_libs = libbgflac.la
else
flac_libs =
endif

AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"

noinst_LTLIBRARIES = libgmerlin_encoders.la $(flac_libs)

libgmerlin_encoders_la_SOURCES = \
id3v1.c \
id3v2.c

libbgflac_la_CFLAGS  = @FLAC_CFLAGS@
libbgflac_la_SOURCES = bgflac.c
