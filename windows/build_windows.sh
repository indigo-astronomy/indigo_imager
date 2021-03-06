#!/bin/bash

APP=ain_imager
VIEWER=ain_viewer

SAVE_PATH=$PATH:/c/Program\ Files\ \(x86\)/Inno\ Setup\ 6/

export PATH=/c/Qt/5.12.10/mingw73_32/bin:/c/Qt/Tools/mingw730_32/bin/:$SAVE_PATH

pushd .
[ ! -d "${APP}_32" ] && mkdir ${APP}_32
cd ${APP}_32
qmake ../../${APP}_src/${APP}.pro
mingw32-make -f Makefile.release

[ ! -d "${APP}" ] && mkdir ${APP}
cd ${APP}
cp ../release/${APP}.exe .
windeployqt ${APP}.exe
popd

pushd .
export PATH=/c/Qt/5.12.10/mingw73_32/bin:/c/Qt/Tools/mingw730_32/bin/:$SAVE_PATH
[ ! -d "${VIEWER}_32" ] && mkdir ${VIEWER}_32
cd ${VIEWER}_32
qmake ../../${VIEWER}_src/${VIEWER}.pro
mingw32-make -f Makefile.release
cp release/${VIEWER}.exe ../${APP}_32/${APP}
popd


pushd .
export PATH=/c/Qt/5.12.10/mingw73_64/bin:/c/Qt/Tools/mingw730_64/bin/:$SAVE_PATH
[ ! -d "${APP}_64" ] && mkdir ${APP}_64
cd ${APP}_64
qmake ../../${APP}_src/${APP}.pro
mingw32-make -f Makefile.release

[ ! -d "${APP}" ] && mkdir ${APP}
cd ${APP}
cp ../release/${APP}.exe .
windeployqt ${APP}.exe
popd

pushd .
export PATH=/c/Qt/5.12.10/mingw73_64/bin:/c/Qt/Tools/mingw730_64/bin/:$SAVE_PATH
[ ! -d "${VIEWER}_64" ] && mkdir ${VIEWER}_64
cd ${VIEWER}_64
qmake ../../${VIEWER}_src/${VIEWER}.pro
mingw32-make -f Makefile.release
cp release/${VIEWER}.exe ../${APP}_64/${APP}
popd

APP_VERSION=`grep AIN_VERSION ../ain_imager_src/version.h | sed 's/"//g' |awk '{ print $3 }'`

iscc -DArch=32 -DMyAppVersion=$APP_VERSION ${APP}.iss
iscc -DArch=64 -DMyAppVersion=$APP_VERSION ${APP}.iss
