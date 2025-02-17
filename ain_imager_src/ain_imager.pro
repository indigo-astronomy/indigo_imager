QT += core gui widgets network printsupport concurrent multimedia
CONFIG += c++11 release app_bundle

unix:mac {
	CONFIG += app_bundle
	ICON=$$PWD/../resource/appicon.icns
}

QMAKE_CXXFLAGS += -O3 -g
QMAKE_CXXFLAGS_RELEASE += -O3

OBJECTS_DIR=object
MOC_DIR=moc

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS QZEROCONF_STATIC

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	$$PWD/main.cpp \
	$$PWD/changeproperty.cpp \
	$$PWD/qservicemodel.cpp \
	$$PWD/capture_tab.cpp \
	$$PWD/focuser_tab.cpp \
	$$PWD/guider_tab.cpp \
	$$PWD/telescope_tab.cpp \
	$$PWD/customobject.cpp \
	$$PWD/customobjectmodel.cpp \
	$$PWD/qaddcustomobject.cpp \
	$$PWD/solver_tab.cpp \
	$$PWD/imagerwindow.cpp \
	$$PWD/qindigoservice.cpp \
	$$PWD/indigoclient.cpp \
	$$PWD/qindigoservers.cpp \
	$$PWD/propertycache.cpp \
	$$PWD/handlepropertychange.cpp \
	$$PWD/focusgraph.cpp \
	$$PWD/blobpreview.cpp \
	$$PWD/sequence_tab.cpp \
	$$PWD/syncutils.cpp \
	$$PWD/qconfigdialog.cpp \
	$$PWD/sequencer/SelectObject.cpp \
	$$PWD/sequencer/IndigoSequence.cpp \
	$$PWD/sequencer/IndigoSequenceItem.cpp \
	$$PWD/sequencer/SequenceItemModel.cpp \
	$$PWD/sequencer/IndigoSequenceParser.cpp \
	$$PWD/sequencer/SexagesimalConverter.cpp \
	$$PWD/sequencer/QLineEditSG.cpp \
	$$PWD/../common_src/coordconv.c \
	$$PWD/../object_data/indigo_cat_data.c \
	$$PWD/../common_src/utils.cpp \
	$$PWD/../common_src/imagepreview.cpp \
	$$PWD/../common_src/imageviewer.cpp \
	$$PWD/../common_src/fits.c \
	$$PWD/../common_src/xisf.c \
	$$PWD/../common_src/xml.c \
	$$PWD/../common_src/stretcher.cpp \
	$$PWD/../common_src/image_stats.cpp \
	$$PWD/../common_src/dslr_raw.c \
	$$PWD/../external/qcustomplot/qcustomplot.cpp


RESOURCES += \
	$$PWD/scripts/Sequencer.js \
	$$PWD/../qdarkstyle/style.qrc \
	$$PWD/../resource/control_panel.qss \
	$$PWD/../resource/appicon.png \
	$$PWD/../resource/indigo_logo.png \
	$$PWD/../resource/save.png \
	$$PWD/../resource/delete.png \
	$$PWD/../resource/zoom-fit-best.png \
	$$PWD/../resource/zoom-original.png \
	$$PWD/../resource/bonjour_service.png \
	$$PWD/../resource/manual_service.png \
	$$PWD/../resource/no-preview.png \
	$$PWD/../resource/led-red.png \
	$$PWD/../resource/led-grey.png \
	$$PWD/../resource/led-green.png \
	$$PWD/../resource/led-orange.png \
	$$PWD/../resource/led-red-cb.png \
	$$PWD/../resource/led-green-cb.png \
	$$PWD/../resource/led-orange-cb.png \
	$$PWD/../resource/stop.png \
	$$PWD/../resource/play.png \
	$$PWD/../resource/pause.png \
	$$PWD/../resource/record.png \
	$$PWD/../resource/focus.png \
	$$PWD/../resource/calibrate.png \
	$$PWD/../resource/guide.png \
	$$PWD/../resource/focus_in.png \
	$$PWD/../resource/focus_out.png \
	$$PWD/../resource/previous.png \
	$$PWD/../resource/next.png \
	$$PWD/../resource/zoom-in.png \
	$$PWD/../resource/zoom-out.png \
	$$PWD/../resource/arrow-up.png \
	$$PWD/../resource/arrow-down.png \
	$$PWD/../resource/edit.png \
	$$PWD/../resource/folder.png \
	$$PWD/../resource/download.png \
	$$PWD/../resource/histogram.png \
	$$PWD/../resource/error.wav \
	$$PWD/../resource/ok.wav \
	$$PWD/../resource/warning.wav \
	$$PWD/../resource/spinner.gif


# Additional import path used to resolve QML modules in Qt Creator\'s code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
	$$PWD/qservicemodel.h \
	$$PWD/imagerwindow.h \
	$$PWD/qindigoservice.h \
	$$PWD/indigoclient.h \
	$$PWD/propertycache.h \
	$$PWD/customobject.h \
	$$PWD/customobjectmodel.h \
	$$PWD/qaddcustomobject.h \
	$$PWD/qindigoservers.h \
	$$PWD/logger.h \
	$$PWD/focusgraph.h \
	$$PWD/conf.h \
	$$PWD/widget_state.h \
	$$PWD/blobpreview.h \
	$$PWD/syncutils.h \
	$$PWD/qconfigdialog.h \
	$$PWD/sequencer/SelectObject.h \
	$$PWD/sequencer/IndigoSequence.h \
	$$PWD/sequencer/IndigoSequenceItem.h \
	$$PWD/sequencer/SequenceItemModel.h \
	$$PWD/sequencer/IndigoSequenceParser.h \
	$$PWD/sequencer/SexagesimalConverter.h \
	$$PWD/sequencer/QLineEditSG.h \
	$$PWD/../common_src/version.h \
	$$PWD/../object_data/indigo_cat_data.h \
	$$PWD/../common_src/utils.h \
	$$PWD/../common_src/image_preview_lut.h \
	$$PWD/../common_src/imagepreview.h \
	$$PWD/../common_src/imageviewer.h \
	$$PWD/../common_src/fits.h \
	$$PWD/../common_src/xisf.h \
	$$PWD/../common_src/xml.h \
	$$PWD/../common_src/pixelformat.h \
	$$PWD/../external/qcustomplot/qcustomplot.h \
	$$PWD/../common_src/coordconv.h \
	$$PWD/../common_src/stretcher.h \
	$$PWD/../common_src/image_stats.h \
	$$PWD/../common_src/dslr_raw.h

INCLUDEPATH += \
	"$$PWD/../indigo/indigo_libs" \
	"$$PWD/../external" \
	"$$PWD/../external/libraw/" \
	"$$PWD/../external/lz4/" \
	"$$PWD/../common_src" \
	"$$PWD/../object_data" \
	"$$PWD/../ain_imager_src" \
	"$$PWD/../ain_imager_src/sequencer"

unix:!mac | win32 {
	LIBS += -L"$$PWD/../external/libraw/lib" -L"$$PWD/../external/lz4" -lraw -lz
}

unix {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
}

unix:mac {
	LIBS += \
		-L"$$PWD/../external/libraw/lib" -L"$$PWD/../external/lz4" -lraw -lz \
		-L"$$PWD/../external/libjpeg/.libs" -L"$$PWD/../indigo/build/lib" -lindigo -ljpeg -llz4
}

unix:!mac {
	LIBS += -L"$$PWD/../external/libjpeg/.libs" -L"$$PWD/../indigo/build/lib" -lindigo -ljpeg -l:liblz4.a
}

DISTFILES += \
	$$PWD/README.md \
	$$PWD/LICENCE.md

win32 {
	DEFINES += INDIGO_WINDOWS
	INCLUDEPATH += "$$PWD/../external/indigo_sdk/include"
	LIBS += -llz4 "$$PWD/../external/indigo_sdk/lib/libindigo_client.lib" -lws2_32
}
