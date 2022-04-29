#!/bin/sh
cd external/libjpeg
autoreconf -fiv
./configure --enable-shared=no --enable-static=yes
make
cd ../..

cd external/libraw
mkdir lib/
make -f Makefile.mingw library
cd ../..

cd external/lz4
make
cd ../..

