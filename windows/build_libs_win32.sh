#!/bin/sh
export PATH=/c/Qt/Tools/mingw730_32/bin/:$PATH
cd ../external/LibRaw-0.20.2
mingw32-make -f Makefile.mingw clean
mkdir lib/
mingw32-make -f Makefile.mingw library
cd ../../windows

