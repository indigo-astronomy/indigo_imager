#!/bin/sh
cd libjpeg
autoreconf -fiv
./configure --enable-shared=no --enable-static=yes
make
cd ..
