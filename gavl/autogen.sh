#!/bin/sh
aclocal -I m4
libtoolize --automake --copy --force
autoheader
autoconf
automake -a
echo "You can now run ./configure"
