#!/bin/sh
SUBDIRS="src"

make distclean

CLEANFILES="Makefile.in *.o *.lo *.la .libs .deps config.h*"

TOPCLEANFILES="INSTALL aclocal.m4 config.guess config.status config.sub configure gmerlin-viszalizer.spec gmerlin-viszalizer.desktop libtool ltmain.sh autom4te*.cache depcomp install-sh missing mkinstalldirs Makefile.in"

echo "Cleaning up..."
for i in $TOPCLEANFILES; do
echo "Removing $i"
rm -rf $i
done

for i in $SUBDIRS; do
  for j in $CLEANFILES; do
  echo Removing $i/$j
  rm -rf $i/$j
  done
done

echo "You can now rerun autogen.sh"
