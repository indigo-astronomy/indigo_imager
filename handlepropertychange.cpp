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

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "imagerwindow.h"
#include "indigoclient.h"
#include "propertycache.h"

#define set_alert(widget) (widget->setStyleSheet("*:enabled { background-color: #312222;} *:!enabled { background-color: #292222;}"))
#define set_idle(widget) (widget->setStyleSheet("*:enabled {background-color: #272727;} *:!enabled {background-color: #272727;}"))
#define set_busy(widget) (widget->setStyleSheet("*:enabled {background-color: #313120;} *:!enabled {background-color: #292920;}"))
#define set_ok(widget) (widget->setStyleSheet("*:enabled {background-color: #272727;} QSpinBox:!enabled {background-color: #202020;}"))
//#define set_ok(widget) (widget->setStyleSheet("background-color: #202520;"))

template<typename W>
static void set_widget_state(indigo_property *property, W *widget) {
	switch (property->state) {
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
}

template<typename W>
static void configure_spinbox(indigo_item *item, int perm, W *widget) {
	if (item != nullptr) {
		widget->setRange(item->number.min, item->number.max);
		widget->setSingleStep(item->number.step);
		widget->blockSignals(true);
		widget->setValue(item->number.value);
		widget->blockSignals(false);
	}
	if (perm == INDIGO_RO_PERM) {
		widget->setEnabled(false);
	} else {
		widget->setEnabled(true);
	}
}


static void change_combobox_selection(indigo_property *property, QComboBox *combobox) {
	set_widget_state(property, combobox);
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value) {
			QString item = QString(property->items[i].label);
			combobox->setCurrentIndex(combobox->findText(item));
			indigo_debug("[SELECT] %s\n", item.toUtf8().data());
			break;
		}
	}
}


static void add_items_to_combobox(indigo_property *property, QComboBox *items_combobox) {
	items_combobox->clear();
	for (int i = 0; i < property->count; i++) {
		QString item = QString(property->items[i].label);
		if (items_combobox->findText(item) < 0) {
			items_combobox->addItem(item, QString(property->items[i].name));
			indigo_debug("[ADD mode] %s\n", item.toUtf8().data());
			if (property->items[i].sw.value) {
				items_combobox->setCurrentIndex(items_combobox->findText(item));
			}
		} else {
			indigo_debug("[DUPLICATE mode] %s\n", item.toUtf8().data());
		}
	}
}

static void reset_filter_names(indigo_property *property, QComboBox *filter_select) {
	filter_select->clear();
	for (int i = 0; i < property->count; i++) {
		QString filter_name = QString(property->items[i].text.value);
		if (filter_select->findText(filter_name) < 0) {
			filter_select->addItem(filter_name, QString(property->items[i].name));
			indigo_debug("[ADD mode] %s\n", filter_name.toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE mode] %s\n", filter_name.toUtf8().data());
		}
	}
}

static void set_filter_selected(indigo_property *property, QComboBox *filter_select) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], WHEEL_SLOT_ITEM_NAME)) {
			indigo_debug("SELECT: %s = %d\n", property->items[i].name, property->items[i].number.value);
			filter_select->setCurrentIndex((int)property->items[i].number.value-1);
			set_widget_state(property, filter_select);
		}
	}
}

static void update_cooler_onoff(indigo_property *property, QCheckBox *cooler_onoff) {
	cooler_onoff->setEnabled(true);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_COOLER_ON_ITEM_NAME)) {
			cooler_onoff->setChecked(property->items[i].sw.value);
			break;
		}
	}
}

static void update_ccd_temperature(indigo_property *property, QLineEdit *current_temp, QDoubleSpinBox *set_temp, bool update_value = false) {
	indigo_debug("change %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_TEMPERATURE_ITEM_NAME)) {
			if (update_value) {
				configure_spinbox(&property->items[i], property->perm, set_temp);
				set_temp->setValue(property->items[i].number.target);
			} else {
				configure_spinbox(nullptr, property->perm, set_temp);
			}
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].number.value);
			char temperature[INDIGO_VALUE_SIZE];
			snprintf(temperature, INDIGO_VALUE_SIZE, "%.1f", property->items[i].number.value);
			current_temp->setText(temperature);
			set_widget_state(property, current_temp);
		}
	}
}

static void update_cooler_power(indigo_property *property, QLineEdit *cooler_pwr) {
	indigo_debug("change %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_COOLER_POWER_ITEM_NAME)) {
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].number.value);
			char power[INDIGO_VALUE_SIZE];
			snprintf(power, INDIGO_VALUE_SIZE, "%.1f%%", property->items[i].number.value);
			cooler_pwr->setText(power);
			set_widget_state(property, cooler_pwr);
		}
	}
}

static void update_focuser_poition(indigo_property *property, QSpinBox *set_position) {
	indigo_debug("change %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], FOCUSER_POSITION_ITEM_NAME) ||
		    client_match_item(&property->items[i], FOCUSER_STEPS_ITEM_NAME)) {
			set_widget_state(property, set_position);
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].number.value);
			configure_spinbox(&property->items[i], property->perm, set_position);
		}
	}
}

static void update_imager_selection_property(indigo_property *property, QSpinBox *star_x, QSpinBox *star_y, ImageViewer *viewer, FocusGraph *focuser_graph) {
	double x = 0, y = 0;
	int size = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_IMAGER_SELECTION_X_ITEM_NAME)) {
			x = property->items[i].number.value;
			configure_spinbox(&property->items[i], property->perm, star_x);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_SELECTION_Y_ITEM_NAME)) {
			y = property->items[i].number.value;
			configure_spinbox(&property->items[i], property->perm, star_y);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_SELECTION_RADIUS_ITEM_NAME)) {
			double max = property->items[i].number.value * 2 + 2;
			size = (int)round(property->items[i].number.value * 2 + 1);
			focuser_graph->set_yaxis_range(0, max);
		}
	}
	viewer->moveResizeSelection(x, y, size);
	star_x->blockSignals(true);
	star_x->setValue(x);
	star_x->blockSignals(false);
	star_y->blockSignals(true);
	star_y->setValue(y);
	star_y->blockSignals(false);
}

static void update_guider_selection_property(indigo_property *property, QSpinBox *star_x, QSpinBox *star_y, ImageViewer *viewer) {
	double x = 0, y = 0;
	int size = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_X_ITEM_NAME)) {
			x = property->items[i].number.value;
			configure_spinbox(&property->items[i], property->perm, star_x);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_Y_ITEM_NAME)) {
			y = property->items[i].number.value;
			configure_spinbox(&property->items[i], property->perm, star_y);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME)) {
			size = (int)round(property->items[i].number.value * 2 + 1);
		}
	}
	viewer->moveResizeSelection(x, y, size);
	star_x->blockSignals(true);
	star_x->setValue(x);
	star_x->blockSignals(false);
	star_y->blockSignals(true);
	star_y->setValue(y);
	star_y->blockSignals(false);
}

static void update_focus_setup_property(indigo_property *property, QSpinBox *initial_step, QSpinBox *final_step, QSpinBox *focus_backlash, QSpinBox *focus_stack) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_INITIAL_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, initial_step);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_FINAL_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, final_step);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, focus_backlash);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_STACK_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, focus_stack);
		}
	}
}


static void update_agent_imager_batch_property(indigo_property *property, QDoubleSpinBox *exposure_time, QDoubleSpinBox *exposure_delay, QSpinBox *frame_count) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
		if (client_match_item(&property->items[i], AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, exposure_time);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_BATCH_DELAY_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, exposure_delay);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_BATCH_COUNT_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, frame_count);
		}
	}
}

static void update_ccd_frame_property(indigo_property *property, QSpinBox *roi_x, QSpinBox *roi_y, QSpinBox *roi_w, QSpinBox *roi_h) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
		if (client_match_item(&property->items[i], CCD_FRAME_LEFT_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, roi_x);
		} else if (client_match_item(&property->items[i], CCD_FRAME_TOP_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, roi_y);
		} else if (client_match_item(&property->items[i], CCD_FRAME_WIDTH_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, roi_w);
		} else if (client_match_item(&property->items[i], CCD_FRAME_HEIGHT_ITEM_NAME)) {
			configure_spinbox(&property->items[i], property->perm, roi_h);
		}
	}
}


static void update_agent_imager_pause_process_property(indigo_property *property, QPushButton* pause_button) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_PAUSE_PROCESS_ITEM_NAME)) {
			if(property->state == INDIGO_BUSY_STATE) {
				pause_button->setText("Resume");
				set_busy(pause_button);
			} else {
				pause_button->setText("Pause");
				set_ok(pause_button);
			}
		}
	}
}

static void update_wheel_slot_property(indigo_property *property, QComboBox *filter_select) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], WHEEL_SLOT_ITEM_NAME)) {
			indigo_property *p = properties.get(property->device, WHEEL_SLOT_NAME_PROPERTY_NAME);
			unsigned int current_filter = (unsigned int)property->items[i].number.value - 1;
			set_widget_state(property, filter_select);
			if (p && current_filter < p->count) {
				filter_select->setCurrentIndex(filter_select->findText(p->items[current_filter].text.value));
				indigo_debug("[SELECT filter] %s\n", p->items[current_filter].label);
			}
		}
	}
}

static void update_agent_imager_stats_property(
	indigo_property *property,
	QLabel *FWHM_label,
	QLabel *HFD_label,
	QLabel *peak_label,
	QLabel *drift_label,
	QProgressBar *exposure_progress,
	QProgressBar *process_progress,
	QProgressBar *focusing_progress,
	QPushButton *exposure_button,
	QPushButton *preview_button,
	QPushButton *focusing_button,
	QPushButton *focusing_preview_button,
	FocusGraph *focus_graph,
	QVector<double> &fwhm_data
) {
	double exp_elapsed, exp_time = 1;
	double drift_x, drift_y;
	int frames_complete, frames_total;
	static bool exposure_running = false;
	static bool focusing_running = false;
	static bool preview_running = false;
	static int prev_frame = -1;
	double FWHM;

	indigo_item *exposure_item = properties.get_item(property->device, AGENT_IMAGER_BATCH_PROPERTY_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME);
	if (exposure_item) exp_time = exposure_item->number.value;


	indigo_property *p = properties.get(property->device, AGENT_START_PROCESS_PROPERTY_NAME);
	if (p && p->state == INDIGO_BUSY_STATE ) {
		for (int i = 0; i < p->count; i++) {
			if (!strcmp(p->items[i].name, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME)) {
				exposure_running = p->items[i].sw.value;
			} else if (!strcmp(p->items[i].name, AGENT_IMAGER_START_FOCUSING_ITEM_NAME)) {
				focusing_running = p->items[i].sw.value;
			} else if (!strcmp(p->items[i].name, AGENT_IMAGER_START_PREVIEW_ITEM_NAME)) {
				preview_running = p->items[i].sw.value;
			}
		}
	}

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_FWHM_ITEM_NAME)) {
			 FWHM = property->items[i].number.value;
			 char fwhm_str[50];
			 snprintf(fwhm_str, 50, "%.2f", FWHM);
			 FWHM_label->setText(fwhm_str);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_HFD_ITEM_NAME)) {
			 double HFD = property->items[i].number.value;
			 char hfd_str[50];
			 snprintf(hfd_str, 50, "%.2f", HFD);
			 HFD_label->setText(hfd_str);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_PEAK_ITEM_NAME)) {
			int peak = (int)property->items[i].number.value;
			char peak_str[50];
			snprintf(peak_str, 50, "%d", peak);
			peak_label->setText(peak_str);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_DRIFT_X_ITEM_NAME)) {
			drift_x = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_DRIFT_Y_ITEM_NAME)) {
			drift_y = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME)) {
			exp_elapsed = exp_time - property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_FRAME_ITEM_NAME)) {
			frames_complete = (int)property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_STATS_FRAMES_ITEM_NAME)) {
			frames_total = (int)property->items[i].number.value;
		}
	}
	char drift_str[50];
	snprintf(drift_str, 50, "%.2f, %.2f", drift_x, drift_y);
	drift_label->setText(drift_str);
	if (exposure_running) {
		set_widget_state(property, exposure_button);
		if (property->state == INDIGO_BUSY_STATE) {
			//exposure_button->setText("Stop");
			exposure_button->setIcon(QIcon(":resource/stop.png"));
			preview_button->setEnabled(false);
			focusing_button->setEnabled(false);
			focusing_preview_button->setEnabled(false);
			exposure_progress->setRange(0, exp_time);
			exposure_progress->setValue(exp_elapsed);
			exposure_progress->setFormat("Exposure: %v of %m seconds elapsed...");
			if (frames_total < 0) {
				process_progress->setRange(0, frames_complete);
				process_progress->setValue(frames_complete - 1);
				process_progress->setFormat("Process: exposure %v complete...");
			} else {
				process_progress->setRange(0, frames_total);
				process_progress->setValue(frames_complete - 1);
				process_progress->setFormat("Process: exposure %v of %m complete...");
			}
			indigo_debug("frames total = %d", frames_total);

		} else if (property->state == INDIGO_OK_STATE) {
			//exposure_button->setText("Start");
			exposure_button->setIcon(QIcon(":resource/record.png"));
			preview_button->setEnabled(true);
			focusing_button->setEnabled(true);
			focusing_preview_button->setEnabled(true);
			exposure_progress->setRange(0, 100);
			exposure_progress->setValue(100);
			exposure_progress->setFormat("Exposure: Complete");
			process_progress->setRange(0, 100);
			process_progress->setValue(100);
			process_progress->setFormat("Process: Complete");
		} else {
			//exposure_button->setText("Start");
			exposure_button->setIcon(QIcon(":resource/record.png"));
			preview_button->setEnabled(true);
			focusing_button->setEnabled(true);
			focusing_preview_button->setEnabled(true);
			exposure_progress->setRange(0, 1);
			exposure_progress->setValue(0);
			exposure_progress->setFormat("Exposure: Aborted");
			process_progress->setValue(frames_complete - 1);
			if (frames_total < 0) {
				process_progress->setFormat("Process: exposure %v complete");
			} else {
				process_progress->setFormat("Process: exposure %v of %m complete");
			}
		}
	} else if (focusing_running || preview_running) {
		if (frames_complete != prev_frame) {
			fwhm_data.append(FWHM);
			if (fwhm_data.size() > 100) fwhm_data.removeFirst();
			focus_graph->redraw_data(fwhm_data);
			prev_frame = frames_complete;
		}
		set_widget_state(property, focusing_button);
		if (property->state == INDIGO_BUSY_STATE) {
			//focusing_button->setText("Stop");
			focusing_button->setIcon(QIcon(":resource/stop.png"));
			preview_button->setEnabled(false);
			exposure_button->setEnabled(false);
			focusing_preview_button->setEnabled(false);
			focusing_progress->setRange(0, exp_time);
			focusing_progress->setValue(exp_elapsed);
			focusing_progress->setFormat("Focusing: %v of %m seconds elapsed...");
		} else if(property->state == INDIGO_OK_STATE) {
			//focusing_button->setText("Focus");
			focusing_button->setIcon(QIcon(":resource/record.png"));
			preview_button->setEnabled(true);
			exposure_button->setEnabled(true);
			focusing_preview_button->setEnabled(true);
			focusing_progress->setRange(0, 100);
			focusing_progress->setValue(100);
			focusing_progress->setFormat("Focusing: Complete");
		} else {
			//focusing_button->setText("Focus");
			focusing_button->setIcon(QIcon(":resource/record.png"));
			preview_button->setEnabled(true);
			exposure_button->setEnabled(true);
			focusing_preview_button->setEnabled(true);
			focusing_progress->setRange(0, 1);
			focusing_progress->setValue(0);
			focusing_progress->setFormat("Focusing: Stopped");
		}
	}
}

static void update_ccd_exposure(
	indigo_property *property,
	QDoubleSpinBox *exposure_time,
	QProgressBar *exposure_progress,
	QProgressBar *process_progress,
	QPushButton *exposure_button,
	QPushButton *preview_button,
	QPushButton *focusing_button,
	QPushButton *focusing_preview_button
) {
	double exp_elapsed, exp_time;
	set_widget_state(property, focusing_preview_button);
	set_widget_state(property, preview_button);
	if (property->state == INDIGO_BUSY_STATE) {
		exp_time = exposure_time->value();
		for (int i = 0; i < property->count; i++) {
			if (client_match_item(&property->items[i], CCD_EXPOSURE_ITEM_NAME)) {
				exp_elapsed = exp_time - property->items[i].number.value;
			}
		}
		//preview_button->setText("Stop");
		preview_button->setIcon(QIcon(":resource/stop.png"));
		//focusing_preview_button->setText("Stop");
		focusing_preview_button->setIcon(QIcon(":resource/stop.png"));
		exposure_button->setEnabled(false);
		focusing_button->setEnabled(false);
		exposure_progress->setRange(0, exp_time);
		exposure_progress->setValue(exp_elapsed);
		exposure_progress->setFormat("Preview: %v of %m seconds elapsed...");
		process_progress->setRange(0, 1);
		process_progress->setValue(0);
		process_progress->setFormat("Preview in progress...");
	} else if(property->state == INDIGO_OK_STATE) {
		//preview_button->setText("Preview");
		preview_button->setIcon(QIcon(":resource/play.png"));
		//focusing_preview_button->setText("Preview");
		focusing_preview_button->setIcon(QIcon(":resource/play.png"));
		exposure_button->setEnabled(true);
		focusing_button->setEnabled(true);
		exposure_progress->setRange(0, 100);
		exposure_progress->setValue(100);
		process_progress->setRange(0,100);
		process_progress->setValue(100);
		exposure_progress->setFormat("Preview: Complete");
		process_progress->setFormat("Process: Complete");
	} else {
		//preview_button->setText("Preview");
		preview_button->setIcon(QIcon(":resource/play.png"));
		//focusing_preview_button->setText("Preview");
		focusing_preview_button->setIcon(QIcon(":resource/play.png"));
		exposure_button->setEnabled(true);
		focusing_button->setEnabled(true);
		exposure_progress->setRange(0, 1);
		exposure_progress->setValue(0);
		process_progress->setRange(0, 1);
		process_progress->setValue(0);
		exposure_progress->setFormat("Exposure: Aborted");
		process_progress->setFormat("Process: Aborted");
	}
}

static void update_guider_stats(
	indigo_property *property, ImageViewer *viewer,
	QLabel *ra_dec_drift_label,
	QLabel *x_y_drift_label,
	QLabel *pulse_label,
	QLabel *rmse_label,
	FocusGraph *drift_graph,
	QVector<double> &drift_ra,
	QVector<double> &drift_dec
) {
	double ref_x = 0, ref_y = 0;
	double d_ra = 0, d_dec = 0;
	double cor_ra = 0, cor_dec = 0;
	double rmse_ra = 0, rmse_dec = 0;
	double d_x = 0, d_y = 0;
	int size = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_REFERENCE_X_ITEM_NAME)) {
			ref_x = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_REFERENCE_Y_ITEM_NAME)) {
			ref_y = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_DRIFT_RA_ITEM_NAME)) {
			d_ra = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_DRIFT_DEC_ITEM_NAME)) {
			d_dec = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_DRIFT_X_ITEM_NAME)) {
			d_x = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_DRIFT_Y_ITEM_NAME)) {
			d_y = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_RMSE_RA_ITEM_NAME)) {
			rmse_ra = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_RMSE_DEC_ITEM_NAME)) {
			rmse_dec = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_CORR_RA_ITEM_NAME)) {
			cor_ra = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_CORR_DEC_ITEM_NAME)) {
			cor_dec = property->items[i].number.value;
		}

	}
	viewer->moveReference(ref_x, ref_y);

	char label_str[50];
	snprintf(label_str, 50, "%+.2f  %+.2f", d_ra, d_dec);
	ra_dec_drift_label->setText(label_str);

	snprintf(label_str, 50, "%+.2f  %+.2f", d_x, d_y);
	x_y_drift_label->setText(label_str);

	snprintf(label_str, 50, "%+.2f  %+.2f", cor_ra, cor_dec);
	pulse_label->setText(label_str);

	snprintf(label_str, 50, "%.2f  %.2f", rmse_ra, rmse_dec);
	rmse_label->setText(label_str);

	drift_ra.append(d_ra);
	drift_dec.append(d_dec);

	if (drift_dec.size() > 120) {
		drift_dec.removeFirst();
		drift_ra.removeFirst();
	}
	drift_graph->redraw_data2(drift_ra, drift_dec);
}


void ImagerWindow::on_window_log(indigo_property* property, char *message) {
	char timestamp[16];
	char log_line[512];
	char message_line[512];
	struct timeval tmnow;

	if (!message) return;

	gettimeofday(&tmnow, NULL);
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		lt = localtime(&rawtime);
	}
	strftime(timestamp, sizeof(log_line), "%H:%M:%S", lt);
#else
	strftime(timestamp, sizeof(log_line), "%H:%M:%S", localtime((const time_t *) &tmnow.tv_sec));
#endif
	snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%03ld", tmnow.tv_usec/1000);

	if (property) {
		snprintf(message_line, 512, "%s.%s: %s", property->device, property->name, message);
		switch (property->state) {
		case INDIGO_ALERT_STATE:
			snprintf(log_line, 512, "<font color = \"#E00000\">%s %s<\font>", timestamp, message_line);
			break;
		case INDIGO_BUSY_STATE:
			snprintf(log_line, 512, "<font color = \"orange\">%s %s<\font>", timestamp, message_line);
			break;
		default:
			snprintf(log_line, 512, "%s %s", timestamp, message_line);
			break;
		}
		indigo_debug("[message] %s\n", message_line);
	} else {
		snprintf(log_line, 512, "%s %s", timestamp, message);
		indigo_debug("[message] %s\n", message);
	}
	mLog->appendHtml(log_line); // Adds the message to the widget
}

void ImagerWindow::property_define(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE] = {0, 0};
	char selected_guider_agent[INDIGO_VALUE_SIZE] = {0, 0};
	static pthread_mutex_t l_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (!strncmp(property->device, "Server", 6)) {
		if (client_match_device_property(property, property->device, "LOAD")) {
			if (properties.get(property->device, property->name)) return;

			// load indigo_agent_image and indigo_agent_guider
			bool imager_not_loaded = true;
			bool guider_not_loaded = true;
			indigo_property *p = properties.get(property->device, "DRIVERS");
			if (p) {
				imager_not_loaded = !indigo_get_switch(p, "indigo_agent_imager");
				guider_not_loaded = !indigo_get_switch(p, "indigo_agent_guider");
			}
			char *device_name = (char*)malloc(INDIGO_NAME_SIZE);
			strncpy(device_name, property->device, INDIGO_NAME_SIZE);
			QtConcurrent::run([=]() {
				pthread_mutex_lock(&l_mutex);
				if (imager_not_loaded) {
					static const char *items[] = { "DRIVER" };
					static const char *values[] = { "indigo_agent_imager" };
					indigo_change_text_property(NULL, device_name, "LOAD", 1, items, values);
				}
				if (guider_not_loaded) {
					static const char *items[] = { "DRIVER" };
					static const char *values[] = { "indigo_agent_guider" };
					indigo_change_text_property(NULL, device_name, "LOAD", 1, items, values);
				}
				pthread_mutex_unlock(&l_mutex);
				free(device_name);
			});
		}
		return;
	}

	if(!strncmp(property->device, "Imager Agent", 12)) {
		QString name = QString(property->device);
		if (m_agent_imager_select->findText(name) < 0) {
			m_agent_imager_select->addItem(name, name);
			indigo_debug("[ADD imager agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE imager agent] %s\n", name.toUtf8().data());
		}
	}

	if(!strncmp(property->device, "Guider Agent", 12)) {
		QString name = QString(property->device);
		if (m_agent_guider_select->findText(name) < 0) {
			m_agent_guider_select->addItem(name, name);
			indigo_debug("[ADD guider agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE guider agent] %s\n", name.toUtf8().data());
		}
	}

	if ((!get_selected_imager_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) &&
	    (!get_selected_guider_agent(selected_guider_agent) || strncmp(property->device, "Guider Agent", 12))) {
		return;
	}

	if (client_match_device_property(property, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_camera_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_focuser_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_size_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_POSITION_PROPERTY_NAME)) {
		update_focuser_poition(property, m_focus_position);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_STEPS_PROPERTY_NAME)) {
		update_focuser_poition(property, m_focus_steps);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME)) {
		update_imager_selection_property(property, m_star_x, m_star_y, m_imager_viewer, m_focus_graph);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME)) {
		update_focus_setup_property(property, m_initial_step, m_final_step, m_focus_backlash, m_focus_stack);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		update_agent_imager_batch_property(property, m_exposure_time, m_exposure_delay, m_frame_count);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME)) {
		update_ccd_frame_property(property, m_roi_x, m_roi_y, m_roi_w, m_roi_h);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		reset_filter_names(property, m_filter_select);
		indigo_property *p = properties.get(property->device, WHEEL_SLOT_PROPERTY_NAME);
		if (p) set_filter_selected(p, m_filter_select);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_PROPERTY_NAME)) {
		set_filter_selected(property, m_filter_select);
	}
	if (client_match_device_property(property, selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_pause_process_property(property, m_pause_button);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME)) {
		update_cooler_onoff(property, m_cooler_onoff);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_POWER_PROPERTY_NAME)) {
		update_cooler_power(property, m_cooler_pwr);
	}
	if (client_match_device_property(property, selected_agent, CCD_TEMPERATURE_PROPERTY_NAME)) {
		update_ccd_temperature(property, m_current_temp, m_set_temp, true);
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_guider_camera_select);
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_guider_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME)) {
		update_guider_selection_property(property, m_guide_star_x, m_guide_star_y, m_guider_viewer);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_STATS_PROPERTY_NAME)) {
		update_guider_stats(
			property,
			m_guider_viewer,
			m_guider_rd_drift_label,
			m_guider_xy_drift_label,
			m_guider_pulse_label,
			m_guider_rmse_label,
			m_guider_graph,
			m_drift_data_ra,
			m_drift_data_dec
		);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_detection_mode_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_dec_guiding_select);
	}
}

void ImagerWindow::on_property_define(indigo_property* property, char *message) {
	property_define(property, message);
	properties.create(property);
}


void ImagerWindow::on_property_change(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE] = {0, 0};
	char selected_guider_agent[INDIGO_VALUE_SIZE] = {0, 0};

	if ((!get_selected_imager_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) &&
	    (!get_selected_guider_agent(selected_guider_agent) || strncmp(property->device, "Guider Agent", 12))) {
		return;
	}

	if (client_match_device_property(property, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		change_combobox_selection(property, m_camera_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME)) {
		change_combobox_selection(property, m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME)) {
		change_combobox_selection(property, m_focuser_select);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_POSITION_PROPERTY_NAME)) {
		update_focuser_poition(property, m_focus_position);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_STEPS_PROPERTY_NAME)) {
		update_focuser_poition(property, m_focus_steps);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		change_combobox_selection(property, m_frame_size_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		change_combobox_selection(property, m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		reset_filter_names(property, m_filter_select);
		indigo_property *p = properties.get(property->device, WHEEL_SLOT_PROPERTY_NAME);
		if (p) set_filter_selected(p, m_filter_select);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_PROPERTY_NAME)) {
		update_wheel_slot_property(property, m_filter_select);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME)) {
		update_imager_selection_property(property, m_star_x, m_star_y, m_imager_viewer, m_focus_graph);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME)) {
		update_focus_setup_property(property, m_initial_step, m_final_step, m_focus_backlash, m_focus_stack);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		//update_agent_imager_batch_property(property, m_exposure_time, m_exposure_delay, m_frame_count);
	}
	if (client_match_device_property(property, selected_agent, CCD_EXPOSURE_PROPERTY_NAME)) {
		indigo_property *p = properties.get(property->device, AGENT_START_PROCESS_PROPERTY_NAME);
		if (!m_save_blob && p && p->state != INDIGO_BUSY_STATE ) {
			update_ccd_exposure(
				property,
				m_exposure_time,
				m_exposure_progress,
				m_process_progress,
				m_exposure_button,
				m_preview_button,
				m_focusing_button,
				m_focusing_preview_button
			);
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_STATS_PROPERTY_NAME)) {
		update_agent_imager_stats_property(
			property,
			m_FWHM_label,
			m_HFD_label,
			m_peak_label,
			m_drift_label,
			m_exposure_progress,
			m_process_progress,
			m_focusing_progress,
			m_exposure_button,
			m_preview_button,
			m_focusing_button,
			m_focusing_preview_button,
			m_focus_graph,
			m_focus_fwhm_data
		);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME)) {
		update_ccd_frame_property(property, m_roi_x, m_roi_y, m_roi_w, m_roi_h);
	}
	if (client_match_device_property(property, selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_pause_process_property(property, m_pause_button);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME)) {
		update_cooler_onoff(property, m_cooler_onoff);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_POWER_PROPERTY_NAME)) {
		update_cooler_power(property, m_cooler_pwr);
	}
	if (client_match_device_property(property, selected_agent, CCD_TEMPERATURE_PROPERTY_NAME)) {
		update_ccd_temperature(property, m_current_temp, m_set_temp);
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		change_combobox_selection(property, m_guider_camera_select);
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME)) {
		change_combobox_selection(property, m_guider_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME)) {
		update_guider_selection_property(property, m_guide_star_x, m_guide_star_y, m_guider_viewer);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_STATS_PROPERTY_NAME)) {
		update_guider_stats(
			property,
			m_guider_viewer,
			m_guider_rd_drift_label,
			m_guider_xy_drift_label,
			m_guider_pulse_label,
			m_guider_rmse_label,
			m_guider_graph,
			m_drift_data_ra,
			m_drift_data_dec
		);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME)) {
		change_combobox_selection(property, m_detection_mode_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME)) {
		change_combobox_selection(property, m_dec_guiding_select);
	}
	properties.create(property);
}

void ImagerWindow::property_delete(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE] = {0, 0};
	char selected_guider_agent[INDIGO_VALUE_SIZE] = {0, 0};

	indigo_debug("[REMOVE REMOVE REMOVE REMOVE REMOVE] %s.%s\n", property->device, property->name);
	if ((!get_selected_imager_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) &&
	    (!get_selected_guider_agent(selected_guider_agent) || strncmp(property->device, "Guider Agent", 12))) {
		return;
	}
	indigo_debug("[REMOVE REMOVE REMOVE REMOVE] %s.%s\n", property->device, property->name);

	if (client_match_device_property(property, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		m_camera_select->clear();
	}
	if (client_match_device_property(property, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		m_wheel_select->clear();
	}
	if (client_match_device_property(property, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		m_focuser_select->clear();
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_frame_size_select->clear();
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_frame_type_select->clear();
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_roi_x->setValue(0);
		m_roi_x->setEnabled(false);
		m_roi_y->setValue(0);
		m_roi_y->setEnabled(false);
		m_roi_w->setValue(0);
		m_roi_w->setEnabled(false);
		m_roi_h->setValue(0);
		m_roi_h->setEnabled(false);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_filter_select->clear();
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		m_cooler_onoff->setEnabled(false);
		m_cooler_onoff->setChecked(false);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_POWER_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("REMOVE %s", property->name);
		m_cooler_pwr->setText("");
	}
	if (client_match_device_property(property, selected_agent, CCD_TEMPERATURE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("REMOVE %s", property->name);
		m_current_temp->setText("");
		m_set_temp->setEnabled(false);
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		m_guider_camera_select->clear();
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		m_guider_select->clear();
	}
	if (client_match_device_property(property, selected_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_detection_mode_select->clear();
	}
	if (client_match_device_property(property, selected_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_dec_guiding_select->clear();
	}
}

void ImagerWindow::on_property_delete(indigo_property* property, char *message) {

	if ((strncmp(property->device, "Imager Agent", 12)) && (strncmp(property->device, "Guider Agent", 12))) {
		properties.remove(property);
		free(property);
		return;
	}

	property_delete(property, message);

	if (client_match_device_property(property, property->device, INFO_PROPERTY_NAME) ||
		client_match_device_no_property(property, property->device)) {
		QString name = QString(property->device);
		int selected_index = m_agent_imager_select->currentIndex();
		int index = m_agent_imager_select->findText(name);
		if (index >= 0) {
			m_agent_imager_select->removeItem(index);
			if (selected_index == index) {
				m_agent_imager_select->setCurrentIndex(0);
				on_agent_selected(0);
			}
			indigo_debug("[REMOVE imager agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[NOT FOUND imager agent] %s\n", name.toUtf8().data());
		}
		selected_index = m_agent_guider_select->currentIndex();
		index = m_agent_guider_select->findText(name);
		if (index >= 0) {
			m_agent_guider_select->removeItem(index);
			if (selected_index == index) {
				m_agent_guider_select->setCurrentIndex(0);
				on_guider_agent_selected(0);
			}
			indigo_debug("[REMOVE guider agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[NOT FOUND guider agent] %s\n", name.toUtf8().data());
		}
	}
	properties.remove(property);
	free(property);
}
