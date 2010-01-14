#!/bin/sh
OLD_YEARS="2001 - 2008"
NEW_YEARS="2001 - 2010"
find . -name '*.[ch]' -exec sed -i "s/$OLD_YEARS/$NEW_YEARS/g" {} \;
