QT += core gui widgets

CONFIG += c++11
CONFIG -= app_bundle

TARGET = dome_view_test
TEMPLATE = app

OBJECTS_DIR = object
MOC_DIR = moc

SOURCES += \
	main.cpp \
	../DomeView.cpp

HEADERS += \
	../DomeView.h

INCLUDEPATH += \
	..

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
