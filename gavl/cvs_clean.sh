#!/bin/sh

CVSCLEANFILES="config.guess config.status config.sub configure gavl.spec gavl.pc libtool ltmain.sh autom4te-2.53.cache autom4te.cache depcomp install-sh missing  mkinstalldirs"

SUBDIRS="src gavl include"

echo "Cleaning up..."
for i in $CVSCLEANFILES; do
echo "Removing $i"
rm -rf $i
done
for i in $SUBDIRS; do
echo Removing $i/Makefile
rm -f $i/Makefile 
echo Removing $i/Makefile.in
rm -f $i/Makefile.in
done
echo "You can now rerun autogen.sh"
