#!/bin/sh
# generates the configure script

action_failed=0

if [ `uname` = FreeBSD ]; then
        echo "oO. Found FreeBSD ... Good luck!!!!"
        aclocal="aclocal19 -I /usr/local/share/aclocal"
        autoconf=autoconf259
        automake=automake19
else
        aclocal=aclocal
        autoconf=autoconf
        automake=automake
fi

echo "Creating configure script..."
echo "Running aclocal..."

$aclocal || action_failed=1

if [ $action_failed = 1 ]; then
        echo "Error running aclocal. It is possible that you have an incompatible version of aclocal. Please try 1.7.9"
        return 1
fi

echo "Running autoconf..."
$autoconf || action_failed=1

if [ $action_failed = 1 ]; then
        echo "Error running autoconf. It is possible that you have an incompatible version of autoconf. Please try 2.60"
        return 2
fi

echo "Running automake --add-missing --copy..."
$automake --add-missing --copy || action_failed=1

if [ $action_failed = 1 ]; then
        echo "Error running automake. It is possible that you have an incompatible version of automake. Please try 1.7.9"
        return 3
fi

echo "Hurray! Successfully created the configure script!"
