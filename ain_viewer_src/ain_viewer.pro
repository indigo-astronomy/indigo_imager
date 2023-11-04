QT += core gui widgets network printsupport concurrent
CONFIG += c++11 debug

unix:mac {
	CONFIG += app_bundle
	ICON=../resource/ain_viewer.icns
}

QMAKE_CXXFLAGS += -O3
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
	textdialog.cpp \
	viewerwindow.cpp \
	../common_src/coordconv.c \
	../common_src/utils.cpp \
	../common_src/imagepreview.cpp \
	../common_src/imageviewer.cpp \
	../common_src/image_stats.cpp \
	../common_src/fits.c \
	../common_src/raw_to_fits.c \
	../common_src/xisf.c \
	../common_src/xml.c \
	../common_src/dslr_raw.c \
	../common_src/stretcher.cpp

RESOURCES += \
	../qdarkstyle/style.qrc \
	../resource/control_panel.qss \
	../resource/ain_viewer.png \
	../resource/previous.png \
	../resource/next.png \
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
	viewerwindow.h \
	textdialog.h \
	conf.h \
	../common_src/version.h \
	../common_src/utils.h \
	../common_src/image_preview_lut.h \
	../common_src/imagepreview.h \
	../common_src/imageviewer.h \
	../common_src/image_stats.h \
	../common_src/fits.h \
	../common_src/raw_to_fits.h \
	../common_src/xisf.h \
	../common_src/xml.h \
	../common_src/pixelformat.h \
	../common_src/coordconv.h \
	../common_src/dslr_raw.h \
	../common_src/stretcher.h

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += "../indigo/indigo_libs" + "../external" + "../external/qtzeroconf/" + "../external/libraw/" + "../external/lz4/" + "../common_src" + "../ain_viewer_src"
LIBS += -L"../external/libraw/lib" -L"../../external/libraw/lib" -L"../../external/lz4" -L"../external/lz4" -lraw -lz

unix:!mac {
	INCLUDEPATH += "../external/libjpeg"
	LIBS += -L"../external/libjpeg/.libs" -L"../indigo/build/lib" -l:libindigo.a -lz -ljpeg -l:liblz4.a
	#LIBS += -L"../external/libjpeg/.libs" -L"../indigo/build/lib" -lindigo -ljpeg -l:liblz4.a
}

unix:mac {
	INCLUDEPATH += "../external/libjpeg"
	LIBS += -L"../external/libjpeg/.libs" -L"../indigo/build/lib" -lindigo -ljpeg -llz4
}

DISTFILES += \
	README.md \
	LICENCE.md \

win32 {
	DEFINES += INDIGO_WINDOWS
	INCLUDEPATH += ../../external/indigo_sdk/include
	LIBS += -llz4 ../../external/indigo_sdk/lib/libindigo_client.lib -lws2_32
}
