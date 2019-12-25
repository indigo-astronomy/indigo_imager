#!/bin/sh
cd libjpeg
./configure --enable-shared=no --enable-static=yes
make
cd ..
