
if HAVE_MUSICBRAINZ
musicbrainz_sources = musicbrainz.c sha1.c
musicbrainz_libs = @MUSICBRAINZ_LIBS@
musicbrainz_cflags = @MUSICBRAINZ_CFLAGS@
else
musicbrainz_sources =
musicbrainz_libs =
musicbrainz_cflags = 
endif

AM_CFLAGS = $(musicbrainz_cflags)

libdir = @GMERLIN_PLUGIN_DIR@

lib_LTLIBRARIES = i_cdaudio.la

i_cdaudio_la_SOURCES = \
i_cdaudio.c \
cdaudio.c \
cdaudio_linux.c \
$(musicbrainz_sources)

i_cdaudio_la_LIBADD = $(musicbrainz_libs)
i_cdaudio_la_LDFLAGS  = -avoid-version -module

noinst_HEADERS = cdaudio.h sha1.h
