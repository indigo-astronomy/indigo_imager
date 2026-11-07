# AIN solver agent tests - see README.md
#
#   make              build the test helpers
#   make test         run all cases
#   make test-quick   run the cases that need neither indexes nor long sequences
#   make test-unit    run the unit tests, they need no server and no display
#   make list         list the cases
#   make clean        remove the helpers
#
# make test CASES="menu_switch 80" runs selected cases.

ROOT_DIR := $(abspath ..)
INDIGO_ROOT ?= $(ROOT_DIR)/indigo
BUILD_DIR := build
QUICK_CASES := startup_load menu_switch switch_failures sequencer_script

QT_FLAGS := $(shell pkg-config --cflags Qt5Widgets)
QT_LIBS := $(shell pkg-config --libs Qt5Widgets)
AIN_INCLUDES := -I$(ROOT_DIR)/ain_imager_src -I$(ROOT_DIR)/common_src -I$(INDIGO_ROOT)/indigo_libs -I$(ROOT_DIR)/external -I$(ROOT_DIR)/external/libraw -I$(ROOT_DIR)/external/lz4 -I$(ROOT_DIR)/external/libjpeg -I$(ROOT_DIR)/object_data

HELPERS := $(BUILD_DIR)/ain_harness.so $(BUILD_DIR)/conf_offset
UNIT_TESTS := $(BUILD_DIR)/wcs_file_test

QTCORE_FLAGS := $(shell pkg-config --cflags Qt5Core)
QTCORE_LIBS := $(shell pkg-config --libs Qt5Core)
PLATESOLVER_INCLUDES := -I$(ROOT_DIR)/ain_platesolver_src -I$(ROOT_DIR)/common_src -I$(ROOT_DIR)/external/indigo_solver -I$(INDIGO_ROOT)/indigo_libs

all: $(HELPERS)

$(BUILD_DIR)/ain_harness.so: harness/ain_harness.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) -shared -fPIC -O1 -std=gnu++11 -Wall $< -o $@ $(QT_FLAGS) $(QT_LIBS)

$(BUILD_DIR)/conf_offset: harness/conf_offset.cpp $(ROOT_DIR)/ain_imager_src/conf.h
	@mkdir -p $(BUILD_DIR)
	$(CXX) -fPIC -std=gnu++11 $< -o $@ $(AIN_INCLUDES) $(QT_FLAGS) $(QT_LIBS)

$(BUILD_DIR)/wcs_file_test: unit/wcs_file_test.cpp $(ROOT_DIR)/ain_platesolver_src/wcsfile.cpp $(ROOT_DIR)/ain_platesolver_src/wcsfile.h $(ROOT_DIR)/ain_platesolver_src/solverengine.h $(ROOT_DIR)/common_src/coordconv.c
	@mkdir -p $(BUILD_DIR)
	$(CC) -c -fPIC -O1 $(ROOT_DIR)/common_src/coordconv.c -o $(BUILD_DIR)/coordconv.o -I$(ROOT_DIR)/common_src
	$(CXX) -fPIC -O1 -std=gnu++11 -Wall unit/wcs_file_test.cpp $(ROOT_DIR)/ain_platesolver_src/wcsfile.cpp $(BUILD_DIR)/coordconv.o -o $@ $(PLATESOLVER_INCLUDES) $(QTCORE_FLAGS) $(QTCORE_LIBS)

test-unit: $(UNIT_TESTS)
	@for test in $(UNIT_TESTS); do echo "=== $$(basename $$test)"; $$test || exit 1; done

test: all
	INDIGO_ROOT="$(INDIGO_ROOT)" ./run_tests.sh $(CASES)

test-quick: all
	INDIGO_ROOT="$(INDIGO_ROOT)" ./run_tests.sh $(QUICK_CASES)

list:
	@./run_tests.sh -l

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test test-quick test-unit list clean
