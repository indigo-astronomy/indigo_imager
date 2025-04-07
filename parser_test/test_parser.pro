QT += core gui  # Add gui module

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = test_parser
TEMPLATE = app

SOURCES += \
    test_parser.cpp \
    ../ain_imager_src/sequencer/IndigoSequenceParser.cpp \
    ../ain_imager_src/sequencer/SequenceItemModel.cpp

HEADERS += \
    ../ain_imager_src/sequencer/IndigoSequenceParser.h \
    ../ain_imager_src/sequencer/SequenceItemModel.h

INCLUDEPATH += \
    ../ain_imager_src/sequencer

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
