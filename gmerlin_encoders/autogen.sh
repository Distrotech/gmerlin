#!/bin/sh
./make_potfiles
if test -d /usr/local/share/aclocal; then
ACLOCAL_FLAGS="-I /usr/local/share/aclocal"
else
ACLOCAL_FLAGS=""
fi
echo -n "doing aclocal..."
aclocal $ACLOCAL_FLAGS -I m4
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
