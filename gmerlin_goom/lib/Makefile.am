# libgoom2

if HAVE_MMX
MMX_FILES=mmx.c xmmx.c 
else
MMX_FILES=
endif

if HAVE_PPC
PPC_FILES=ppc_zoom_ultimate.s ppc_drawings.s
else
PPC_FILES=
endif

goom2_lib_LTLIBRARIES = libgoom2.la
goom2_libdir = $(libdir)

libgoom2_la_LDFLAGS = -export-dynamic -export-symbols-regex "goom.*" 

libgoom2_la_SOURCES = \
	goomsl_yacc.y goomsl_lex.l goomsl.c goomsl_hash.c goomsl_heap.c \
	goom_tools.c $(MMX_FILES) $(PPC_FILES) \
	config_param.c convolve_fx.c filters.c \
	flying_stars_fx.c gfontlib.c gfontrle.c \
	goom_core.c graphic.c ifs.c lines.c \
	mathtools.c sound_tester.c surf3d.c \
	tentacle3d.c plugin_info.c \
	v3d.c drawmethods.c \
	cpu_info.c

AM_YFLAGS=-d

