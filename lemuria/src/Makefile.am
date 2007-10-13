AM_CFLAGS = @ESD_CFLAGS@ @GAVL_CFLAGS@

if HAVE_ESD
esdprogs = lemuria
else
esdprogs =
endif

# xaos/src/ui/libui.la

bin_PROGRAMS = $(esdprogs)

lemuria_LDADD = ../lib/liblemuria.la @ESD_LIBS@

lemuria_SOURCES = main.c

