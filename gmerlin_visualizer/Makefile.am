# aclocal Path
ACLOCAL_AMFLAGS = -I m4

Desktopdir = $(datadir)/applications
Desktop_DATA = gmerlin-visualizer.desktop

Icondir = @GMERLIN_DATA_DIR@/icons
Icon_DATA = visualizer_icon.png

SUBDIRS = src m4

bin_SCRIPTS = gmerlin_visualizer_launcher

EXTRA_DIST = visualizer_icon.png \
gmerlin-visualizer.desktop.in \
gmerlin-visualizer.spec.in \
autogen.sh \
cvs_clean.sh \
gmerlin_visualizer_launcher
