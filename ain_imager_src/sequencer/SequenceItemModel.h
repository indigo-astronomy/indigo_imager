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

#ifndef __SEQUENCEITEMMODEL_H
#define __SEQUENCEITEMMODEL_H

#include <QString>
#include <QMap>
#include <QLinkedList>
#include <QPair>
#include <QStringList>
#include <QPair>
#include <QObject>
#include <QIcon>

#define cs(str) (const char*)(str)
// Sequence command constants
#define SC_WAIT                     cs("wait")
#define SC_WAIT_UNTIL               cs("wait_until")
#define SC_SEND_MESSAGE             cs("send_message")
#define SC_LOAD_CONFIG              cs("load_config")
#define SC_LOAD_DRIVER              cs("load_driver")
#define SC_UNLOAD_DRIVER            cs("unload_driver")
#define SC_SELECT_IMAGER_CAMERA     cs("select_imager_camera")
#define SC_SELECT_FILTER_WHEEL      cs("select_filter_wheel")
#define SC_SELECT_FOCUSER           cs("select_focuser")
#define SC_SELECT_ROTATOR           cs("select_rotator")
#define SC_SELECT_MOUNT             cs("select_mount")
#define SC_SELECT_GPS               cs("select_gps")
#define SC_SELECT_GUIDER_CAMERA     cs("select_guider_camera")
#define SC_SELECT_GUIDER            cs("select_guider")
#define SC_SELECT_FRAME_TYPE        cs("select_frame_type")
#define SC_SELECT_IMAGE_FORMAT      cs("select_image_format")
#define SC_SELECT_CAMERA_MODE       cs("select_camera_mode")
#define SC_SELECT_FILTER            cs("select_filter")
#define SC_SET_GAIN                 cs("set_gain")
#define SC_SET_OFFSET               cs("set_offset")
#define SC_ENABLE_COOLER            cs("enable_cooler")
#define SC_DISABLE_COOLER           cs("disable_cooler")
#define SC_ENABLE_DITHERING         cs("enable_dithering")
#define SC_DISABLE_DITHERING        cs("disable_dithering")
#define SC_ENABLE_MERIDIAN_FLIP     cs("enable_meridian_flip")
#define SC_DISABLE_MERIDIAN_FLIP    cs("disable_meridian_flip")
#define SC_SET_DIRECTORY            cs("set_directory")
#define SC_SET_OBJECT_NAME          cs("set_object_name")
#define SC_CAPTURE_BATCH            cs("capture_batch")
#define SC_FOCUS                    cs("focus")
#define SC_FOCUS_IGNORE_FAILURE     cs("focus_ignore_failure")
#define SC_CLEAR_FOCUS_SELECTION    cs("clear_focus_selection")
#define SC_SET_FOCUSER_POSITION     cs("set_focuser_position")
#define SC_PARK                     cs("park")
#define SC_HOME                     cs("home")
#define SC_UNPARK                   cs("unpark")
#define SC_ENABLE_TRACKING          cs("enable_tracking")
#define SC_DISABLE_TRACKING         cs("disable_tracking")
#define SC_SLEW                     cs("slew")
#define SC_WAIT_FOR_GPS             cs("wait_for_gps")
#define SC_CALIBRATE_GUIDING        cs("calibrate_guiding")
#define SC_START_GUIDING            cs("start_guiding")
#define SC_STOP_GUIDING             cs("stop_guiding")
#define SC_CLEAR_GUIDER_SELECTION   cs("clear_guider_selection")
#define SC_SYNC_CENTER              cs("sync_center")
#define SC_PRECISE_GOTO             cs("precise_goto")
#define SC_SET_ROTATOR_ANGLE        cs("set_rotator_angle")
#define SC_REPEAT                   cs("repeat")
#define SC_SET_FITS_HEADER          cs("set_fits_header")
#define SC_REMOVE_FITS_HEADER       cs("remove_fits_header")
#define SC_CONTINUE_ON_FAILURE      cs("continue_on_failure")
#define SC_ABORT_ON_FAILURE         cs("abort_on_failure")
#define SC_RECOVER_ON_FAILURE       cs("recover_on_failure")
#define SC_RECOVERY_POINT           cs("recovery_point")
#define SC_ENABLE_VERBOSE           cs("enable_verbose")
#define SC_DISABLE_VERBOSE          cs("disable_verbose")

#define CC_CAPTURE                  cs("Capture")
#define CC_FILTER_WHEEL             cs("Filter Wheel")
#define CC_FLOW_CONTROL             cs("Flow Control")
#define CC_MOUNT                    cs("Mount")
#define CC_GUIDER                   cs("Guider")
#define CC_FOCUSER                  cs("Focuser")
#define CC_ROTATOR                  cs("Rotator")
#define CC_DEVICES                  cs("Devices")
#define CC_MISC                     cs("Misc")

#define __SEPARATOR__               cs("__separator__")

#define DATE_TIME_FORMAT            cs("yyyy-MM-dd hh:mm:ss")

enum ParamWidget {
	LineEdit,
	SpinBox,
	DoubleSpinBox,
	ComboBox,
	CheckBox,
	LineEditSG_RA,
	LineEditSG_DEC,
	DateTimeEdit
};

class SequenceItemModel : public QObject {
	Q_OBJECT
public:
	struct ParameterInfo {
		QString label;
		ParamWidget paramWidget;
		QStringList comboOptions;
		QPair<double, double> numericRange;
		double numericDefaultValue;
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
	const QLinkedList<QPair<QString, QStringList>>& getCategories() const;
	const QMap<QString, QIcon>& getCategoryIcons() const;

	void setComboOptions(const QString& type, int paramId, const QStringList& options);
	void clearComboOptions(const QString& type, int paramId);
	QStringList getComboOptions(const QString& type, int paramId) const;
	void setNumericRange(const QString& type, int paramId, double min, double max);
	QPair<double, double> getNumericRange(const QString& type, int paramId) const;
	void setNumericIncrement(const QString& type, int paramId, double increment);
	double getNumericIncrement(const QString& type, int paramId) const;
	void setNumericDefaultValue(const QString& type, int paramId, double value);
	double getNumericDefultValue(const QString& type, int paramId) const;

signals:
	void numericRangeChanged(const QString& type, int paramId, double min, double max);
	void numericIncrementChanged(const QString& type, int paramId, double increment);
	void comboOptionsChanged(const QString& type, int paramId, const QStringList& options);

private:
	SequenceItemModel();
	void initializeModel();
	QMap<QString, WidgetTypeInfo> widgetTypeMap;
	QLinkedList<QPair<QString, QStringList>> categoriesList;

	QMap<QString, QIcon> categoryIcons;
};

#endif // __SEQUENCEITEMMODEL_H