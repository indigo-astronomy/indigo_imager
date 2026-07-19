QT += core gui widgets
CONFIG += c++11 debug app_bundle

TARGET = ain_guidelog_viewer

unix:mac {
	CONFIG += app_bundle
	ICON = $$PWD/../resource/appicon.icns
}

QMAKE_CXXFLAGS += -O3 -g
QMAKE_CXXFLAGS_RELEASE += -O3

OBJECTS_DIR = object
MOC_DIR = moc

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
	$$PWD/main.cpp \
	$$PWD/guidelogviewerwindow.cpp \
	$$PWD/pecurvewindow.cpp \
	$$PWD/pecurve.cpp \
	$$PWD/guidelogparser.cpp \
	$$PWD/guidelogstats.cpp \
	$$PWD/../common_src/balancebar.cpp \
	$$PWD/../external/simpleplot/simpleplot.cpp

HEADERS += \
	$$PWD/guidelogviewerwindow.h \
	$$PWD/pecurvewindow.h \
	$$PWD/pecurve.h \
	$$PWD/guidelogparser.h \
	$$PWD/guidelogstats.h \
	$$PWD/../common_src/balancebar.h \
	$$PWD/../external/simpleplot/simpleplot.h

RESOURCES += \
	$$PWD/../resource/fonts.qrc \
	$$PWD/../resource/ain_guidelog_viewer.qrc \
	$$PWD/../qdarkstyle/style.qrc

INCLUDEPATH += \
	"$$PWD/../external/simpleplot" \
	"$$PWD/../common_src"

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target
