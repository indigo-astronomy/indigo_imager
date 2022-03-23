// Copyright (c) 2020 Rumen G.Bogdanovski
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

#include <stdio.h>
#include <QApplication>
#include <QMainWindow>
#include <QComboBox>
#include <indigo/indigo_bus.h>
#include <imageviewer.h>
#include <widget_state.h>
#include <conf.h>

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
#include <QTextEdit>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLCDNumber>
#include <QMessageBox>
#include <QActionGroup>
#include <QLineEdit>
#include <QCheckBox>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFileDialog>
#include <QThread>
#include <QtConcurrentRun>
#include "focusgraph.h"


class ImagerWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit ImagerWindow(QWidget *parent = nullptr);
	virtual ~ImagerWindow();
	void property_define_delete(indigo_property* property, char *message, bool action_deleted);

	bool get_selected_imager_agent(char * selected_agent) const {
		if (!selected_agent || !m_agent_imager_select) return false;
		strncpy(selected_agent, m_agent_imager_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		indigo_debug("SELECTED IMAGER AGENT = %s", selected_agent);
		return true;
	};

	bool get_selected_guider_agent(char * selected_agent) const {
		if (!selected_agent || !m_agent_guider_select) return false;
		strncpy(selected_agent, m_agent_guider_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		indigo_debug("SELECTED GUIDER AGENT = %s", selected_agent);
		return true;
	};

	bool get_selected_mount_agent(char * selected_agent) const {
		if (!selected_agent || !m_agent_mount_select) return false;
		strncpy(selected_agent, m_agent_mount_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		indigo_debug("SELECTED GUIDER AGENT = %s", selected_agent);
		return true;
	};

	bool get_selected_solver_agent(char * selected_agent) const {
		if (!selected_agent || !m_agent_solver_select) return false;
		strncpy(selected_agent, m_agent_solver_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		indigo_debug("SELECTED SOLVER AGENT = %s", selected_agent);
		return true;
	};

	void play_sound(const char *sound_file);

	void property_delete(indigo_property* property, char *message);
	void property_define(indigo_property* property, char *message);

	friend void update_focus_failreturn(ImagerWindow *w, indigo_property *property);
	friend void set_filter_selected(ImagerWindow *w, indigo_property *property);
	friend void reset_filter_names(ImagerWindow *w, indigo_property *property);
	friend void update_cooler_onoff(ImagerWindow *w, indigo_property *property);
	friend void update_cooler_power(ImagerWindow *w, indigo_property *property);
	friend void update_focuser_temperature(ImagerWindow *w, indigo_property *property);
	friend void update_focuser_temperature_compensation_steps(ImagerWindow *w, indigo_property *property);
	friend void update_focuser_mode(ImagerWindow *w, indigo_property *property);
	friend void update_imager_selection_property(ImagerWindow *w, indigo_property *property);
	friend void update_guider_selection_property(ImagerWindow *w, indigo_property *property);
	friend void update_agent_imager_gain_offset_property(ImagerWindow *w, indigo_property *property);
	friend void update_agent_guider_gain_offset_property(ImagerWindow *w, indigo_property *property);
	friend void update_focus_setup_property(ImagerWindow *w, indigo_property *property);
	friend void update_focus_estimator_property(ImagerWindow *w, indigo_property *property);
	friend void update_agent_imager_batch_property(ImagerWindow *w, indigo_property *property);
	friend void update_ccd_frame_property(ImagerWindow *w, indigo_property *property);
	friend void update_wheel_slot_property(ImagerWindow *w, indigo_property *property);
	friend void update_agent_imager_stats_property(ImagerWindow *w, indigo_property *property);
	friend void update_ccd_exposure(ImagerWindow *w, indigo_property *property);
	friend void update_guider_stats(ImagerWindow *w, indigo_property *property);
	friend void update_guider_settings(ImagerWindow *w, indigo_property *property);
	friend void update_guider_apply_dec_backlash(ImagerWindow *w, indigo_property *property);
	friend void agent_guider_start_process_change(ImagerWindow *w, indigo_property *property);
	friend void update_mount_ra_dec(ImagerWindow *w, indigo_property *property, bool update_input);
	friend void update_mount_az_alt(ImagerWindow *w, indigo_property *property);
	friend void update_mount_lst(ImagerWindow *w, indigo_property *property);
	friend void update_mount_park(ImagerWindow *w, indigo_property *property);
	friend void update_mount_track(ImagerWindow *w, indigo_property *property);
	friend void update_mount_slew_rates(ImagerWindow *w, indigo_property *property);
	friend void update_mount_side_of_pier(ImagerWindow *w, indigo_property *property);
	friend void update_agent_imager_dithering_property(ImagerWindow *w, indigo_property *property);
	friend void update_mount_gps_lon_lat_elev(ImagerWindow *w, indigo_property *property);
	friend void update_mount_gps_utc(ImagerWindow *w, indigo_property *property);
	friend void update_mount_gps_status(ImagerWindow *w, indigo_property *property);
	friend void update_mount_lon_lat(ImagerWindow *w, indigo_property *property);
	friend void update_mount_utc(ImagerWindow *w, indigo_property *property);
	friend void update_mount_agent_sync_time(ImagerWindow *w, indigo_property *property);
	friend void condigure_guider_overlays(ImagerWindow *w, char *device, indigo_property *property);
	friend void log_guide_header(ImagerWindow *w, char *device_name);
	friend void update_solver_agent_wcs(ImagerWindow *w, indigo_property *property);
	friend void update_solver_agent_hints(ImagerWindow *w, indigo_property *property);
	friend void define_ccd_exposure_property(ImagerWindow *w, indigo_property *property);
	friend int update_solver_agent_pa_error(ImagerWindow *w, indigo_property *property);
	friend void update_solver_agent_pa_settings(ImagerWindow *w, indigo_property *property);

	bool save_blob;

signals:
	void enable_blobs(bool on);
	void rebuild_blob_previews();

	void set_enabled(QWidget *widget, bool enabled);
	void set_widget_state(QWidget *widget, int state);
	void set_guider_label(int state, const char *text);
	void set_spinbox_value(QSpinBox *widget, double value);
	void set_spinbox_value(QDoubleSpinBox *widget, double value);
	void configure_spinbox(QSpinBox *widget, indigo_item *item, int perm);
	void configure_spinbox(QDoubleSpinBox *widget, indigo_item *item, int perm);
	void set_checkbox_checked(QCheckBox *widget, bool checked);
	void set_checkbox_state(QCheckBox *widget, int state);
	void set_text(QLabel *widget, QString text);
	void set_text(QLineEdit *widget, QString text);
	void set_text(QPushButton *widget, QString text);
	void set_text(QCheckBox *widget, QString text);
	void show_widget(QWidget *widget, bool show);

	void set_lcd(QLCDNumber *widget, QString text, int state);

	void set_combobox_current_text(QComboBox *combobox, const QString &item);
	void set_combobox_current_index(QComboBox *combobox, int index);
	void set_lineedit_text(QLineEdit *line_edit, const QString &text);
	void clear_combobox(QComboBox *combobox);
	void add_combobox_item(QComboBox *combobox, const QString &item, const QString& data);
	void remove_combobox_item(QComboBox *combobox, int index);

	void show_focuser_selection(bool show);
	void move_resize_focuser_selection(double x, double y, int size);

	void show_guider_selection(bool show);
	void move_resize_guider_selection(double x, double y, int size);
	void move_resize_guider_extra_selection(QList<QPointF> &point_list, int size);
	void show_guider_extra_selection(bool show);
	void show_guider_reference(bool show);
	void move_guider_reference(double x, double y);
	void show_guider_edge_clipping(bool show);
	void resize_guider_edge_clipping(double edge_clipping);

public slots:
	void on_exposure_start_stop(bool clicked);
	void on_preview_start_stop(bool clicked);
	void on_abort(bool clicked);
	void on_pause(bool clicked);
	void on_window_log(indigo_property* property, char *message);
	void on_property_define(indigo_property* property, char *message);
	void on_property_change(indigo_property* property, char *message);
	void on_property_delete(indigo_property* property, char *message);
	void on_message_sent(indigo_property* property, char *message);
	void on_blobs_changed(bool status);
	void on_save_noname_images_changed(bool status);
	void on_sound_notifications_changed(bool status);
	void on_restore_window_size_changed(bool status);
	void on_bonjour_changed(bool status);
	void on_use_suffix_changed(bool status);
	void on_use_state_icons_changed(bool status);
	void on_use_system_locale_changed(bool status);
	void on_log_error();
	void on_log_info();
	void on_log_debug();
	void on_log_trace();
	void on_image_save_act();
	void on_data_directory_prefix_act();
	void on_acl_load_act();
	void on_acl_append_act();
	void on_acl_save_act();
	void on_acl_clear_act();
	void on_servers_act();
	void on_exit_act();
	void on_about_act();

	void on_imager_cb_changed(int balance);
	void on_imager_stretch_changed(int level);
	void on_guider_cb_changed(int balance);
	void on_guider_stretch_changed(int level);

	void on_focus_show_fwhm();
	void on_focus_show_hfd();

	void on_antialias_view(bool status);
	void on_imager_show_reference(bool status);
	void on_antialias_guide_view(bool status);
	void on_create_preview(indigo_property *property, indigo_item *item);
	void on_obsolete_preview(indigo_property *property, indigo_item *item);
	void on_remove_preview(indigo_property *property, indigo_item *item);

	void on_agent_selected(int index);
	void on_wheel_selected(int index);
	void on_focuser_selected(int index);
	void on_camera_selected(int index);
	void on_ccd_mode_selected(int index);
	void on_ccd_image_format_selected(int index);
	void on_frame_type_selected(int index);
	void on_dither_agent_selected(int index);
	void on_agent_imager_dithering_changed(int index);
	void on_filter_selected(int index);
	void on_cooler_onoff(bool state);
	void on_temperature_set(double value);
	void on_agent_imager_gain_changed(int value);
	void on_agent_imager_offset_changed(int value);

	void on_focus_start_stop(bool clicked);
	void on_focus_preview_start_stop(bool clicked);
	void on_focus_mode_selected(int index);
	void on_focus_estimator_selected(int index);
	void on_selection_changed(double value);
	void on_focuser_selection_radius_changed(int value);
	void on_focuser_position_changed(int value);
	void on_image_right_click(double x, double y);
	void on_image_right_click_ra_dec(double ra, double dec);
	void on_focus_in(bool clicked);
	void on_focus_out(bool clicked);
	void on_focuser_subframe_changed(int index);
	void on_focuser_backlash_changed(int value);
	void on_focuser_bl_overshoot_changed(double value);
	void on_focuser_failreturn_changed(int state);
	void on_focuser_reverse_changed(int index);
	void on_focuser_temp_compensation_changed(int state);
	void on_focuser_temp_compensation_steps_changed(int value);
	void on_guider_agent_selected(int index);
	void on_guider_camera_selected(int index);
	void on_guider_selected(int index);
	void on_guider_selection_changed(double value);
	void on_guider_subframe_changed(int index);
	void on_guider_selection_radius_changed(int value);
	void on_guider_selection_star_count_changed(int value);
	void on_guider_edge_clipping_changed(int value);
	void on_guider_image_right_click(double x, double y);
	void on_guider_preview_start_stop(bool clicked);
	void on_guider_calibrate_start_stop(bool clicked);
	void on_guider_guide_start_stop(bool clicked);
	void on_guider_stop(bool clicked);
	void on_detection_mode_selected(int index);
	void on_dec_guiding_selected(int index);
	void on_guider_clear_selection(bool clicked);
	void on_guider_ccd_mode_selected(int index);
	void on_agent_guider_gain_changed(int value);
	void on_agent_guider_offset_changed(int value);

	void on_guider_agent_exposure_changed(double value);
	void on_guider_agent_callibration_changed(double value);
	void on_guider_apply_backlash_changed(int state);
	void on_guider_agent_pulse_changed(double value);
	void on_guider_agent_aggressivity_changed(int value);
	void on_change_guider_agent_i_gain_changed(double value);
	void on_change_guider_agent_is_changed(int value);
	void on_guider_bw_save_changed(int index);
	void on_guide_show_rd_drift();
	void on_guide_show_rd_pulse();
	void on_guide_show_xy_drift();
	void on_guider_save_log(bool status);
	void on_indigo_save_log(bool status);

	void on_mount_agent_selected(int index);
	void on_mount_selected(int index);
	void on_mount_goto(int index);
	void on_mount_sync(int index);
	void on_mount_abort(int index);
	void on_mount_track(int state);
	void on_mount_park(int state);
	void on_mount_move_north();
	void on_mount_stop_north();
	void on_mount_move_south();
	void on_mount_stop_south();
	void on_mount_move_east();
	void on_mount_stop_east();
	void on_mount_move_west();
	void on_mount_stop_west();
	void on_mount_set_guide_rate(int state);
	void on_mount_set_center_rate(int state);
	void on_mount_set_find_rate(int state);
	void on_mount_set_max_rate(int state);
	void on_mount_gps_selected(int index);
	void on_mount_coord_source_selected(int index);
	void on_mount_sync_time(int state);
	void on_mount_set_coordinates_to_agent();
	void on_mount_solve_and_center();
	void on_mount_solve_and_sync();
	void on_mount_guider_agent_selected(int index);
	void on_trigger_solve();
	void on_mount_polar_align();
	void on_mount_polar_align_stop();
	void on_mount_recalculate_polar_error();
	void on_image_source2_selected(int index);
	void on_image_source3_selected(int index);

	void on_solver_agent_selected(int index);
	void on_solver_ra_dec_hints_changed(bool clicked);
	void on_solver_hints_changed(int value);
	void on_solver_hints_changed(double value);
	void on_image_source1_selected(int index);
	void on_object_selected();
	void on_object_clicked(QListWidgetItem *item);
	void on_object_search_changed(const QString &obj_name);
	void on_object_search_entered();
	void on_mount_agent_set_pa_settings(double value);
	void on_mount_agent_set_pa_refraction(bool clicked);

	void on_tab_changed(int index);

	void on_set_enabled(QWidget *widget, bool enabled) {
		widget->setEnabled(enabled);
	};

	void on_show(QWidget *widget, bool show) {
		if(show) {
			widget->show();
		} else {
			widget->hide();
		}
	};

	void on_set_text(QLabel *widget, QString text) {
		widget->setText(text);
	};

	void on_set_text(QLineEdit *widget, QString text) {
		widget->setText(text);
	};

	void on_set_text(QPushButton *widget, QString text) {
		widget->setText(text);
	};

	void on_set_text(QCheckBox *widget, QString text) {
		widget->setText(text);
	};

	void on_set_spinbox_value(QSpinBox *widget, double value) {
		widget->blockSignals(true);
		widget->setValue(value);
		widget->blockSignals(false);
	};

	void on_set_spinbox_value(QDoubleSpinBox *widget, double value) {
		widget->blockSignals(true);
		widget->setValue(value);
		widget->blockSignals(false);
	};

	void on_set_checkbox_checked(QCheckBox *widget, bool checked) {
		widget->blockSignals(true);
		widget->setChecked(checked);
		widget->blockSignals(false);
	}

	void on_set_checkbox_state(QCheckBox *widget, int state) {
		widget->blockSignals(true);
		widget->setCheckState((Qt::CheckState)state);
		widget->blockSignals(false);
	}

	void on_set_widget_state(QWidget *widget, int state) {
		switch (state) {
			case INDIGO_IDLE_STATE:
				set_idle(widget);
				break;
			case INDIGO_OK_STATE:
				set_ok(widget);
				break;
			case INDIGO_BUSY_STATE:
				set_busy(widget);
				break;
			case INDIGO_ALERT_STATE:
				set_alert(widget);
				break;
			case AIN_OK_STATE:
				set_ok2(widget);
				break;
		}
	};

	void on_set_lcd(QLCDNumber *widget, QString text, int state) {
		switch (state) {
			case INDIGO_IDLE_STATE:
				set_idle(widget);
				break;
			case INDIGO_OK_STATE:
				set_ok(widget);
				break;
			case INDIGO_BUSY_STATE:
				set_busy(widget);
				break;
			case INDIGO_ALERT_STATE:
				set_alert(widget);
				break;
		}
		widget->display(text);
	};

	void on_set_guider_label(int state, const char *text) {
		if (text) m_guider_viewer->getTextLabel()->setText(text);
		switch (state) {
			case INDIGO_IDLE_STATE:
				set_idle(m_guider_viewer->getTextLabel());
				break;
			case INDIGO_OK_STATE:
				set_ok2(m_guider_viewer->getTextLabel());
				break;
			case INDIGO_BUSY_STATE:
				set_busy(m_guider_viewer->getTextLabel());
				break;
			case INDIGO_ALERT_STATE:
				set_alert(m_guider_viewer->getTextLabel());
				break;
		}
	};

	void on_configure_spinbox(QSpinBox *widget, indigo_item *item, int perm) {
		configure_spinbox_int(widget, item, perm);
	};

	void on_configure_spinbox(QDoubleSpinBox *widget, indigo_item *item, int perm) {
		configure_spinbox_double(widget, item, perm);
	};

	void on_set_combobox_current_text(QComboBox *combobox, const QString &item) {
		combobox->setCurrentText(item);
	};

	void on_set_combobox_current_index(QComboBox *combobox, int index) {
		combobox->setCurrentIndex(index);
	};

	void on_clear_combobox(QComboBox *combobox) {
		combobox->clear();
	};

	void on_add_combobox_item(QComboBox *combobox, const QString &item, const QString& data) {
		if (combobox->findText(item) < 0) {
			combobox->addItem(item, data);
			indigo_debug("[ADD] %s\n", item.toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE] %s\n", item.toUtf8().data());
		}
	};

	void on_remove_combobox_item(QComboBox *combobox, int index) {
		combobox->removeItem(index);
	};

	void on_set_lineedit_text(QLineEdit *line_edit, const QString &text) {
		line_edit->blockSignals(true);
		line_edit->setText(text);
		line_edit->blockSignals(false);
	};
private:
	QTextEdit* mLog;
	QTabWidget *m_tools_tabbar;

	// Capture tab
	QComboBox *m_agent_imager_select;
	QComboBox *m_camera_select;
	QComboBox *m_wheel_select;
	QComboBox *m_frame_type_select;
	QComboBox *m_frame_format_select;
	QComboBox *m_frame_size_select;
	QComboBox *m_dither_agent_select;
	QSpinBox  *m_roi_x, *m_roi_w;
	QSpinBox  *m_roi_y, *m_roi_h;
	QSpinBox  *m_dither_aggr;
	QSpinBox  *m_dither_to;
	QSpinBox  *m_dither_skip;
	QSpinBox  *m_imager_gain;
	QSpinBox  *m_imager_offset;
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
	QPushButton *m_exposure_button;
	QPushButton *m_preview_button;
	QDoubleSpinBox *m_preview_exposure_time;

	// Focuser tabbar
	QComboBox *m_focuser_select;
	QComboBox *m_focus_mode_select;
	QComboBox *m_focus_estimator_select;
	QDoubleSpinBox  *m_star_x;
	QDoubleSpinBox  *m_star_y;
	QSpinBox  *m_focus_star_radius;
	QComboBox *m_focuser_subframe_select;
	QComboBox *m_focuser_reverse_select;
	QSpinBox  *m_initial_step;
	QSpinBox  *m_final_step;
	QSpinBox  *m_focus_backlash;
	QDoubleSpinBox  *m_focus_bl_overshoot;
	QSpinBox  *m_focus_stack;
	QSpinBox  *m_focus_position;
	QSpinBox  *m_focus_steps;
	QLabel    *m_FWHM_label;
	QLabel    *m_HFD_label;
	QLabel    *m_peak_label;
	QLabel    *m_contrast_label;
	QLabel    *m_drift_label;
	QCheckBox *m_focuser_failreturn_cbox;
	QDoubleSpinBox *m_focuser_exposure_time;
	QPushButton *m_focusing_button;
	QPushButton *m_focusing_in_button;
	QPushButton *m_focusing_out_button;
	QPushButton *m_focusing_preview_button;
	QProgressBar *m_focusing_progress;
	FocusGraph *m_focus_graph;
	QLabel     *m_focus_graph_label;
	QFrame     *m_hfd_stats_frame;
	QFrame     *m_contrast_stats_frame;
	QVector<double> m_focus_fwhm_data;
	QVector<double> m_focus_hfd_data;
	QVector<double> m_focus_contrast_data;
	QVector<double> *m_focus_display_data;
	QLineEdit *m_focuser_temperature;
	QCheckBox *m_temperature_compensation_cbox;
	QFrame    *m_temperature_compensation_frame;
	QSpinBox  *m_focuser_temperature_compensation_steps;

	// Guider tab
	QComboBox *m_agent_guider_select;
	QComboBox *m_guider_camera_select;
	QDoubleSpinBox  *m_guider_exposure;
	QDoubleSpinBox  *m_guider_delay;
	QComboBox *m_guider_select;
	QDoubleSpinBox  *m_guide_star_x;
	QDoubleSpinBox  *m_guide_star_y;
	QSpinBox  *m_guide_star_radius;
	QSpinBox  *m_guide_star_count;
	QSpinBox  *m_guide_edge_clipping;
	QComboBox *m_guider_save_bw_select;
	QComboBox *m_guider_subframe_select;
	QComboBox *m_guider_frame_size_select;
	QSpinBox  *m_guider_gain;
	QSpinBox  *m_guider_offset;

	QDoubleSpinBox  *m_guide_cal_step;
	QDoubleSpinBox  *m_guide_rotation;
	QDoubleSpinBox  *m_guide_ra_speed;
	QDoubleSpinBox  *m_guide_dec_speed;
	QDoubleSpinBox  *m_guide_dec_backlash;
	QCheckBox *m_guider_apply_backlash_cbox;
	QDoubleSpinBox  *m_guide_min_error;
	QDoubleSpinBox  *m_guide_min_pulse;
	QDoubleSpinBox  *m_guide_max_pulse;
	QSpinBox  *m_guide_ra_aggr;
	QSpinBox  *m_guide_dec_aggr;
	QDoubleSpinBox  *m_guide_i_gain_ra;
	QDoubleSpinBox  *m_guide_i_gain_dec;
	QSpinBox  *m_guide_is;
	FocusGraph *m_guider_graph;
	QVector<double> m_drift_data_ra;
	QVector<double> m_drift_data_dec;
	QVector<double> m_pulse_data_ra;
	QVector<double> m_pulse_data_dec;
	QVector<double> m_drift_data_x;
	QVector<double> m_drift_data_y;
	QVector<double> *m_guider_data_1;
	QVector<double> *m_guider_data_2;
	QLabel *m_guider_graph_label;
	QLabel *m_guider_rd_drift_label;
	QLabel *m_guider_xy_drift_label;
	QLabel *m_guider_pulse_label;
	QLabel *m_guider_rmse_label;
	QPushButton *m_guider_guide_button;
	QPushButton *m_guider_preview_button;
	QPushButton *m_guider_calibrate_button;

	QComboBox *m_detection_mode_select;
	QComboBox *m_dec_guiding_select;

	FILE *m_guide_log;
	int m_guider_process;

	// Telescope tab
	QComboBox *m_agent_mount_select;
	QComboBox *m_mount_select;
	QLCDNumber *m_mount_ra_label;
	QLCDNumber *m_mount_dec_label;
#ifdef USE_LCD
	QLCDNumber *m_mount_lst_label;
	QLCDNumber *m_mount_az_label;
	QLCDNumber *m_mount_alt_label;
#else
	QLabel *m_mount_lst_label;
	QLabel *m_mount_az_label;
	QLabel *m_mount_alt_label;
#endif
	QLabel *m_mount_side_of_pier_label;
	QLineEdit *m_mount_ra_input;
	QLineEdit *m_mount_dec_input;
	QPushButton *m_mount_goto_button;
	QPushButton *m_mount_sync_button;
	QPushButton *m_mount_abort_button;
	QCheckBox *m_mount_track_cbox;
	QCheckBox *m_mount_park_cbox;
	QCheckBox *m_mount_guide_rate_cbox;
	QCheckBox *m_mount_center_rate_cbox;
	QCheckBox *m_mount_find_rate_cbox;
	QCheckBox *m_mount_max_rate_cbox;
	QComboBox *m_mount_gps_select;
	QComboBox *m_mount_guider_select;
	QComboBox *m_mount_coord_source_select;
	QLabel *m_mount_latitude;
	QLabel *m_mount_longitude;
	QLabel *m_mount_elevation;
	QLabel *m_mount_utc;
	QLineEdit *m_mount_lat_input;
	QLineEdit *m_mount_lon_input;
	QCheckBox *m_mount_sync_time_cbox;
	QLabel *m_gps_latitude;
	QLabel *m_gps_longitude;
	QLabel *m_gps_elevation;
	QLabel *m_gps_utc;
	QLabel *m_gps_status;
	QListWidget* m_object_list;
	QLineEdit* m_object_search_line;

	QPushButton *m_mount_solve_and_center_button;
	QPushButton *m_mount_solve_and_sync_button;

	//QCheckBox *m_mount_use_solver_cbox;
	QComboBox *m_solver_source_select2;
	QDoubleSpinBox *m_solver_exposure2;
	QLabel *m_solver_status_label2;
	QLabel *m_pa_status_label;

	//Solver agent
	QComboBox *m_agent_solver_select;
	QComboBox *m_solver_source_select1;
	QDoubleSpinBox *m_solver_exposure1;
	QPushButton *m_solve_button;
	QLabel *m_solver_ra_solution;
	QLabel *m_solver_dec_solution;
	QLabel *m_solver_angle_solution;
	QLabel *m_solver_scale_solution;
	QLabel *m_solver_fsize_solution;
	QLabel *m_solver_parity_solution;
	QLabel *m_solver_usedindex_solution;
	QLabel *m_solver_status_label1;

	QLineEdit *m_solver_ra_hint;
	QLineEdit *m_solver_dec_hint;
	QDoubleSpinBox *m_solver_radius_hint;
	QSpinBox *m_solver_ds_hint;
	QSpinBox *m_solver_parity_hint;
	QSpinBox *m_solver_depth_hint;
	QSpinBox *m_solver_tlimit_hint;
	QString m_last_solver_source;

	QComboBox *m_solver_source_select3;
	QDoubleSpinBox *m_solver_exposure3;
	QDoubleSpinBox *m_pa_move_ha;
	/*
	QCheckBox *m_pa_use_initial_cbox;
	QLineEdit *m_pa_dec_input;
	QLineEdit *m_pa_ha_input;
	*/
	QPushButton *m_mount_start_pa_button;
	QPushButton *m_mount_recalculate_pe_button;
	QPushButton *m_mount_pa_stop_button;
	QCheckBox *m_pa_refraction_cbox;
	QLabel *m_pa_error_az_label;
	QLabel *m_pa_error_alt_label;
	QLabel *m_pa_error_label;
	QMutex m_property_mutex;

	int m_stderr;

	// Image viewer
	ImageViewer *m_imager_viewer;
	ImageViewer *m_guider_viewer;
	ImageViewer *m_visible_viewer;
	indigo_item *m_indigo_item;

	QString m_image_key;
	QString m_guider_key;

	QIndigoServers *mIndigoServers;
	QServiceModel *mServiceModel;

	char m_image_path[PATH_LEN];
	//char *m_image_formrat;
	QString m_selected_filter;

	void window_log(char *message, int state = INDIGO_OK_STATE);

	void change_jpeg_settings_property(
		const char *agent,
		const int jpeg_quality,
		const double black_threshold,
		const double white_threshold
	);

	void configure_spinbox_int(QSpinBox *widget, indigo_item *item, int perm);
	void configure_spinbox_double(QDoubleSpinBox *widget, indigo_item *item, int perm);

	void create_focuser_tab(QFrame *capture_frame);
	void create_imager_tab(QFrame *camera_frame);
	void create_guider_tab(QFrame *guider_frame);
	void create_telescope_tab(QFrame *solver_frame);
	void create_solver_tab(QFrame *telescope_frame);
	void change_ccd_frame_property(const char *agent) const;
	void change_ccd_exposure_property(const char *agent, QDoubleSpinBox *exp_time) const;
	void change_ccd_abort_exposure_property(const char *agent) const;
	void change_ccd_mode_property(const char *agent, QComboBox *frame_size_select) const;
	void change_ccd_image_format_property(const char *agent) const;
	void change_ccd_frame_type_property(const char *agent) const;
	void change_agent_batch_property(const char *agent) const;
	void change_agent_batch_property_for_focus(const char *agent) const;
	void change_agent_start_exposure_property(const char *agent) const;
	void change_agent_pause_process_property(const char *agent) const;
	void change_agent_abort_process_property(const char *agent) const;
	void change_wheel_slot_property(const char *agent) const;
	void change_cooler_onoff_property(const char *agent) const;
	void change_ccd_temperature_property(const char *agent) const;
	void change_ccd_upload_property(const char *agent, const char *item_name) const;
	void change_related_agent(const char *agent, const char *old_agent, const char *new_agent) const;
	void set_mount_agent_selected_imager_agent() const;
	void change_agent_imager_dithering_property(const char *agent) const;
	void change_agent_gain_property(const char *agent, QSpinBox *ccd_gain) const;
	void change_agent_offset_property(const char *agent, QSpinBox *ccd_offset) const;

	void change_agent_start_preview_property(const char *agent) const;
	void change_agent_start_focusing_property(const char *agent) const;
	void change_agent_star_selection(const char *agent) const;
	void change_agent_focus_params_property(const char *agent, bool set_backlash) const;
	void change_agent_focuser_bl_overshoot(const char *agent) const;
	void change_focuser_steps_property(const char *agent) const;
	void change_focuser_position_property(const char *agent) const;
	void change_focuser_focus_in_property(const char *agent) const;
	void change_focuser_focus_out_property(const char *agent) const;
	void change_focuser_subframe(const char *agent) const;
	void change_focus_estimator_property(const char *agent) const;
	void change_focuser_reverse_property(const char *agent) const;
	void change_focuser_temperature_compensation_steps(const char *agent) const;

	void select_focuser_data(focuser_display_data show);

	void change_guider_agent_star_selection(const char *agent) const;
	void clear_guider_agent_star_selection(const char *agent) const;
	void change_guider_agent_star_count(const char *agent) const;
	void change_guider_agent_subframe(const char *agent) const;
	void change_agent_start_guide_property(const char *agent) const;
	void change_agent_start_calibrate_property(const char *agent) const;
	void change_detection_mode_property(const char *agent) const;
	void change_dec_guiding_property(const char *agent) const;
	void change_guider_agent_exposure(const char *agent) const;
	void change_guider_agent_callibration(const char *agent) const;
	void change_guider_agent_apply_dec_backlash(const char *agent) const;
	void change_guider_agent_pulse_min_max(const char *agent) const;
	void change_guider_agent_aggressivity(const char *agent) const;
	void change_guider_agent_i(const char *agent) const;
	void change_guider_agent_edge_clipping(const char *agent) const;
	void change_agent_ccd_peview(const char *agent, bool enable) const;

	void change_mount_agent_equatorial(const char *agent, bool sync=false) const;
	void change_mount_agent_abort(const char *agent) const;
	void change_mount_agent_location(const char *agent, QString property_prefix) const;

	void change_solver_agent_hints_property(const char *agent) const;
	void clear_solver_agent_releated_agents(const char *agent) const;
	void change_solver_agent_abort(const char *agent) const;

	void set_agent_releated_agent(const char *agent, const char *related_agent, bool select) const;
	void set_agent_solver_sync_action(const char *agent, char *item) const;

	void update_solver_widgets_at_start(const char *image_agent, const char *solver_agent);

	void mount_agent_set_switch_async(char *property, char *item, bool move);

	void trigger_solve_and_sync(bool recenter);
	void trigger_solve();
	void trigger_polar_alignment(bool recalculate);
	void change_solver_agent_pa_settings(const char *agent) const;

	void select_guider_data(guider_display_data show);

	void setup_preview(const char *agent);
	bool open_image(QString file_name, int *image_size, unsigned char **image_data);

	bool show_preview_in_imager_viewer(QString &key);
	bool show_preview_in_guider_viewer(QString &key);
	void show_selected_preview_in_solver_tab(QString &solver_source);
	bool save_blob_item_with_prefix(indigo_item *item, const char *prefix, char *file_name);
	bool save_blob_item(indigo_item *item, char *file_name);
	void save_blob_item(indigo_item *item);

	void show_message(const char *title, const char *message, QMessageBox::Icon icon = QMessageBox::Warning) {
		QMessageBox msgBox(this);
		indigo_error(message);
		msgBox.setWindowTitle(title);
		msgBox.setIcon(icon);
		msgBox.setText(message);
		msgBox.exec();
	};
};

#endif // IMAGERWINDOW_H
