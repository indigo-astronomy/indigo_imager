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
	main.cpp \
	viewerwindow.cpp \
	../common_src/utils.cpp \
	../common_src/imagepreview.cpp \
	../common_src/imageviewer.cpp \
	../common_src/fits.c \
	../common_src/debayer.c \

RESOURCES += \
	../qdarkstyle/style.qrc \
	../resource/control_panel.qss \
	../resource/ain_viewer.png \
	../resource/indigo_logo.png \
	../resource/zoom-fit-best.png \
	../resource/zoom-original.png \
	../resource/bonjour_service.png \
	../resource/manual_service.png \
	../resource/no-preview.png \
	../resource/zoom-in.png \
	../resource/zoom-out.png \
	../resource/histogram.png


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
	viewerwindow.h \
	conf.h \
	../common_src/utils.h \
	../common_src/image_preview_lut.h \
	../common_src/imagepreview.h \
	../common_src/imageviewer.h \
	../common_src/fits.h \
	../common_src/debayer.h \
	../common_src/pixelformat.h

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += "../indigo/indigo_libs" + "../external" + "../external/qtzeroconf/" + "../common_src" + "../ain_viewer_src"

unix {
	INCLUDEPATH += "../external/libjpeg"
	LIBS += -L"../external/libjpeg/.libs" -L"../indigo/build/lib" -lindigo -ljpeg
}

DISTFILES += \
	README.md \
	LICENCE.md \

win32 {
	LIBS += -lws2_32
        DEFINES += INDIGO_WINDOWS

	SOURCES += \
	    ../indigo/indigo_libs/indigo_base64.c \
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
	    ../indigo/indigo_libs/indigo/indigo_bus.h \
	    ../indigo/indigo_libs/indigo/indigo_client.h \
	    ../indigo/indigo_libs/indigo/indigo_client_xml.h \
	    ../indigo/indigo_libs/indigo/indigo_config.h \
	    ../indigo/indigo_libs/indigo/indigo_io.h \
	    ../indigo/indigo_libs/indigo/indigo_version.h \
	    ../indigo/indigo_libs/indigo/indigo_xml.h \
	    ../indigo/indigo_libs/indigo/indigo_token.h \
	    ../indigo/indigo_libs/indigo/indigo_names.h
}
