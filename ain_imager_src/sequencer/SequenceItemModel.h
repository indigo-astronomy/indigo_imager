#ifndef __SEQUENCEITEMMODEL_H
#define __SEQUENCEITEMMODEL_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QPair>
#include <QObject>

// Sequence command constants
#define SC_WAIT "wait"
#define SC_SEND_MESSAGE "send_message"
#define SC_LOAD_CONFIG "load_config"
#define SC_LOAD_DRIVER "load_driver"
#define SC_UNLOAD_DRIVER "unload_driver"
#define SC_SELECT_IMAGER_CAMERA "select_imager_camera"
#define SC_SELECT_FILTER_WHEEL "select_filter_wheel"
#define SC_SELECT_FOCUSER "select_focuser"
#define SC_SELECT_ROTATOR "select_rotator"
#define SC_SELECT_MOUNT "select_mount"
#define SC_SELECT_GPS "select_gps"
#define SC_SELECT_GUIDER_CAMERA "select_guider_camera"
#define SC_SELECT_GUIDER "select_guider"
#define SC_SELECT_FRAME_TYPE "select_frame_type"
#define SC_SELECT_IMAGE_FORMAT "select_image_format"
#define SC_SELECT_CAMERA_MODE "select_camera_mode"
#define SC_SELECT_FILTER "select_filter"
#define SC_SET_GAIN "set_gain"
#define SC_SET_OFFSET "set_offset"
#define SC_ENABLE_COOLER "enable_cooler"
#define SC_DISABLE_COOLER "disable_cooler"
#define SC_ENABLE_DITHERING "enable_dithering"
#define SC_DISABLE_DITHERING "disable_dithering"
#define SC_ENABLE_MERIDIAN_FLIP "enable_meridian_flip"
#define SC_DISABLE_MERIDIAN_FLIP "disable_meridian_flip"
#define SC_SET_DIRECTORY "set_directory"
#define SC_SET_OBJECT_NAME "set_object_name"
#define SC_CAPTURE_BATCH "capture_batch"
#define SC_FOCUS "focus"
#define SC_FOCUS_IGNORE_FAILURE "focus_ignore_failure"
#define SC_CLEAR_FOCUS_SELECTION "clear_focus_selection"
#define SC_PARK "park"
#define SC_HOME "home"
#define SC_UNPARK "unpark"
#define SC_SLEW "slew"
#define SC_WAIT_FOR_GPS "wait_for_gps"
#define SC_CALIBRATE_GUIDING "calibrate_guiding"
#define SC_START_GUIDING "start_guiding"
#define SC_STOP_GUIDING "stop_guiding"
#define SC_CLEAR_GUIDER_SELECTION "clear_guider_selection"
#define SC_SYNC_CENTER "sync_center"
#define SC_PRECISE_GOTO "precise_goto"
#define SC_REPEAT "repeat"

enum ParamWidget {
	LineEdit,
	SpinBox,
	DoubleSpinBox,
	ComboBox,
	CheckBox,
	LineEditSG_RA,
	LineEditSG_DEC
};

class SequenceItemModel : public QObject {
	Q_OBJECT
public:
	struct ParameterInfo {
		QString label;
		ParamWidget paramWidget;
		QStringList comboOptions;
		QPair<double, double> numericRange;
		double numericIncrement;

		ParameterInfo(const QString& l = "", ParamWidget w = LineEdit) : label(l), paramWidget(w), numericRange(0.0, 100.0), numericIncrement(1.0) {}
	};

	struct WidgetTypeInfo {
		QString label;
		QMap<int, ParameterInfo> parameters;
	};

	static SequenceItemModel& instance();

	// Delete copy constructor and assignment operator
	SequenceItemModel(const SequenceItemModel&) = delete;
	SequenceItemModel& operator=(const SequenceItemModel&) = delete;

	const QMap<QString, WidgetTypeInfo>& getWidgetTypes() const;
	void setComboOptions(const QString& type, int paramId, const QStringList& options);
	void clearComboOptions(const QString& type, int paramId);
	QStringList getComboOptions(const QString& type, int paramId) const;
	void setNumericRange(const QString& type, int paramId, double min, double max);
	QPair<double, double> getNumericRange(const QString& type, int paramId) const;
	void setNumericIncrement(const QString& type, int paramId, double increment);
	double getNumericIncrement(const QString& type, int paramId) const;

signals:
	void numericRangeChanged(const QString& type, int paramId, double min, double max);
	void numericIncrementChanged(const QString& type, int paramId, double increment);
	void comboOptionsChanged(const QString& type, int paramId, const QStringList& options);

private:
	SequenceItemModel();
	void initializeModel();
	QMap<QString, WidgetTypeInfo> widgetTypeMap;
};

#endif // __SEQUENCEITEMMODEL_H