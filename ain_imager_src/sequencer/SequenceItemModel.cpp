// Copyright (c) 2025 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "SequenceItemModel.h"

//#include <QDebug>

SequenceItemModel& SequenceItemModel::instance() {
	static SequenceItemModel instance;
	return instance;
}

SequenceItemModel::SequenceItemModel() {
	initializeModel();
}

void SequenceItemModel::initializeModel() {
	// Initialize with existing types
	widgetTypeMap = {
		{SC_WAIT, {"Wait", {{0, {"Seconds", SpinBox}}}}},
		{SC_SEND_MESSAGE, {"Send Message", {{0, {"Message", LineEdit}}}}},
		{SC_LOAD_CONFIG, {"Load Config", {{0, {"Name", ComboBox}}}}},
		{SC_LOAD_DRIVER, {"Load Driver", {{0, {"Name", LineEdit}}}}},
		{SC_UNLOAD_DRIVER, {"Unload Driver", {{0, {"Name", LineEdit}}}}},
		{SC_SELECT_IMAGER_CAMERA, {"Select Imager Camera", {{0, {"Camera", ComboBox}}}}},
		{SC_SELECT_FILTER_WHEEL, {"Select Filter Wheel", {{0, {"Wheel", ComboBox}}}}},
		{SC_SELECT_FOCUSER, {"Select Focuser", {{0, {"Focuser", ComboBox}}}}},
		{SC_SELECT_ROTATOR, {"Select Rotator", {{0, {"Rotator", ComboBox}}}}},
		{SC_SELECT_MOUNT, {"Select Mount", {{0, {"Mount", ComboBox}}}}},
		{SC_SELECT_GPS, {"Select GPS", {{0, {"GPS", ComboBox}}}}},
		{SC_SELECT_GUIDER_CAMERA, {"Select Guider Camera", {{0, {"Camera", ComboBox}}}}},
		{SC_SELECT_GUIDER, {"Select Guider", {{0, {"Guider", ComboBox}}}}},
		{SC_SELECT_FRAME_TYPE, {"Select Frame Type", {{0, {"Frame Type", ComboBox}}}}},
		{SC_SELECT_IMAGE_FORMAT, {"Select Image Format", {{0, {"Image Format", ComboBox}}}}},
		{SC_SELECT_CAMERA_MODE, {"Select Camera Mode", {{0, {"Camera Mode", ComboBox}}}}},
		{SC_SELECT_FILTER, {"Select Filter", {{0, {"Filter", ComboBox}}}}},
		{SC_SET_GAIN, {"Set Camera Gain", {{0, {"Value", SpinBox}}}}},
		{SC_SET_OFFSET, {"Set Camera Offset", {{0, {"Value", SpinBox}}}}},
		{SC_ENABLE_COOLER, {"Enable Camera Cooler", {{0, {"Temperature (°C)", DoubleSpinBox}}}}},
		{SC_DISABLE_COOLER, {"Disable Camera Cooler", {}}},
		{SC_ENABLE_DITHERING, {"Enable Dithering", {{0, {"Amount (px)", SpinBox}}, {1, {"Time Limit (s)", SpinBox}}, {2, {"Skip Frames", SpinBox}}}}},
		{SC_DISABLE_DITHERING, {"Disable Dithering", {}}},
		{SC_ENABLE_MERIDIAN_FLIP, {"Enable Meridian Flip", {{0, {"Use Solver", CheckBox}}, {1, {"Time", SpinBox}}}}},
		{SC_DISABLE_MERIDIAN_FLIP, {"Disable Meridian Flip", {}}},
		{SC_SET_DIRECTORY, {"Set Directory (on server)", {{0, {"Directory", LineEdit}}}}},
		{SC_SET_OBJECT_NAME, {"Set Object Name", {{0, {"Name", LineEdit}}}}},
		{SC_CAPTURE_BATCH, {"Capture Batch", {{0, {"Count", SpinBox}}, {1, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_FOCUS, {"Focus", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_FOCUS_IGNORE_FAILURE, {"Focus (continue on failure)", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_CLEAR_FOCUS_SELECTION, {"Clear Focus Selection", {}}},
		{SC_PARK, {"Park", {}}},
		{SC_HOME, {"Home", {}}},
		{SC_UNPARK, {"Unpark", {}}},
		{SC_SLEW, {"Slew", {{0, {"RA", LineEditSG_RA}}, {1, {"Dec", LineEditSG_DEC}}}}},
		{SC_WAIT_FOR_GPS, {"Wait for GPS", {}}},
		{SC_CALIBRATE_GUIDING, {"Calibrate Guiding", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_START_GUIDING, {"Start Guiding", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_STOP_GUIDING, {"Stop Guiding", {}}},
		{SC_CLEAR_GUIDER_SELECTION, {"Clear Guider Selection", {}}},
		{SC_SYNC_CENTER, {"Sync Center", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_PRECISE_GOTO, {"Precise Goto", {{0, {"Exposure (s)", DoubleSpinBox}}, {1, {"RA", LineEditSG_RA}}, {2, {"Dec", LineEditSG_DEC}}}}},
		{SC_SET_ROTATOR_ANGLE, {"Set Rotator Angle", {{0, {"Angle(°)", DoubleSpinBox}}}}},
		{SC_REPEAT, {"Repeat", {{0, {"Count", SpinBox}}}}}
	};

	//{"select_imager_agent", {"Select Imager Agent", {{0, {"Agent", "QComboBox"}}}}},
	//{"select_mount_agent", {"Select Mount Agent", {{0, {"Agent", "QComboBox"}}}}},
	//{"select_guider_agent", {"Select Guider Agent", {{0, {"Agent", "QComboBox"}}}}},
	//{"select_dome", {"Select Dome", {{0, {"Dome", "QComboBox"}}}}},
	//{"set_gamma", {"Set Gamma", {{0, {"Value", "QSpinBox"}}}}},
	//{"select_program", {"Select Program", {{0, {"Name", "QComboBox"}}}}},
	//{"select_aperture", {"Select Aperture", {{0, {"Name", "QComboBox"}}}}},
	//{"select_shutter", {"Select Shutter", {{0, {"Name", "QComboBox"}}}}},
	//{"select_iso", {"Select ISO", {{0, {"Name", "QComboBox"}}}}},

	categoriesList = {
		{"Capture", {
			SC_SELECT_CAMERA_MODE,
			SC_SELECT_IMAGE_FORMAT,
			SC_SELECT_FRAME_TYPE,
			SC_SET_DIRECTORY,
			SC_SET_OBJECT_NAME,
			SC_SET_GAIN,
			SC_SET_OFFSET,
			SC_ENABLE_COOLER,
			SC_DISABLE_COOLER,
			SC_CAPTURE_BATCH
		}},
		{"Filter", {
			SC_SELECT_FILTER
		}},
		{"Loop", {
			SC_REPEAT
		}},
		{"Mount", {
			SC_SLEW,
			SC_PARK,SC_UNPARK,
			SC_HOME,
			SC_SYNC_CENTER,
			SC_PRECISE_GOTO,
			SC_ENABLE_MERIDIAN_FLIP,
			SC_DISABLE_MERIDIAN_FLIP
		}},
		{"Guiding", {
			SC_CALIBRATE_GUIDING,
			SC_START_GUIDING,
			SC_STOP_GUIDING,
			SC_CLEAR_GUIDER_SELECTION,
			SC_ENABLE_DITHERING,
			SC_DISABLE_DITHERING
		}},
		{"Focuser", {
			SC_FOCUS,
			SC_FOCUS_IGNORE_FAILURE,
			SC_CLEAR_FOCUS_SELECTION
		}},
		{"Rotator", {
			SC_SET_ROTATOR_ANGLE
		}},
		{"Devices", {
			SC_SELECT_FOCUSER,
			SC_SELECT_GPS,
			SC_SELECT_GUIDER_CAMERA,
			SC_SELECT_GUIDER,
			SC_SELECT_MOUNT,
			SC_SELECT_ROTATOR,
			SC_SELECT_IMAGER_CAMERA,
			SC_SELECT_FILTER_WHEEL
		}},
		{"Other", {
			SC_WAIT,
			SC_SEND_MESSAGE,
			SC_LOAD_CONFIG,
			SC_LOAD_DRIVER,
			SC_UNLOAD_DRIVER,
			SC_WAIT_FOR_GPS
		}}
	};

	categoryIcons = {
		{"Capture", QIcon(":resource/shutter-grey.png")},
		{"Filter", QIcon(":resource/wheel-grey.png")},
		{"Loop", QIcon(":resource/menu-loop-grey.png")},
		{"Mount", QIcon(":resource/mount-grey.png")},
		{"Guiding", QIcon(":resource/guider-grey.png")},
		{"Focuser", QIcon(":resource/focuser-grey.png")},
		{"Rotator", QIcon(":resource/rotator-grey.png")},
		{"Devices", QIcon(":resource/server.png")},
		{"Other", QIcon(":resource/led-grey-dev.png")}
	};

	// Set combo options
	setComboOptions(SC_SELECT_FRAME_TYPE, 0, {"Light", "Bias", "Dark", "Flat", "Dark Flat"});
	setComboOptions(SC_SELECT_IMAGE_FORMAT, 0, {"FITS format" , "XISF format", "Raw Data", "JPEG format", "TIFF format", "PNG format"});

	// Set default ranges and increments

	// Time-related parameters
	setNumericRange(SC_WAIT, 0, 0, 3600);
	setNumericIncrement(SC_WAIT, 0, 1.0);

	// Camera settings
	setNumericRange(SC_SET_GAIN, 0, -1000, 10000);
	setNumericIncrement(SC_SET_GAIN, 0, 1.0);

	setNumericRange(SC_SET_OFFSET, 0, 0, 10000);
	setNumericIncrement(SC_SET_OFFSET, 0, 1.0);

	// Temperature control
	setNumericRange(SC_ENABLE_COOLER, 0, -200.0, 50.0);
	setNumericIncrement(SC_ENABLE_COOLER, 0, 0.5);

	// Dithering parameters
	setNumericRange(SC_ENABLE_DITHERING, 0, 0, 15);  // Amount
	setNumericIncrement(SC_ENABLE_DITHERING, 0, 1.0);
	setNumericRange(SC_ENABLE_DITHERING, 1, 0, 360); // Time limit
	setNumericIncrement(SC_ENABLE_DITHERING, 1, 1.0);
	setNumericRange(SC_ENABLE_DITHERING, 2, 0, 10);  // Skip frames
	setNumericIncrement(SC_ENABLE_DITHERING, 2, 1.0);

	// Meridian flip
	setNumericRange(SC_ENABLE_MERIDIAN_FLIP, 1, 0, 3600);
	setNumericIncrement(SC_ENABLE_MERIDIAN_FLIP, 1, 1.0);

	// Capture parameters
	setNumericRange(SC_CAPTURE_BATCH, 0, -1, 65535);    // Count
	setNumericIncrement(SC_CAPTURE_BATCH, 0, 1.0);
	setNumericRange(SC_CAPTURE_BATCH, 1, 0, 7200); // Exposure
	setNumericIncrement(SC_CAPTURE_BATCH, 1, 1.0);

	// Focus parameters
	setNumericRange(SC_FOCUS, 0, 0, 180.0);
	setNumericIncrement(SC_FOCUS, 0, 0.1);

	// Focus (continue on failure)
	setNumericRange(SC_FOCUS_IGNORE_FAILURE, 0, 0, 180.0);
	setNumericIncrement(SC_FOCUS_IGNORE_FAILURE, 0, 0.1);

	// Guiding parameters
	setNumericRange(SC_CALIBRATE_GUIDING, 0, 0, 180.0);
	setNumericIncrement(SC_CALIBRATE_GUIDING, 0, 0.1);
	setNumericRange(SC_START_GUIDING, 0, 0, 180.0);
	setNumericIncrement(SC_START_GUIDING, 0, 0.1);

	// Sync and goto
	setNumericRange(SC_SYNC_CENTER, 0, 0, 180.0);
	setNumericIncrement(SC_SYNC_CENTER, 0, 1.0);
	setNumericRange(SC_PRECISE_GOTO, 0, 0, 80.0);
	setNumericIncrement(SC_PRECISE_GOTO, 0, 1.0);

	// Repeat count
	setNumericRange(SC_REPEAT, 0, 1, 100000);
	setNumericIncrement(SC_REPEAT, 0, 1.0);

	// Rotator angle
	setNumericRange(SC_SET_ROTATOR_ANGLE, 0, -180, 360.0);
	setNumericIncrement(SC_SET_ROTATOR_ANGLE, 0, 1.0);

	//qDebug() << "Initialized SequenceItemModel with" << widgetTypeMap.size() << "widget types";
}

const QMap<QString, SequenceItemModel::WidgetTypeInfo>& SequenceItemModel::getWidgetTypes() const {
	return widgetTypeMap;
}

const QLinkedList<QPair<QString, QStringList>>& SequenceItemModel::getCategories() const {
	return categoriesList;
}

const QMap<QString, QIcon>& SequenceItemModel::getCategoryIcons() const {
	return categoryIcons;
}

void SequenceItemModel::setComboOptions(const QString& type, int paramId, const QStringList& options) {
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			widgetTypeMap[type].parameters[paramId].comboOptions = options;
			emit comboOptionsChanged(type, paramId, options);
		}
	}
}

void SequenceItemModel::clearComboOptions(const QString& type, int paramId) {
	setComboOptions(type, paramId, QStringList());
}

QStringList SequenceItemModel::getComboOptions(const QString& type, int paramId) const {
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			return widgetTypeMap[type].parameters[paramId].comboOptions;
		}
	}
	return QStringList();
}

void SequenceItemModel::setNumericRange(const QString& type, int paramId, double min, double max) {
	//qDebug() << "Setting numeric range for" << type << "parameter" << paramId << "to" << min << "to" << max;
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			widgetTypeMap[type].parameters[paramId].numericRange = qMakePair(min, max);
			emit numericRangeChanged(type, paramId, min, max);
		}
	}
}

QPair<double, double> SequenceItemModel::getNumericRange(const QString& type, int paramId) const {
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			return widgetTypeMap[type].parameters[paramId].numericRange;
		}
	}
	return qMakePair(0.0, 100.0);
}

void SequenceItemModel::setNumericIncrement(const QString& type, int paramId, double increment) {
	//qDebug() << "Setting numeric increment for" << type << "parameter" << paramId << "to" << increment;
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			widgetTypeMap[type].parameters[paramId].numericIncrement = increment;
			emit numericIncrementChanged(type, paramId, increment);
		}
	}
}

double SequenceItemModel::getNumericIncrement(const QString& type, int paramId) const {
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			return widgetTypeMap[type].parameters[paramId].numericIncrement;
		}
	}
	return 1.0; // Default increment
}
