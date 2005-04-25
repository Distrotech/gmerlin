
if HAVE_MUSICBRAINZ
musicbrainz_sources = musicbrainz.c
musicbrainz_libs = @MUSICBRAINZ_LIBS@
musicbrainz_cflags = @MUSICBRAINZ_CFLAGS@
else
musicbrainz_sources =
musicbrainz_libs =
musicbrainz_cflags = 
endif

AM_CFLAGS = $(musicbrainz_cflags) $(cdparanoia_cflags) @CDIO_CFLAGS@ @CDIO_CDDA_CFLAGS@ @CDIO_PARANOIA_CFLAGS@

libdir = @GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = i_cdaudio.la

i_cdaudio_la_SOURCES = \
i_cdaudio.c \
cdaudio.c \
cdaudio_xml.c \
sha1.c	\
cdparanoia.c \
cdtext.c \
$(musicbrainz_sources) \
$(cdparanoia_sources)


i_cdaudio_la_LIBADD = $(musicbrainz_libs) @CDIO_LIBS@ @CDIO_CDDA_LIBS@ @CDIO_PARANOIA_LIBS@
i_cdaudio_la_LDFLAGS  = -avoid-version -module

noinst_HEADERS = cdaudio.h sha1.h
