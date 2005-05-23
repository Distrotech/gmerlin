if HAVE_GTK
gtk_pc = gmerlin-gtk.pc
else
gtk_pc = 
endif


SUBDIRS = lib plugins tests apps icons skins include doc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gmerlin.pc $(gtk_pc) 

EXTRA_DIST=gmerlin.spec.in gmerlin.pc.in autogen.sh cvs_clean.sh
