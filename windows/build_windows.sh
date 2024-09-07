#!/bin/bash

APP=ain_imager
VIEWER=ain_viewer
QT_VER=5.15.2
MINGW_VER=81

SAVE_PATH=$PATH:/c/Program\ Files\ \(x86\)/Inno\ Setup\ 6/


./build_libs_win64.sh
pushd .
export PATH=/c/Qt/${QT_VER}/mingw${MINGW_VER}_64/bin:/c/Qt/Tools/mingw${MINGW_VER}0_64/bin/:$SAVE_PATH
[ ! -d "${APP}_64" ] && mkdir ${APP}_64
cd ${APP}_64
qmake ../../${APP}_src/${APP}.pro
mingw32-make -f Makefile.release

[ ! -d "${APP}" ] && mkdir ${APP}
cd ${APP}
cp ../release/${APP}.exe .
cp ../../../external/indigo_sdk/lib/libindigo_client.dll .
windeployqt ${APP}.exe
popd

pushd .
export PATH=/c/Qt/${QT_VER}/mingw${MINGW_VER}_64/bin:/c/Qt/Tools/mingw${MINGW_VER}0_64/bin/:$SAVE_PATH
[ ! -d "${VIEWER}_64" ] && mkdir ${VIEWER}_64
cd ${VIEWER}_64
qmake ../../${VIEWER}_src/${VIEWER}.pro
mingw32-make -f Makefile.release
cp release/${VIEWER}.exe ../${APP}_64/${APP}
popd

APP_VERSION=`grep AIN_VERSION ../common_src/version.h | sed 's/"//g' |awk '{ print $3 }'`

iscc -DArch=64 -DMyAppVersion=$APP_VERSION ${APP}.iss
