QT += core gui widgets network printsupport concurrent
CONFIG += c++11 debug

unix:mac {
	CONFIG += app_bundle
	ICON=$$PWD/../resource/ain_viewer.icns
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
	$$PWD/main.cpp \
	$$PWD/textdialog.cpp \
	$$PWD/viewerwindow.cpp \
	$$PWD/../common_src/coordconv.c \
	$$PWD/../common_src/utils.cpp \
	$$PWD/../common_src/imagepreview.cpp \
	$$PWD/../common_src/imageviewer.cpp \
	$$PWD/../common_src/image_stats.cpp \
	$$PWD/../common_src/fits.c \
	$$PWD/../common_src/raw_to_fits.c \
	$$PWD/../common_src/xisf.c \
	$$PWD/../common_src/xml.c \
	$$PWD/../common_src/dslr_raw.c \
	$$PWD/../common_src/stretcher.cpp

RESOURCES += \
	$$PWD/../qdarkstyle/style.qrc \
	$$PWD/../resource/control_panel.qss \
	$$PWD/../resource/ain_viewer.png \
	$$PWD/../resource/previous.png \
	$$PWD/../resource/next.png \
	$$PWD/../resource/indigo_logo.png \
	$$PWD/../resource/zoom-fit-best.png \
	$$PWD/../resource/zoom-original.png \
	$$PWD/../resource/bonjour_service.png \
	$$PWD/../resource/manual_service.png \
	$$PWD/../resource/no-preview.png \
	$$PWD/../resource/zoom-in.png \
	$$PWD/../resource/zoom-out.png \
	$$PWD/../resource/histogram.png


# Additional import path used to resolve QML modules in Qt Creator\'s code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
	$$PWD/viewerwindow.h \
	$$PWD/textdialog.h \
	$$PWD/conf.h \
	$$PWD/../common_src/version.h \
	$$PWD/../common_src/utils.h \
	$$PWD/../common_src/image_preview_lut.h \
	$$PWD/../common_src/imagepreview.h \
	$$PWD/../common_src/imageviewer.h \
	$$PWD/../common_src/image_stats.h \
	$$PWD/../common_src/fits.h \
	$$PWD/../common_src/raw_to_fits.h \
	$$PWD/../common_src/xisf.h \
	$$PWD/../common_src/xml.h \
	$$PWD/../common_src/pixelformat.h \
	$$PWD/../common_src/coordconv.h \
	$$PWD/../common_src/dslr_raw.h \
	$$PWD/../common_src/stretcher.h

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += "$$PWD/../indigo/indigo_libs" \
			   "$$PWD/../external" \
			   "$$PWD/../external/qtzeroconf/" \
			   "$$PWD/../external/libraw/" \
			   "$$PWD/../external/lz4/" \
			   "$$PWD/../common_src" \
			   "$$PWD/../ain_viewer_src"

LIBS += -L"$$PWD/../external/libraw/lib" -L"$$PWD/../../external/libraw/lib" -"L$$PWD/../../external/lz4" -L"$$PWD/../external/lz4" -lraw -lz

unix:!mac {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
	LIBS += -L"$$PWD/../external/libjpeg/.libs" -"L$$PWD/../indigo/build/lib" -l:libindigo.a -lz -ljpeg -l:liblz4.a
	#LIBS += -L"../external/libjpeg/.libs" -L"../indigo/build/lib" -lindigo -ljpeg -l:liblz4.a
}

unix:mac {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
	LIBS += -L"$$PWD/../external/libjpeg/.libs" -L"$$PWD/../indigo/build/lib" -lindigo -ljpeg -llz4
}

DISTFILES += \
	$$PWD/README.md \
	$$PWD/LICENCE.md

win32 {
	DEFINES += INDIGO_WINDOWS
	INCLUDEPATH += $$PWD/../../external/indigo_sdk/include
	LIBS += -llz4 $$PWD/../../external/indigo_sdk/lib/libindigo_client.lib -lws2_32
}
