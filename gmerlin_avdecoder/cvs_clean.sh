#!/bin/sh
SUBDIRS="lib lib/real_rtsp lib/libw32dll lib/libw32dll/DirectShow lib/libw32dll/qtx lib/libw32dll/qtx/qtxsdk lib/libw32dll/wine plugins include tests"

make distclean

CLEANFILES="Makefile.in *.o *.lo *.la .libs .deps"

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
