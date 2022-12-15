QT += core gui widgets network printsupport concurrent multimedia
CONFIG += c++11 release app_bundle

unix:mac {
	CONFIG += app_bundle
	ICON=../resource/appicon.icns
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
	main.cpp \
	changeproperty.cpp \
	qservicemodel.cpp \
	capture_tab.cpp \
	focuser_tab.cpp \
	guider_tab.cpp \
	telescope_tab.cpp \
	customobject.cpp \
	customobjectmodel.cpp \
	qaddcustomobject.cpp \
	solver_tab.cpp \
	imagerwindow.cpp \
	qindigoservice.cpp \
	indigoclient.cpp \
	qindigoservers.cpp \
	propertycache.cpp \
	handlepropertychange.cpp \
	focusgraph.cpp \
	blobpreview.cpp \
	syncutils.cpp \
	qconfigdialog.cpp \
	../common_src/coordconv.c \
	../object_data/indigo_cat_data.c \
	../common_src/utils.cpp \
	../common_src/imagepreview.cpp \
	../common_src/imageviewer.cpp \
	../common_src/fits.c \
	../common_src/xisf.c \
	../common_src/xml.c \
	../common_src/debayer.c \
	../common_src/stretcher.cpp \
	../common_src/dslr_raw.c \
	../external/qcustomplot/qcustomplot.cpp


RESOURCES += \
	../qdarkstyle/style.qrc \
	../resource/control_panel.qss \
	../resource/appicon.png \
	../resource/indigo_logo.png \
	../resource/save.png \
	../resource/delete.png \
	../resource/zoom-fit-best.png \
	../resource/zoom-original.png \
	../resource/bonjour_service.png \
	../resource/manual_service.png \
	../resource/no-preview.png \
	../resource/led-red.png \
	../resource/led-grey.png \
	../resource/led-green.png \
	../resource/led-orange.png \
	../resource/led-red-cb.png \
	../resource/led-green-cb.png \
	../resource/led-orange-cb.png \
	../resource/stop.png \
	../resource/play.png \
	../resource/pause.png \
	../resource/record.png \
	../resource/focus.png \
	../resource/calibrate.png \
	../resource/guide.png \
	../resource/focus_in.png \
	../resource/focus_out.png \
	../resource/previous.png \
	../resource/next.png \
	../resource/zoom-in.png \
	../resource/zoom-out.png \
	../resource/histogram.png \
	../resource/error.wav \
	../resource/ok.wav \
	../resource/warning.wav


# Additional import path used to resolve QML modules in Qt Creator\'s code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
	qservicemodel.h \
	imagerwindow.h \
	qindigoservice.h \
	indigoclient.h \
	propertycache.h \
	customobject.h \
	customobjectmodel.h \
	qaddcustomobject.h \
	qindigoservers.h \
	logger.h \
	focusgraph.h \
	conf.h \
	widget_state.h \
	blobpreview.h \
	syncutils.h \
	qconfigdialog.h \
	../common_src/version.h \
	../object_data/indigo_cat_data.h \
	../common_src/utils.h \
	../common_src/image_preview_lut.h \
	../common_src/imagepreview.h \
	../common_src/imageviewer.h \
	../common_src/fits.h \
	../common_src/xisf.h \
	../common_src/xml.h \
	../common_src/debayer.h \
	../common_src/pixelformat.h \
	../external/qcustomplot/qcustomplot.h \
	../common_src/coordconv.h \
	../common_src/stretcher.h \
	../common_src/dslr_raw.h



include(../external/qtzeroconf/qtzeroconf.pri)

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += "../indigo/indigo_libs" + "../external" + "../external/qtzeroconf/" + "../external/libraw/" + "../external/lz4/" + "../common_src" + "../object_data" + "../ain_imager_src"

unix:!mac | win32 {
	LIBS += -L"../external/libraw/lib" -L"../../external/libraw/lib" -L"../external/lz4" -L"../../external/lz4" -lraw -lz
}

unix {
	INCLUDEPATH += "../external/libjpeg"
}

unix:mac {
	LIBS += -L"../external/libraw/lib" -L"../external/lz4" -lraw -lz \
		-L"../external/libjpeg/.libs" -L"../indigo/build/lib" -lindigo -ljpeg -llz4
}

unix:!mac {
	LIBS += -L"../external/libjpeg/.libs" -L"../indigo/build/lib" -lindigo -ljpeg -l:liblz4.a
}

DISTFILES += \
	README.md \
	LICENCE.md \

win32 {
        DEFINES += INDIGO_WINDOWS
		LIBS += -llz4

        SOURCES += \
            ../indigo/indigo_libs/indigo_base64.c \
            ../indigo/indigo_libs/indigo_md5.c \
            ../indigo/indigo_libs/indigo_bus.c \
            ../indigo/indigo_libs/indigo_client.c \
            ../indigo/indigo_libs/indigo_client_xml.c \
            ../indigo/indigo_libs/indigo_version.c \
            ../indigo/indigo_libs/indigo_io.c \
            ../indigo/indigo_libs/indigo_token.c \
            ../indigo/indigo_libs/indigo_xml.c

        HEADERS += \
            ../indigo/indigo_libs/indigo/indigo_base64.h \
            ../indigo/indigo_libs/indigo/indigo_base64_luts.h \
            ../indigo/indigo_libs/indigo/indigo_md5.h \
            ../indigo/indigo_libs/indigo/indigo_bus.h \
            ../indigo/indigo_libs/indigo/indigo_client.h \
            ../indigo/indigo_libs/indigo/indigo_client_xml.h \
            ../indigo/indigo_libs/indigo/indigo_config.h \
            ../indigo/indigo_libs/indigo/indigo_io.h \
            ../indigo/indigo_libs/indigo/indigo_version.h \
            ../indigo/indigo_libs/indigo/indigo_xml.h \
            ../indigo/indigo_libs/indigo/indigo_token.h \
            ../indigo/indigo_libs/indigo/indigo_names.h \
            ../indigo/indigo_libs/indigo/indigo_platesolver.h
}
