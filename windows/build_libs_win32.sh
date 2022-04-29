#!/bin/sh
export PATH=/c/Qt/Tools/mingw730_32/bin/:$PATH
cd ../external/libraw
mingw32-make -f Makefile.mingw clean
mkdir lib/
mingw32-make -f Makefile.mingw library
cd ../../windows

cd ../external/lz4
mingw32-make clean
mingw32-make
cd ../../windows

