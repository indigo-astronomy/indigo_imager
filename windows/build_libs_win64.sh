#!/bin/sh
export PATH=/c/Qt/Tools/mingw730_64/bin/:$PATH
cd ../external/libraw
mingw32-make -f Makefile.mingw clean
mkdir lib/
mingw32-make -f Makefile.mingw library
cd ../../windows

cd ../external/lz4
mkdir dll/
mingw32-make clean
mingw32-make CC=gcc
cd ../../windows
