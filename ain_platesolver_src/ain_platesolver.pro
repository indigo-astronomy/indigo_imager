QT += core gui widgets network printsupport concurrent
CONFIG += c++11 debug

unix:mac {
	CONFIG += app_bundle
	ICON=$$PWD/../resource/ain_viewer.icns
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
	$$PWD/solverclient.cpp \
	$$PWD/platesolverwindow.cpp \
	$$PWD/objectoverlay.cpp \
	$$PWD/../ain_viewer_src/textdialog.cpp \
	$$PWD/../ain_viewer_src/viewerwindow.cpp \
	$$PWD/../common_src/coordconv.c \
	$$PWD/../common_src/utils.cpp \
	$$PWD/../common_src/imagepreview.cpp \
	$$PWD/../common_src/imageviewer.cpp \
	$$PWD/../common_src/live_stacker.cpp \
	$$PWD/../common_src/antialiaseditems.cpp \
	$$PWD/../common_src/image_stats.cpp \
	$$PWD/../common_src/fits.c \
	$$PWD/../common_src/raw_to_fits.c \
	$$PWD/../common_src/xisf.c \
	$$PWD/../common_src/xml.c \
	$$PWD/../common_src/dslr_raw.c \
	$$PWD/../common_src/snr_calculator.cpp \
	$$PWD/../common_src/snr_overlay.cpp \
	$$PWD/../common_src/image_inspector.cpp \
	$$PWD/../common_src/image_inspector_overlay.cpp \
	$$PWD/../common_src/stretcher.cpp

RESOURCES += \
	$$PWD/../resource/fonts.qrc \
	$$PWD/../qdarkstyle/style.qrc \
	$$PWD/../resource/ain_viewer.qrc \



# Additional import path used to resolve QML modules in Qt Creator\'s code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
	$$PWD/solverclient.h \
	$$PWD/platesolverwindow.h \
	$$PWD/objectoverlay.h \
	$$PWD/../ain_viewer_src/viewerwindow.h \
	$$PWD/../ain_viewer_src/textdialog.h \
	$$PWD/../ain_viewer_src/conf.h \
	$$PWD/../common_src/version.h \
	$$PWD/../common_src/utils.h \
	$$PWD/../common_src/image_preview_lut.h \
	$$PWD/../common_src/imagepreview.h \
	$$PWD/../common_src/imageviewer.h \
	$$PWD/../common_src/live_stacker.h \
	$$PWD/../common_src/antialiaseditems.h \
	$$PWD/../common_src/image_stats.h \
	$$PWD/../common_src/fits.h \
	$$PWD/../common_src/raw_to_fits.h \
	$$PWD/../common_src/xisf.h \
	$$PWD/../common_src/xml.h \
	$$PWD/../common_src/pixelformat.h \
	$$PWD/../common_src/coordconv.h \
	$$PWD/../common_src/snr_calculator.h \
	$$PWD/../common_src/snr_overlay.h \
	$$PWD/../common_src/image_inspector_overlay.h \
	$$PWD/../common_src/image_inspector.h \
	$$PWD/../common_src/dslr_raw.h \
	$$PWD/../common_src/stretcher.h

#unix:!mac {
#    CONFIG += link_pkgconfig
#    PKGCONFIG += indigo
#}

INCLUDEPATH += \
	"$$PWD/../indigo/indigo_libs" \
	"$$PWD/../external" \
	"$$PWD/../external/qtzeroconf/" \
	"$$PWD/../external/libraw/" \
	"$$PWD/../external/lz4/" \
	"$$PWD/../common_src" \
	"$$PWD/../ain_viewer_src" \
	"$$PWD/../ain_platesolver_src"

# The agent is a shared library that needs libindigo, the viewer has to share the bus with it. A static
# INDIGO client library would be a second copy of the bus, so libindigo is linked here as a shared library.
# It also provides jpeg and libraw, a second copy of those in the executable would interpose its own.
INDIGO_LIB_DIR = $$PWD/../indigo/build/lib
INDIGO_FLAVOR = $$(AIN_INDIGO_FLAVOR)

LIBS += -L"$$PWD/../external/lz4" -L"$$INDIGO_LIB_DIR" -lz

unix:!mac {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
	LIBS += -l:libindigo.so -l:libindigocat.so -l:liblz4.a -ldl
}

unix:mac {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
	LIBS += -lindigo -lindigocat -llz4
}

# A development build runs with the INDIGO it was built against, a package with the installed one
isEmpty(INDIGO_FLAVOR):exists($$INDIGO_LIB_DIR/libindigo.*) {
	QMAKE_RPATHDIR += $$clean_path($$INDIGO_LIB_DIR)
}

DISTFILES += \
	$$PWD/README.md \
	$$PWD/LICENCE.md
