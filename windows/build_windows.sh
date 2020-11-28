#!/bin/bash
export PATH=$PATH:/c/Qt/5.12.9/mingw73_64/bin:/c/Qt/Tools/mingw730_64/bin/

qmake ../ain_imager.pro
mingw32-make
