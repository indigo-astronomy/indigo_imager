QT += core gui widgets network
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
	main.cpp \
	qservicemodel.cpp \
	imagerwindow.cpp \
	qindigoservice.cpp \
	indigoclient.cpp \
	qindigoservers.cpp \
	blobpreview.cpp \
	fits/fits.c \
	debayer/debayer.c \


RESOURCES += \
	qdarkstyle/style.qrc \
	resource/control_panel.qss \
	resource/appicon.png \
	resource/indigo_logo.png \
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
	resource/led-grey-dev.png \
	resource/led-green-dev.png \
	resource/server.png \
	resource/ccd-grey.png \
	resource/ccd-green.png \
	resource/mount-grey.png \
	resource/mount-green.png \
	resource/wheel-grey.png \
	resource/wheel-green.png \
	resource/ao-grey.png \
	resource/ao-green.png \
	resource/focuser-grey.png \
	resource/focuser-green.png \
	resource/gps-grey.png \
	resource/gps-green.png \
	resource/guider-grey.png \
	resource/guider-green.png \
	resource/dome-grey.png \
	resource/dome-green.png \
	resource/weather-grey.png \
	resource/weather-green.png \
	resource/powerbox-grey.png \
	resource/powerbox-green.png \
	resource/flatbox-grey.png \
	resource/flatbox-green.png \
	resource/joystick-grey.png \
	resource/joystick-green.png \
	resource/shutter-grey.png \
	resource/shutter-green.png \
	resource/sqm-grey.png \
	resource/sqm-green.png \
	resource/dustcap-grey.png \
	resource/dustcap-green.png


# Additional import path used to resolve QML modules in Qt Creator\'s code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
	version.h \
	qservicemodel.h \
	imagerwindow.h \
	qindigoservice.h \
	indigoclient.h \
	blobpreview.h \
	qindigoservers.h \
	logger.h \
	fits/fits.h \
	debayer/debayer.h \
	debayer/pixelformat.h \
	conf.h


include(qtzeroconf/qtzeroconf.pri)

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += "$${PWD}/indigo/indigo_libs"

unix {
	INCLUDEPATH += "$${PWD}/libjpeg"
	LIBS += -L"$${PWD}/libjpeg/.libs" -L"$${PWD}/indigo/build/lib" -lindigo -ljpeg
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
            indigo/indigo_libs/indigo/indigo_names.h
}
