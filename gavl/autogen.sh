#!/bin/sh
aclocal
libtoolize --automake --copy --force
autoheader
autoconf
automake -a
echo "You can now run ./configure"
