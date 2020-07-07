// Copyright (c) 2019 Rumen G.Bogdanovski & David Hulse
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


#ifndef IMAGERWINDOW_H
#define IMAGERWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <QComboBox>
#include <indigo/indigo_bus.h>
#include "image-viewer.h"

class QServiceModel;
class QIndigoServers;

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QMenuBar>
#include <QProgressBar>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QActionGroup>
#include <QLineEdit>
#include <QCheckBox>
#include <QStandardPaths>
#include <QDir>


class ImagerWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit ImagerWindow(QWidget *parent = nullptr);
	virtual ~ImagerWindow();
	void property_define_delete(indigo_property* property, char *message, bool action_deleted);

	bool get_selected_agent(char * selected_agent) {
		if (!selected_agent || !m_agent_select) return false;
		strncpy(selected_agent, m_agent_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		indigo_debug("SELECTED AGENT = %s", selected_agent);
		return true;
	};

	void property_delete(indigo_property* property, char *message);
	void property_define(indigo_property* property, char *message);
signals:
	void enable_blobs(bool on);
	void rebuild_blob_previews();

public slots:
	void on_start(bool clicked);
	void on_preview(bool clicked);
	void on_abort(bool clicked);
	void on_pause(bool clicked);
	void on_window_log(indigo_property* property, char *message);
	void on_property_define(indigo_property* property, char *message);
	void on_property_change(indigo_property* property, char *message);
	void on_property_delete(indigo_property* property, char *message);
	void on_message_sent(indigo_property* property, char *message);
	void on_blobs_changed(bool status);
	void on_bonjour_changed(bool status);
	void on_use_suffix_changed(bool status);
	void on_use_state_icons_changed(bool status);
	void on_use_system_locale_changed(bool status);
	void on_log_error();
	void on_log_info();
	void on_log_debug();
	void on_log_trace();
	void on_servers_act();
	void on_exit_act();
	void on_about_act();
	void on_no_stretch();
	void on_normal_stretch();
	void on_hard_stretch();
	void on_create_preview(indigo_property *property, indigo_item *item);
	void on_obsolete_preview(indigo_property *property, indigo_item *item);
	void on_remove_preview(indigo_property *property, indigo_item *item);

	void on_agent_selected(int index);
	void on_wheel_selected(int index);
	void on_focuser_selected(int index);
	void on_camera_selected(int index);
	void on_ccd_mode_selected(int index);
	void on_frame_type_selected(int index);
	void on_filter_selected(int index);
	void on_cooler_onoff(bool state);
	void on_teperature_set();

	void on_focus_start(bool clicked);
	void on_selection_changed(int value);
	void on_image_right_click(int x, int y);
	void on_focus_in(bool clicked);
	void on_focus_out(bool clicked);


private:
	bool m_preview;
	bool m_focusing;
	QPlainTextEdit* mLog;

	// Capture tab
	QComboBox *m_agent_select;
	QComboBox *m_camera_select;
	QComboBox *m_wheel_select;
	QComboBox *m_frame_type_select;
	QComboBox *m_frame_size_select;
	QSpinBox  *m_roi_x, *m_roi_w;
	QSpinBox  *m_roi_y, *m_roi_h;
	QDoubleSpinBox *m_exposure_time;
	QDoubleSpinBox *m_exposure_delay;
	QSpinBox *m_frame_count;
	QComboBox *m_filter_select;
	QSpinBox *m_frame_delay;
	QLineEdit *m_object_name;
	QPushButton *m_pause_button;
	QProgressBar *m_exposure_progress;
	QProgressBar *m_process_progress;
	QDoubleSpinBox *m_set_temp;
	QLineEdit *m_current_temp;
	QLineEdit *m_cooler_pwr;
	QCheckBox *m_cooler_onoff;

	// Focuser tabbar
	QComboBox *m_focuser_select;
	QComboBox *m_focus_method_select;
	QSpinBox  *m_star_x;
	QSpinBox  *m_star_y;
	QSpinBox  *m_initial_step;
	QSpinBox  *m_final_step;
	QSpinBox  *m_focus_backlash;
	QSpinBox  *m_focus_stack;
	QSpinBox  *m_focus_position;
	QSpinBox  *m_focus_steps;
	QLabel    *m_FWHM_label;
	QLabel    *m_HFD_label;
	QLabel    *m_peak_label;
	QLabel    *m_drift_label;

	QDoubleSpinBox *m_focuser_exposure_time;

	pal::ImageViewer *m_viewer;

	QIndigoServers *mIndigoServers;
	QServiceModel *mServiceModel;

	void create_focuser_tab(QFrame *capture_frame);
	void create_imager_tab(QFrame *camera_frame);
	void change_ccd_frame_property(const char *agent) const;
	void change_ccd_exposure_property(const char *agent) const;
	void change_ccd_abort_exposure_property(const char *agent) const;
	void change_ccd_mode_property(const char *agent) const;
	void change_ccd_frame_type_property(const char *agent) const;
	void change_agent_batch_property(const char *agent) const;
	void change_agent_start_exposure_property(const char *agent) const;
	void change_agent_pause_process_property(const char *agent) const;
	void change_agent_abort_process_property(const char *agent) const;
	void change_wheel_slot_property(const char *agent) const;
	void change_cooler_onoff_property(const char *agent) const;
	void change_ccd_temperature_property(const char *agent) const;

	void change_agent_start_preview_property(const char *agent) const;
	void change_agent_start_focusing_property(const char *agent) const;
	void change_agent_star_selection(const char *agent) const;
	void change_agent_focus_params_property(const char *agent) const;
	void change_focuser_steps_property(const char *agent) const;
	void change_focuser_focus_in_property(const char *agent) const;
	void change_focuser_focus_out_property(const char *agent) const;

	bool save_blob_item_with_prefix(indigo_item *item, const char *prefix, char *file_name);
	void save_blob_item(indigo_item *item);
};

#endif // IMAGERWINDOW_H
