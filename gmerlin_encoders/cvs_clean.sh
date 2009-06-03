#!/bin/sh
SUBDIRS="lib include plugins plugins/flac plugins/vorbis plugins/lame plugins/faac"

make distclean

CLEANFILES="Makefile.in *.o *.lo *.la .libs .deps"

TOPCLEANFILES="aclocal.m4 config.guess config.status config.sub configure gmerlin.spec gmerlin.pc libtool ltmain.sh autom4te*.cache depcomp install-sh missing mkinstalldirs Makefile.in include/config.h.in COPYING"

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
