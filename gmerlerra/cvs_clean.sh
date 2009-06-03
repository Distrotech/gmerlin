#!/bin/sh

CVSCLEANFILES="config.guess config.status config.sub configure libtool ltmain.sh depcomp install-sh missing mkinstalldirs include/config.h include/config.h.in"

SUBDIRS="tests lib lib/gtk m4 icons apps include include/gui_gtk ."

make distclean

echo "Cleaning up..."
for i in $CVSCLEANFILES; do
echo "Removing $i"
rm -rf $i
done
echo Removing autom4te*
rm -rf autom4te*
for i in $SUBDIRS; do
echo Removing $i/Makefile
rm -f $i/Makefile 
echo Removing $i/Makefile.in
rm -f $i/Makefile.in
echo Removing $i/.libs $i/.deps
rm -rf $i/.libs $i/.deps
done
echo "You can now rerun autogen.sh"
