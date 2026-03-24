#!/bin/bash
make clean
rm ain_viewer_src/Makefile.ain_viewer
rm ain_imager_src/Makefile.ain_imager
rm Makefile
qmake
