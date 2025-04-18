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
		{SC_WAIT_UNTIL, {"Wait Until", {{0, {"Date Time (UTC)", DateTimeEdit}}}}},
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
		{SC_ENABLE_MERIDIAN_FLIP, {"Enable Meridian Flip", {{0, {"Use Solver", CheckBox}}, {1, {"Time (h)", DoubleSpinBox}}}}},
		{SC_DISABLE_MERIDIAN_FLIP, {"Disable Meridian Flip", {}}},
		{SC_SET_DIRECTORY, {"Set Directory (on server)", {{0, {"Directory", LineEdit}}}}},
		{SC_SET_OBJECT_NAME, {"Set Object Name", {{0, {"Name", LineEdit}}}}},
		{SC_CAPTURE_BATCH, {"Capture Batch", {{0, {"Count", SpinBox}}, {1, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_FOCUS, {"Focus", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_FOCUS_IGNORE_FAILURE, {"Focus (continue on failure)", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_CLEAR_FOCUS_SELECTION, {"Clear Focus Selection", {}}},
		{SC_SET_FOCUSER_POSITION, {"Set Focuser Position", {{0, {"Position", DoubleSpinBox}}}}},
		{SC_PARK, {"Park Mount", {}}},
		{SC_HOME, {"Home Mount", {}}},
		{SC_UNPARK, {"Unpark Mount", {}}},
		{SC_ENABLE_TRACKING, {"Enable Tracking", {}}},
		{SC_DISABLE_TRACKING, {"Disable Tracking", {}}},
		{SC_SLEW, {"Slew", {{0, {"RA", LineEditSG_RA}}, {1, {"Dec", LineEditSG_DEC}}}}},
		{SC_WAIT_FOR_GPS, {"Wait for GPS", {}}},
		{SC_CALIBRATE_GUIDING, {"Calibrate Guiding", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_START_GUIDING, {"Start Guiding", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_STOP_GUIDING, {"Stop Guiding", {}}},
		{SC_CLEAR_GUIDER_SELECTION, {"Clear Guider Selection", {}}},
		{SC_SYNC_CENTER, {"Sync Center", {{0, {"Exposure (s)", DoubleSpinBox}}}}},
		{SC_PRECISE_GOTO, {"Precise Goto", {{0, {"Exposure (s)", DoubleSpinBox}}, {1, {"RA", LineEditSG_RA}}, {2, {"Dec", LineEditSG_DEC}}}}},
		{SC_SET_ROTATOR_ANGLE, {"Set Rotator Angle", {{0, {"Angle(°)", DoubleSpinBox}}}}},
		{SC_REPEAT, {"Repeat", {{0, {"Count", SpinBox}}}}},
		{SC_CONTINUE_ON_FAILURE, {"Continue on Failure", {}}},
		{SC_ABORT_ON_FAILURE, {"Abort on Failure", {}}},
		{SC_RECOVER_ON_FAILURE, {"Recover on Failure", {}}},
		{SC_RECOVERY_POINT, {"Recovery Point", {}}},
		{SC_SET_FITS_HEADER, {"Set FITS Header", {{0, {"Keyword", LineEdit}}, {1, {"Value", LineEdit}}}}},
		{SC_REMOVE_FITS_HEADER, {"Remove FITS Header", {{0, {"Keyword", LineEdit}}}}},
		{SC_ENABLE_VERBOSE, {"Enable Verbose Logging", {}}},
		{SC_DISABLE_VERBOSE, {"Disable Verbose Logging", {}}}
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
		{CC_CAPTURE, {
			SC_SET_OBJECT_NAME,
			SC_CAPTURE_BATCH,
			__SEPARATOR__,
			SC_ENABLE_COOLER,
			SC_DISABLE_COOLER,
			__SEPARATOR__,
			SC_SELECT_FRAME_TYPE,
			SC_SELECT_CAMERA_MODE,
			SC_SELECT_IMAGE_FORMAT,
			__SEPARATOR__,
			SC_SET_GAIN,
			SC_SET_OFFSET,
			__SEPARATOR__,
			SC_SET_DIRECTORY
		}},
		{CC_FILTER_WHEEL, {
			SC_SELECT_FILTER
		}},
		{CC_FOCUSER, {
			SC_FOCUS,
			SC_FOCUS_IGNORE_FAILURE,
			SC_SET_FOCUSER_POSITION,
			SC_CLEAR_FOCUS_SELECTION
		}},
		{CC_GUIDER, {
			SC_CALIBRATE_GUIDING,
			SC_START_GUIDING,
			SC_STOP_GUIDING,
			__SEPARATOR__,
			SC_ENABLE_DITHERING,
			SC_DISABLE_DITHERING,
			__SEPARATOR__,
			SC_CLEAR_GUIDER_SELECTION
		}},
		{CC_MOUNT, {
			SC_SLEW,
			SC_PRECISE_GOTO,
			SC_SYNC_CENTER,
			__SEPARATOR__,
			SC_ENABLE_MERIDIAN_FLIP,
			SC_DISABLE_MERIDIAN_FLIP,
			__SEPARATOR__,
			SC_PARK,
			SC_UNPARK,
			SC_HOME,
			__SEPARATOR__,
			SC_ENABLE_TRACKING,
			SC_DISABLE_TRACKING
		}},
		{CC_ROTATOR, {
			SC_SET_ROTATOR_ANGLE
		}},
		{__SEPARATOR__, {}},
		{CC_FLOW_CONTROL, {
			SC_REPEAT,
			__SEPARATOR__,
			SC_CONTINUE_ON_FAILURE,
			SC_ABORT_ON_FAILURE,
			__SEPARATOR__,
			SC_RECOVER_ON_FAILURE,
			SC_RECOVERY_POINT
		}},
		{__SEPARATOR__, {}},
		{CC_DEVICES, {
			SC_SELECT_IMAGER_CAMERA,
			SC_SELECT_FILTER_WHEEL,
			SC_SELECT_FOCUSER,
			SC_SELECT_GUIDER_CAMERA,
			SC_SELECT_GUIDER,
			SC_SELECT_MOUNT,
			SC_SELECT_GPS,
			SC_SELECT_ROTATOR
		}},
		{CC_MISC, {
			SC_WAIT,
			SC_WAIT_UNTIL,
			SC_WAIT_FOR_GPS,
			__SEPARATOR__,
			SC_SEND_MESSAGE,
			__SEPARATOR__,
			SC_SET_FITS_HEADER,
			SC_REMOVE_FITS_HEADER,
			__SEPARATOR__,
			SC_LOAD_DRIVER,
			SC_UNLOAD_DRIVER,
			__SEPARATOR__,
			SC_ENABLE_VERBOSE,
			SC_DISABLE_VERBOSE,
			__SEPARATOR__,
			SC_LOAD_CONFIG
		}}
	};

	categoryIcons = {
		{CC_CAPTURE, QIcon(":resource/shutter-grey.png")},
		{CC_FILTER_WHEEL, QIcon(":resource/wheel-grey.png")},
		{CC_FLOW_CONTROL, QIcon(":resource/menu-loop-grey.png")},
		{CC_MOUNT, QIcon(":resource/mount-grey.png")},
		{CC_GUIDER, QIcon(":resource/guider-grey.png")},
		{CC_FOCUSER, QIcon(":resource/focuser-grey.png")},
		{CC_ROTATOR, QIcon(":resource/rotator-grey.png")},
		{CC_DEVICES, QIcon(":resource/server.png")},
		{CC_MISC, QIcon(":resource/led-grey-dev.png")}
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
	setNumericDefaultValue(SC_ENABLE_DITHERING, 0, 3.0);

	setNumericRange(SC_ENABLE_DITHERING, 1, 0, 360); // Time limit
	setNumericIncrement(SC_ENABLE_DITHERING, 1, 1.0);
	setNumericDefaultValue(SC_ENABLE_DITHERING, 1, 60.0);

	setNumericRange(SC_ENABLE_DITHERING, 2, 0, 10);  // Skip frames
	setNumericIncrement(SC_ENABLE_DITHERING, 2, 1.0);
	setNumericDefaultValue(SC_ENABLE_DITHERING, 2, 0.0);

	// Meridian flip
	setNumericRange(SC_ENABLE_MERIDIAN_FLIP, 1, -1, 1);
	setNumericIncrement(SC_ENABLE_MERIDIAN_FLIP, 1, 0.05);

	// Capture parameters
	setNumericRange(SC_CAPTURE_BATCH, 0, 0, 65535);    // Count
	setNumericIncrement(SC_CAPTURE_BATCH, 0, 1.0);
	setNumericDefaultValue(SC_CAPTURE_BATCH, 0, 1.0);

	setNumericRange(SC_CAPTURE_BATCH, 1, 0, 7200); // Exposure
	setNumericIncrement(SC_CAPTURE_BATCH, 1, 1.0);
	setNumericDefaultValue(SC_CAPTURE_BATCH, 1, 60.0);

	// Focus parameters
	setNumericRange(SC_FOCUS, 0, 0, 180.0);
	setNumericIncrement(SC_FOCUS, 0, 0.1);
	setNumericDefaultValue(SC_FOCUS, 0, 1.0);

	// Focus (continue on failure)
	setNumericRange(SC_FOCUS_IGNORE_FAILURE, 0, 0, 180.0);
	setNumericIncrement(SC_FOCUS_IGNORE_FAILURE, 0, 0.1);
	setNumericDefaultValue(SC_FOCUS_IGNORE_FAILURE, 0, 1.0);

	// Focuser position
	setNumericRange(SC_SET_FOCUSER_POSITION, 0, 0, 100000);
	setNumericIncrement(SC_SET_FOCUSER_POSITION, 0, 1.0);
	setNumericDefaultValue(SC_SET_FOCUSER_POSITION, 0, 0.0);

	// Guiding parameters
	setNumericRange(SC_CALIBRATE_GUIDING, 0, 0, 180.0);
	setNumericIncrement(SC_CALIBRATE_GUIDING, 0, 0.1);
	setNumericDefaultValue(SC_CALIBRATE_GUIDING, 0, 1.0);

	setNumericRange(SC_START_GUIDING, 0, 0, 180.0);
	setNumericIncrement(SC_START_GUIDING, 0, 0.1);
	setNumericDefaultValue(SC_START_GUIDING, 0, 1.0);

	// Sync and goto
	setNumericRange(SC_SYNC_CENTER, 0, 0, 180.0);
	setNumericIncrement(SC_SYNC_CENTER, 0, 1.0);
	setNumericDefaultValue(SC_SYNC_CENTER, 0, 1.0);

	setNumericRange(SC_PRECISE_GOTO, 0, 0, 80.0);
	setNumericIncrement(SC_PRECISE_GOTO, 0, 1.0);
	setNumericDefaultValue(SC_PRECISE_GOTO, 0, 1.0);

	// Repeat count
	setNumericRange(SC_REPEAT, 0, 1, 100000);
	setNumericIncrement(SC_REPEAT, 0, 1.0);
	setNumericDefaultValue(SC_REPEAT, 0, 1.0);

	// Rotator angle
	setNumericRange(SC_SET_ROTATOR_ANGLE, 0, -180, 360.0);
	setNumericIncrement(SC_SET_ROTATOR_ANGLE, 0, 1.0);
	setNumericDefaultValue(SC_SET_ROTATOR_ANGLE, 0, 0.0);

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

void SequenceItemModel::setNumericDefaultValue(const QString& type, int paramId, double value) {
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			widgetTypeMap[type].parameters[paramId].numericDefaultValue = value;
		}
	}
}

double SequenceItemModel::getNumericDefultValue(const QString& type, int paramId) const {
	if (widgetTypeMap.contains(type)) {
		if (widgetTypeMap[type].parameters.contains(paramId)) {
			return widgetTypeMap[type].parameters[paramId].numericDefaultValue;
		}
	}
	return 0.0; // Default value
}
