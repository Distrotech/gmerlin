CFLAGS = @CFLAGS@  -I@top_srcdir@/include

if ARCH_X86
mmx_libs = mmx/libgavl_mmx.la mmxext/libgavl_mmxext.la
mmx_subdirs = mmx mmxext
else
mmx_libs = 
endif

SUBDIRS = c $(mmx_subdirs)

lib_LTLIBRARIES= libgavl.la
#noinst_LTLIBRARIES = libgavl.la

libgavl_la_SOURCES = \
audio.c \
audiobuffer.c \
audioformat.c \
audioframe.c \
colorspace.c \
cputest.c \
interleave.c \
mix.c \
sampleformat.c \
time.c \
timer.c \
video.c \
videoformat.c \
videoframe.c

noinst_HEADERS=colorspace_macros.h

libgavl_la_LIBADD = $(mmx_libs) c/libgavl_c.la

EXTRA_libgavl_la_SOURCES = _video_copy.c _planar_420_422.c

EXTRA_DIST=Makefile.am.in
