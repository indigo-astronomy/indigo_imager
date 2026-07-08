QT += core gui widgets
CONFIG += c++11 debug app_bundle

TARGET = indigo_guidelog_analyzer

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
	$$PWD/guideloganalyzerwindow.cpp \
	$$PWD/../external/simpleplot/simpleplot.cpp

HEADERS += \
	$$PWD/guideloganalyzerwindow.h \
	$$PWD/../external/simpleplot/simpleplot.h

RESOURCES += \
	$$PWD/../resource/fonts.qrc \
	$$PWD/../resource/indigo_guidelog_analyzer.qrc \
	$$PWD/../qdarkstyle/style.qrc

INCLUDEPATH += \
	"$$PWD/../external/simpleplot"

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target
