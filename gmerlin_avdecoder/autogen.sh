#!/bin/sh
echo -n "doing aclocal..."
aclocal
echo "done"
echo -n "doing libtoolize..."
libtoolize --automake --copy --force
echo "done"
echo -n "doing autoheader..."
autoheader
echo "done"
echo -n "doing autoconf..."
autoconf
echo "done"
echo -n "doing automake..."
automake -a
echo "done"
echo "You can now run ./configure"
