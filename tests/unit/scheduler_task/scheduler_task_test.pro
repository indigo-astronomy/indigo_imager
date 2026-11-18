# Unit test of the scheduler task editor: header parsing, the header form and
# loading tasks into the visual editor. Runs without a display (offscreen).
QT += core gui widgets
CONFIG += c++11 console
CONFIG -= app_bundle
TARGET = scheduler_task_test
TEMPLATE = app

ROOT = $$PWD/../../..
SRC = $$ROOT/ain_imager_src

SOURCES += \
	scheduler_task_test.cpp \
	select_object_stub.cpp \
	$$SRC/scheduler/SchedulerTask.cpp \
	$$SRC/scheduler/SchedulerTaskForm.cpp \
	$$SRC/scheduler/SchedulerTaskEditor.cpp \
	$$SRC/scheduler/SchedulerSkyChart.cpp \
	$$SRC/sequencer/IndigoSequence.cpp \
	$$SRC/sequencer/IndigoSequenceItem.cpp \
	$$SRC/sequencer/IndigoSequenceParser.cpp \
	$$SRC/sequencer/SequenceItemModel.cpp \
	$$SRC/sequencer/SexagesimalConverter.cpp \
	$$SRC/sequencer/QLineEditSG.cpp

HEADERS += \
	$$SRC/scheduler/SchedulerTask.h \
	$$SRC/scheduler/SchedulerTaskForm.h \
	$$SRC/scheduler/SchedulerTaskEditor.h \
	$$SRC/scheduler/SchedulerSkyChart.h \
	$$SRC/sequencer/IndigoSequence.h \
	$$SRC/sequencer/IndigoSequenceItem.h \
	$$SRC/sequencer/IndigoSequenceParser.h \
	$$SRC/sequencer/SequenceItemModel.h \
	$$SRC/sequencer/QLineEditSG.h \
	$$SRC/sequencer/SelectObject.h

INCLUDEPATH += $$SRC $$SRC/sequencer $$SRC/scheduler $$ROOT/object_data $$ROOT/common_src $$ROOT/indigo/indigo_libs
DEFINES += SCHEDULER_GUIDE=\\\"$$ROOT/indigo/indigo_docs/INDIGO_SCHEDULER_GUIDE.md\\\"
