
if HAVE_MMX
mmx_libs = mmx/libgavl_mmx.la mmxext/libgavl_mmxext.la
mmx_subdirs = mmx mmxext
else
mmx_libs = 
mmx_subdirs =
endif

if HAVE_SSE
sse_libs = sse/libgavl_sse.la
sse_subdirs = sse
else
sse_libs = 
sse_subdirs =
endif

SUBDIRS = hq c libgdither libsamplerate $(mmx_subdirs) \
$(sse_subdirs)

lib_LTLIBRARIES= libgavl.la
#noinst_LTLIBRARIES = libgavl.la

libgavl_la_SOURCES = \
arith128.c \
audioconverter.c \
audioformat.c \
audioframe.c \
audiooptions.c \
blend.c \
colorspace.c \
cputest.c \
deinterlace.c \
deinterlace_copy.c \
dsp.c \
interleave.c \
memcpy.c \
mix.c \
peakdetector.c \
rectangle.c \
sampleformat.c \
samplerate.c \
scale.c \
scale_context.c \
scale_kernels.c \
scale_table.c \
time.c \
timer.c \
video.c \
videoformat.c \
videoframe.c \
videooptions.c \
volume.c

noinst_HEADERS = \
csp_packed_packed.h  \
csp_packed_planar.h  \
csp_planar_packed.h  \
csp_planar_planar.h

libgavl_la_LIBADD = \
@GMERLIN_DEP_LIBS@ \
$(mmx_libs) \
$(sse_libs) \
c/libgavl_c.la \
hq/libgavl_hq.la \
libgdither/libgdither.la \
libsamplerate/libsamplerate.la \
-lm

EXTRA_DIST=Makefile.am.in
