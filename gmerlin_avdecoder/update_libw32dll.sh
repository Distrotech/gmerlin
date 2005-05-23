#!/bin/sh
LIBDIR=./lib/libw32dll
XINEDIR=./xine-lib/src/libw32dll
SUBDIRS="DirectShow dmo qtx qtx/qtxsdk wine" 
EXTENSIONS="h c s S"
WORKING_DIRECTORY=`pwd`

# rm -rf xine-lib 
rm -f files.txt diff.txt diff-total.txt
cvs -d:pserver:anonymous@cvs.sourceforge.net:/cvsroot/xine login
cvs -z3 -d:pserver:anonymous@cvs.sourceforge.net:/cvsroot/xine co xine-lib
cd $XINEDIR

for i in $SUBDIRS; do 
  for j in $EXTENSIONS; do
    ls -1 $i/*.$j >> $WORKING_DIRECTORY/files.txt; 
  done;
done

ls -1 *.h >> $WORKING_DIRECTORY/files.txt
cd $WORKING_DIRECTORY

for i in `cat files.txt`; do
  diff -u $LIBDIR/$i $XINEDIR/$i > diff.txt
     echo File $i changed
     cat diff.txt
     cat diff.txt >> diff_total.txt
done


