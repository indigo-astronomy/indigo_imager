#include "SequenceItemModel.h"

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
		{"wait", {"Wait", {{0, {"Seconds", "QSpinBox"}}}}},
		{"send_message", {"Send Message", {{0, {"Message", "QLineEdit"}}}}},
		{"load_config", {"Load Config", {{0, {"Name", "QComboBox"}}}}},
		{"load_driver", {"Load Driver", {{0, {"Name", "QLineEdit"}}}}},
		{"unload_driver", {"Unload Driver", {{0, {"Name", "QLineEdit"}}}}},
		//{"select_imager_agent", {"Select Imager Agent", {{0, {"Agent", "QComboBox"}}}}},
		//{"select_mount_agent", {"Select Mount Agent", {{0, {"Agent", "QComboBox"}}}}},
		//{"select_guider_agent", {"Select Guider Agent", {{0, {"Agent", "QComboBox"}}}}},
		{"select_imager_camera", {"Select Imager Camera", {{0, {"Camera", "QComboBox"}}}}},
		{"select_filter_wheel", {"Select Filter Wheel", {{0, {"Wheel", "QComboBox"}}}}},
		{"select_focuser", {"Select Focuser", {{0, {"Focuser", "QComboBox"}}}}},
		{"select_rotator", {"Select Rotator", {{0, {"Rotator", "QComboBox"}}}}},
		{"select_mount", {"Select Mount", {{0, {"Mount", "QComboBox"}}}}},
		//{"select_dome", {"Select Dome", {{0, {"Dome", "QComboBox"}}}}},
		{"select_gps", {"Select GPS", {{0, {"GPS", "QComboBox"}}}}},
		{"select_guider_camera", {"Select Guider Camera", {{0, {"Camera", "QComboBox"}}}}},
		{"select_guider", {"Select Guider", {{0, {"Guider", "QComboBox"}}}}},
		{"select_frame_type_by_label", {"Select Frame Type", {{0, {"Frame Type", "QComboBox"}}}}},
		{"select_image_format_by_label", {"Select Image Format", {{0, {"Image Format", "QComboBox"}}}}},
		{"select_camera_mode_by_label", {"Select Camera Mode", {{0, {"Camera Mode", "QComboBox"}}}}},
		{"select_filter_by_label", {"Select Filter", {{0, {"Filter", "QComboBox"}}}}},
		{"set_gain", {"Set Gain", {{0, {"Value", "QSpinBox"}}}}},
		{"set_offset", {"Set Offset", {{0, {"Value", "QSpinBox"}}}}},
		{"set_gamma", {"Set Gamma", {{0, {"Value", "QSpinBox"}}}}},
		//{"select_program", {"Select Program", {{0, {"Name", "QComboBox"}}}}},
		//{"select_aperture", {"Select Aperture", {{0, {"Name", "QComboBox"}}}}},
		//{"select_shutter", {"Select Shutter", {{0, {"Name", "QComboBox"}}}}},
		//{"select_iso", {"Select ISO", {{0, {"Name", "QComboBox"}}}}},
		{"enable_cooler", {"Enable Cooler", {{0, {"Temperature", "QDoubleSpinBox"}}}}},
		{"disable_cooler", {"Disable Cooler", {}}},
		{"enable_dithering", {"Enable Dithering", {{0, {"Amount", "QSpinBox"}}, {1, {"Time Limit", "QSpinBox"}}, {2, {"Skip Frames", "QSpinBox"}}}}},
		{"disable_dithering", {"Disable Dithering", {}}},
		{"enable_meridian_flip", {"Enable Meridian Flip", {{0, {"Use Solver", "QCheckBox"}}, {1, {"Time", "QSpinBox"}}}}},
		{"disable_meridian_flip", {"Disable Meridian Flip", {}}},
		{"set_directory", {"Set Directory", {{0, {"Directory", "QLineEdit"}}}}},
		{"set_object_name", {"Set Object Name", {{0, {"Name", "QLineEdit"}}}}},
		{"capture_batch", {"Capture Batch", {{0, {"Count", "QSpinBox"}}, {1, {"Exposure", "QDoubleSpinBox"}}}}},
		{"focus", {"Focus", {{0, {"Exposure", "QDoubleSpinBox"}}}}},
		{"focus_ignore_failure", {"Focus (continue on failure)", {{0, {"Exposure", "QDoubleSpinBox"}}}}},
		{"clear_focus_selection", {"Clear Focus Selection", {}}},
		{"park", {"Park", {}}},
		{"home", {"Home", {}}},
		{"unpark", {"Unpark", {}}},
		{"slew", {"Slew", {{0, {"RA", "QLineEditSG-RA"}}, {1, {"Dec", "QLineEditSG-DEC"}}}}},
		{"wait_for_gps", {"Wait for GPS", {}}},
		{"calibrate_guiding", {"Calibrate Guiding", {{0, {"Exposure", "QDoubleSpinBox"}}}}},
		{"start_guiding", {"Start Guiding", {{0, {"Exposure", "QDoubleSpinBox"}}}}},
		{"stop_guiding", {"Stop Guiding", {}}},
		{"clear_guider_selection", {"Clear Guider Selection", {}}},
		{"sync_center", {"Sync Center", {{0, {"Exposure", "QDoubleSpinBox"}}}}},
		{"precise_goto", {"Precise Goto", {{0, {"Exposure", "QDoubleSpinBox"}}, {1, {"RA", "QLineEditSG-RA"}}, {2, {"Dec", "QLineEditSG-DEC"}}}}},
		{"repeat", {"Repeat", {{0, {"Count", "QSpinBox"}}}}}
	};

	// Set combo options
	setComboOptions("select_frame_type_by_label", 0, {"Light", "Bias", "Dark", "Flat", "Dark Flat"});
	setComboOptions("select_image_format_by_label", 0, {"FITS format" , "XISF format", "Raw Data", "JPEG format", "TIFF format", "PNG format"});

	// Set default ranges and increments

	// Time-related parameters
	setNumericRange("wait", 0, 0, 3600);
	setNumericIncrement("wait", 0, 1.0);

	// Camera settings
	setNumericRange("set_gain", 0, 0, 1000);
	setNumericIncrement("set_gain", 0, 1.0);

	setNumericRange("set_offset", 0, 0, 1000);
	setNumericIncrement("set_offset", 0, 1.0);

	setNumericRange("set_gamma", 0, 0, 100);
	setNumericIncrement("set_gamma", 0, 1.0);

	// Temperature control
	setNumericRange("enable_cooler", 0, -150.0, 30.0);
	setNumericIncrement("enable_cooler", 0, 0.5);

	// Dithering parameters
	setNumericRange("enable_dithering", 0, 0, 15);  // Amount
	setNumericIncrement("enable_dithering", 0, 1.0);
	setNumericRange("enable_dithering", 1, 0, 360); // Time limit
	setNumericIncrement("enable_dithering", 1, 1.0);
	setNumericRange("enable_dithering", 2, 0, 10);  // Skip frames
	setNumericIncrement("enable_dithering", 2, 1.0);

	// Meridian flip
	setNumericRange("enable_meridian_flip", 1, 0, 3600);
	setNumericIncrement("enable_meridian_flip", 1, 1.0);

	// Capture parameters
	setNumericRange("capture_batch", 1, -1, 65535);    // Count
	setNumericIncrement("capture_batch", 1, 1.0);
	setNumericRange("capture_batch", 2, 0, 3600.0); // Exposure
	setNumericIncrement("capture_batch", 2, 1.0);

	// Focus parameters
	setNumericRange("focus", 0, 0, 60.0);
	setNumericIncrement("focus", 0, 0.1);

	// Focus (continue on failure)
	setNumericRange("focus_ignore_failure", 0, 0, 60.0);
	setNumericIncrement("focus_ignore_failure", 0, 0.1);

	// Guiding parameters
	setNumericRange("calibrate_guiding_exposure", 0, 0, 60.0);
	setNumericIncrement("calibrate_guiding_exposure", 0, 0.1);
	setNumericRange("start_guiding_exposure", 0, 0, 60.0);
	setNumericIncrement("start_guiding_exposure", 0, 0.1);

	// Sync and goto
	setNumericRange("sync_center", 0, 0, 60.0);
	setNumericIncrement("sync_center", 0, 1.0);
	setNumericRange("precise_goto", 0, 0, 60.0);
	setNumericIncrement("precise_goto", 0, 1.0);

	// Repeat count
	setNumericRange("repeat", 0, 1, 1000);
	setNumericIncrement("repeat", 0, 1.0);
}

const QMap<QString, SequenceItemModel::WidgetTypeInfo>& SequenceItemModel::getWidgetTypes() const {
	return widgetTypeMap;
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

