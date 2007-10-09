# aclocal Path
ACLOCAL_AMFLAGS = -I m4

if HAVE_GTK
gtk_pc = gmerlin-gtk.pc
else
gtk_pc = 
endif

SUBDIRS = po lib plugins tests apps icons skins include doc m4 osd

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gmerlin.pc $(gtk_pc) 

pkgdatadir = $(datadir)/gmerlin/
pkgdata_DATA = plugin.sym
dist_pkgdata_DATA = plugin.sym


EXTRA_DIST= config.rpath gmerlin.spec.in gmerlin.pc.in autogen.sh cvs_clean.sh cpuinfo.sh make_potfiles 
