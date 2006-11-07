#!/bin/sh
# generates the configure script

(aclocal && autoconf && automake -a) || echo "Could not create the configure-script. Maybe you are using some incompatible auto* versions. I tested with aclocal-1.7.9, autoconf-2.60 and automake 1.7.9"
