QT += core gui widgets network printsupport concurrent
CONFIG += c++11 debug

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
	imager_src/main.cpp \
	imager_src/qservicemodel.cpp \
	imager_src/capture_tab.cpp \
	imager_src/focuser_tab.cpp \
	imager_src/guider_tab.cpp \
	imager_src/telescope_tab.cpp \
	imager_src/imagerwindow.cpp \
	imager_src/qindigoservice.cpp \
	imager_src/indigoclient.cpp \
	imager_src/qindigoservers.cpp \
	imager_src/propertycache.cpp \
	imager_src/changeproperty.cpp \
	imager_src/handlepropertychange.cpp \
	imager_src/focusgraph.cpp \
	imager_src/blobpreview.cpp \
	common_src/utils.cpp \
	common_src/imagepreview.cpp \
	common_src/imageviewer.cpp \
	common_src/fits.c \
	common_src/debayer.c \
	external/qcustomplot/qcustomplot.cpp


RESOURCES += \
	qdarkstyle/style.qrc \
	resource/control_panel.qss \
	resource/appicon.png \
	resource/indigo_logo.png \
	resource/zoom-fit-best.png \
	resource/zoom-original.png \
	resource/bonjour_service.png \
	resource/manual_service.png \
	resource/no-preview.png \
	resource/led-red.png \
	resource/led-grey.png \
	resource/led-green.png \
	resource/led-orange.png \
	resource/led-red-cb.png \
	resource/led-green-cb.png \
	resource/led-orange-cb.png \
	resource/stop.png \
	resource/play.png \
	resource/pause.png \
	resource/record.png \
	resource/focus.png \
	resource/calibrate.png \
	resource/guide.png \
	resource/focus_in.png \
	resource/focus_out.png \
	resource/zoom-in.png \
	resource/zoom-out.png


# Additional import path used to resolve QML modules in Qt Creator\'s code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
	imager_src/version.h \
	imager_src/qservicemodel.h \
	imager_src/imagerwindow.h \
	imager_src/qindigoservice.h \
	imager_src/indigoclient.h \
	imager_src/propertycache.h \
	imager_src/qindigoservers.h \
	imager_src/logger.h \
	imager_src/focusgraph.h \
	imager_src/conf.h \
	imager_src/widget_state.h \
	imager_src/blobpreview.h \
	common_src/utils.h \
	common_src/image_preview_lut.h \
	common_src/imagepreview.h \
	common_src/imageviewer.h \
	common_src/fits.h \
	common_src/debayer.h \
	common_src/pixelformat.h \
	external/qcustomplot/qcustomplot.h



include(external/qtzeroconf/qtzeroconf.pri)

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += "$${PWD}/indigo/indigo_libs" + "$${PWD}/external" + "$${PWD}/external/qtzeroconf/" + "$${PWD}/common_src" + "$${PWD}/imager_src"

unix {
	INCLUDEPATH += "$${PWD}/external/libjpeg"
	LIBS += -L"$${PWD}/external/libjpeg/.libs" -L"$${PWD}/indigo/build/lib" -lindigo -ljpeg
}

DISTFILES += \
	README.md \
	LICENCE.md \

win32 {
        DEFINES += INDIGO_WINDOWS

        SOURCES += \
            indigo/indigo_libs/indigo_base64.c \
            indigo/indigo_libs/indigo_bus.c \
            indigo/indigo_libs/indigo_client.c \
            indigo/indigo_libs/indigo_client_xml.c \
            indigo/indigo_libs/indigo_version.c \
	    indigo/indigo_libs/indigo_io.c \
	    indigo/indigo_libs/indigo_token.c \
            indigo/indigo_libs/indigo_xml.c

        HEADERS += \
            indigo/indigo_libs/indigo/indigo_base64.h \
            indigo/indigo_libs/indigo/indigo_base64_luts.h \
            indigo/indigo_libs/indigo/indigo_bus.h \
            indigo/indigo_libs/indigo/indigo_client.h \
            indigo/indigo_libs/indigo/indigo_client_xml.h \
            indigo/indigo_libs/indigo/indigo_config.h \
            indigo/indigo_libs/indigo/indigo_io.h \
            indigo/indigo_libs/indigo/indigo_version.h \
	    indigo/indigo_libs/indigo/indigo_xml.h \
	    indigo/indigo_libs/indigo/indigo_token.h \
            indigo/indigo_libs/indigo/indigo_names.h
}
