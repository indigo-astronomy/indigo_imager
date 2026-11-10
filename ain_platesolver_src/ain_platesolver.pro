QT += core gui widgets network printsupport concurrent
CONFIG += c++11 debug

unix:mac {
	CONFIG += app_bundle
	ICON=$$PWD/../resource/ain_viewer.icns
}

QMAKE_CXXFLAGS += -O3 -g
QMAKE_CXXFLAGS_RELEASE += -O3
# the solver is C and has to run at full speed
QMAKE_CFLAGS += -O3 -g

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
	$$PWD/solverengine.cpp \
	$$PWD/../external/indigo_solver/indigo_solver.c \
	$$PWD/../external/indigo_solver/indigo_solver_utils.c \
	$$PWD/../external/indigo_solver/indigo_solver_healpix.c \
	$$PWD/platesolverwindow.cpp \
	$$PWD/objectoverlay.cpp \
	$$PWD/wcsfile.cpp \
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
	$$PWD/solverengine.h \
	$$PWD/../external/indigo_solver/indigo_solver.h \
	$$PWD/../external/indigo_solver/indigo_solver_utils.h \
	$$PWD/../external/indigo_solver/indigo_solver_healpix.h \
	$$PWD/platesolverwindow.h \
	$$PWD/objectoverlay.h \
	$$PWD/wcsfile.h \
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
	"$$PWD/../external/indigo_solver/" \
	"$$PWD/../common_src" \
	"$$PWD/../ain_viewer_src" \
	"$$PWD/../ain_platesolver_src"

# Only the INDIGO client library and the star and deep sky catalogues are linked, the solver is a part of this
# application and it needs neither libindigo nor the Platesolver Agent. libindigo_client has to include indigo_raw_utils.
INDIGO_LIB_DIR = $$PWD/../indigo/build/lib

LIBS += -L"$$PWD/../external/libraw/lib" -L"$$PWD/../external/lz4" -lraw -lz

unix:!mac {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
	LIBS += -L"$$PWD/../external/libjpeg/.libs" -L"$$INDIGO_LIB_DIR"
	LIBS += -l:libindigo_client.a -l:libindigocat.a -lz -ljpeg -l:liblz4.a
}

unix:mac {
	INCLUDEPATH += "$$PWD/../external/libjpeg"
	LIBS += -L"$$PWD/../external/libjpeg/.libs" -L"$$INDIGO_LIB_DIR"
	# by the full path, the linker would prefer the dylibs in the same folder
	exists($$INDIGO_LIB_DIR/libindigo_client.a):exists($$INDIGO_LIB_DIR/libindigocat.a) {
		LIBS += $$INDIGO_LIB_DIR/libindigo_client.a $$INDIGO_LIB_DIR/libindigocat.a
	} else {
		LIBS += -lindigo_client -lindigocat
	}
	LIBS += -ljpeg -llz4
}

DISTFILES += \
	$$PWD/README.md \
	$$PWD/LICENCE.md

win32 {
	DEFINES += INDIGO_WINDOWS
	INCLUDEPATH += "$$PWD/../external/indigo_sdk/include"
	LIBS += -llz4 "$$PWD/../external/indigo_sdk/lib/libindigo_client.lib" "$$PWD/../external/indigo_sdk/lib/libindigocat.lib" -lws2_32 -lwininet
}
