
if ARCH_X86
mmx_libs = mmx/libgavl_mmx.la mmxext/libgavl_mmxext.la
mmx_subdirs = mmx mmxext
else
mmx_libs = 
endif

AM_CFLAGS = @SECRETRABBIT_CFLAGS@

SUBDIRS = c libgdither $(mmx_subdirs)

lib_LTLIBRARIES= libgavl.la
#noinst_LTLIBRARIES = libgavl.la

libgavl_la_SOURCES = \
audio.c \
audioconverter.c \
audioformat.c \
audioframe.c \
colorspace.c \
cputest.c \
interleave.c \
mix.c \
sampleformat.c \
samplerate.c \
time.c \
timer.c \
video.c \
videoformat.c \
videoframe.c \
volume.c

noinst_HEADERS = \
csp_packed_packed.h  \
csp_packed_planar.h  \
csp_planar_packed.h  \
csp_planar_planar.h

libgavl_la_LIBADD = \
$(mmx_libs) \
c/libgavl_c.la \
libgdither/libgdither.la \
@SECRETRABBIT_LIBS@ \
-lm

EXTRA_libgavl_la_SOURCES = _video_copy.c _planar_420_422.c

EXTRA_DIST=Makefile.am.in
