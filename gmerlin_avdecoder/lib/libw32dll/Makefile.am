# include $(top_srcdir)/misc/Makefile.common

AM_CFLAGS = -I$(srcdir)/wine -DWIN32_PATH=\"@w32_path@\" @LIBBGAV_CFLAGS@

SUBDIRS = wine DirectShow dmo qtx
noinst_HEADERS = libwin32.h w32codec.h
