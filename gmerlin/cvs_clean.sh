#!/bin/sh
SUBDIRS="plugins/cdaudio plugins/v4l plugins/alsa plugins/esd plugins/lqt plugins/oss plugins/wavwrite plugins/x11 plugins/tiff plugins/postprocessors tests include include/gui_gtk icons lib lib/gtk apps apps/player apps/plugincfg apps/alsamixer apps/camelot apps/transcoder"

make distclean

CLEANFILES="Makefile Makefile.in *.o *.lo *.la .libs .deps"

TOPCLEANFILES="aclocal.m4 config.guess config.status config.sub configure gmerlin.spec gmerlin.pc libtool ltmain.sh autom4te*.cache depcomp install-sh missing mkinstalldirs Makefile.in"

echo "Cleaning up..."
for i in $TOPCLEANFILES; do
echo "Removing $i"
rm -rf $i
done

for i in $SUBDIRS; do
  for j in $CLEANFILES; do
  echo rm -rf $i/$j
  rm -rf $i/$j
  done
done

echo "You can now rerun autogen.sh"
