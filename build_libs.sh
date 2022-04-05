#!/bin/sh
cd external/libjpeg
autoreconf -fiv
./configure --enable-shared=no --enable-static=yes
make
cd ../..

cd external/LibRaw-0.20.2
mkdir lib/
make -f Makefile.mingw library
cd ../..

