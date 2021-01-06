#!/bin/bash

SAVE_PATH=$PATH

export PATH=/c/Qt/5.12.10/mingw73_32/bin:/c/Qt/Tools/mingw730_32/bin/:$SAVE_PATH

pushd .
[ ! -d "ain_32" ] && mkdir ain_32
cd ain_32
qmake ../../ain_imager.pro
mingw32-make -f Makefile.release

[ ! -d "ain_imager" ] && mkdir ain_imager
cd ain_imager
cp ../release/ain_imager.exe .
windeployqt ain_imager.exe

popd
export PATH=/c/Qt/5.12.10/mingw73_64/bin:/c/Qt/Tools/mingw730_64/bin/:$SAVE_PATH
[ ! -d "ain_64" ] && mkdir ain_64
cd ain_64
qmake ../../ain_imager.pro
mingw32-make -f Makefile.release

[ ! -d "ain_imager" ] && mkdir ain_imager
cd ain_imager
cp ../release/ain_imager.exe .
windeployqt ain_imager.exe

