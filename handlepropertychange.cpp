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

#include <utils.h>
#include "imagerwindow.h"
#include "indigoclient.h"
#include "propertycache.h"
#include "conf.h"
#include "widget_state.h"

template<typename W>
static void configure_spinbox(ImagerWindow *w, indigo_item *item, int perm, W *widget) {
	indigo_item *item_copy = nullptr;
	if (item != nullptr) {
		item_copy = (indigo_item *)malloc(sizeof(indigo_item));
		memcpy(item_copy, item, sizeof(indigo_item));
	}
	w->configure_spinbox(widget, item_copy, perm);
}

template<typename W>
void configure_spinbox_template(W *widget, indigo_item *item, int perm) {
	if (item != nullptr) {
		double max = (item->number.max < item->number.value) ? item->number.value : item->number.max;
		double min = (item->number.min > item->number.value) ? item->number.value : item->number.min;
		if (min > max) return;

		/* update only if value has changed, while avoiding roudoff error updates */
		widget->blockSignals(true);
		bool update_tooltip = false;
		if(fabs(widget->minimum() - min) > 1e-15) {
			widget->setMinimum(min);
			update_tooltip = true;
		}
		if(fabs(widget->maximum() - max) > 1e-15) {
			widget->setMaximum(max);
			update_tooltip = true;
		}
		if(fabs(widget->singleStep() - item->number.step) > 1e-15) {
			widget->setSingleStep(item->number.step);
			update_tooltip = true;
		}
		if(fabs(widget->value() - item->number.value) > 1e-15) {
			widget->setValue(item->number.value);
		}
		if (update_tooltip) {
			char tooltip[INDIGO_VALUE_SIZE];
			if (item->number.format[strlen(item->number.format) - 1] == 'm') {
				snprintf (
					tooltip,
					sizeof(tooltip),
					"%s, range: [%s, %s] step: %s",
					item->label, indigo_dtos(item->number.min, NULL),
					indigo_dtos(item->number.max, NULL),
					indigo_dtos(item->number.step, NULL)
				);
			} else {
				char format[1600];
				snprintf (
					format,
					sizeof(format),
					"%s, range: [%s, %s] step: %s",
					item->label, item->number.format,
					item->number.format,
					item->number.format
				);
				snprintf (
					tooltip,
					sizeof(tooltip),
					format,
					item->number.min,
					item->number.max,
					item->number.step
				);
			}
			widget->setToolTip(tooltip);
		}
		widget->blockSignals(false);
	}
	if (perm == INDIGO_RO_PERM) {
		widget->setEnabled(false);
	} else {
		widget->setEnabled(true);
	}
	if (item != nullptr) {
		free(item);
	}
}

void ImagerWindow::configure_spinbox_int(QSpinBox *widget, indigo_item *item, int perm) {
	configure_spinbox_template(widget, item, perm);
}

void ImagerWindow::configure_spinbox_double(QDoubleSpinBox *widget, indigo_item *item, int perm) {
	configure_spinbox_template(widget, item, perm);
}

static void change_combobox_selection(ImagerWindow *w, indigo_property *property, QComboBox *combobox) {
	w->set_widget_state(combobox, property->state);
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

static void add_items_to_combobox_filtered(indigo_property *property, const char *begins_with, QComboBox *items_combobox) {
	items_combobox->clear();
	items_combobox->addItem(QString("None"));
	for (int i = 0; i < property->count; i++) {
		if (strncmp(property->items[i].name, begins_with, strlen(begins_with))) {
			indigo_debug("[DOES NOT MATCH mode] '%s' skipped\n", begins_with);
			continue;
		}
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

static void change_combobox_selection_filtered(ImagerWindow *w, indigo_property *property, const char *begins_with, QComboBox *combobox) {
	w->set_widget_state(combobox, property->state);
	bool selected = false;
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value && !strncmp(property->items[i].name, begins_with, strlen(begins_with))) {
			QString item = QString(property->items[i].label);
			combobox->setCurrentIndex(combobox->findText(item));
			indigo_debug("[SELECT] %s\n", item.toUtf8().data());
			selected = true;
			break;
		}
	}
	if (!selected) {
		combobox->setCurrentIndex(0);
		indigo_debug("[SELECT None]");
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

void set_filter_selected(ImagerWindow *w, indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], WHEEL_SLOT_ITEM_NAME)) {
			indigo_debug("SELECT: %s = %d\n", property->items[i].name, property->items[i].number.value);
			w->m_filter_select->setCurrentIndex((int)property->items[i].number.value-1);
			w->set_widget_state(w->m_filter_select, property->state);
		}
	}
}

void update_cooler_onoff(ImagerWindow *w, indigo_property *property) {
	w->set_enabled(w->m_cooler_onoff, true);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_COOLER_ON_ITEM_NAME)) {
			w->m_cooler_onoff->setChecked(property->items[i].sw.value);
			break;
		}
	}
}

static void update_ccd_temperature(ImagerWindow *w, indigo_property *property, QLineEdit *current_temp, QDoubleSpinBox *set_temp, bool update_value = false) {
	indigo_debug("change %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_TEMPERATURE_ITEM_NAME)) {
			if (update_value) {
				configure_spinbox(w, &property->items[i], property->perm, set_temp);
				w->set_spinbox_value(set_temp, property->items[i].number.target);
			} else {
				configure_spinbox(w, nullptr, property->perm, set_temp);
			}
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].number.value);
			char temperature[INDIGO_VALUE_SIZE];
			snprintf(temperature, INDIGO_VALUE_SIZE, "%.1f", property->items[i].number.value);
			current_temp->setText(temperature);
			w->set_widget_state(current_temp, property->state);
		}
	}
}

void update_cooler_power(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_COOLER_POWER_ITEM_NAME)) {
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].number.value);
			char power[INDIGO_VALUE_SIZE];
			snprintf(power, INDIGO_VALUE_SIZE, "%.1f%%", property->items[i].number.value);
			w->m_cooler_pwr->setText(power);
			w->set_widget_state(w->m_cooler_pwr, property->state);
		}
	}
}


void update_agent_imager_dithering_property(ImagerWindow *w, indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_dither_aggr);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_DITHERING_TIME_LIMIT_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_dither_to);
		}
	}
}

static void update_focuser_poition(ImagerWindow *w, indigo_property *property, QSpinBox *set_position) {
	indigo_debug("change %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], FOCUSER_POSITION_ITEM_NAME) ||
		    client_match_item(&property->items[i], FOCUSER_STEPS_ITEM_NAME)) {
			w->set_widget_state(set_position, property->state);
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].number.value);
			configure_spinbox(w, &property->items[i], property->perm, set_position);
		}
	}
}

void update_imager_selection_property(ImagerWindow *w, indigo_property *property) {
	double x = 0, y = 0;
	int size = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_IMAGER_SELECTION_X_ITEM_NAME)) {
			x = property->items[i].number.value;
			configure_spinbox(w, &property->items[i], property->perm, w->m_star_x);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_SELECTION_Y_ITEM_NAME)) {
			y = property->items[i].number.value;
			configure_spinbox(w, &property->items[i], property->perm, w->m_star_y);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_SELECTION_RADIUS_ITEM_NAME)) {
			double max = property->items[i].number.value * 2 + 2;
			size = (int)round(property->items[i].number.value * 2 + 1);
			w->m_focus_graph->set_yaxis_range(0, max);
			configure_spinbox(w, &property->items[i], property->perm, w->m_focus_star_radius);
		}
	}
	w->m_imager_viewer->moveResizeSelection(x, y, size);
}

void update_guider_selection_property(ImagerWindow *w, indigo_property *property) {
	double x = 0, y = 0;
	int size = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_X_ITEM_NAME)) {
			x = property->items[i].number.value;
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_star_x);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_Y_ITEM_NAME)) {
			y = property->items[i].number.value;
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_star_y);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME)) {
			size = (int)round(property->items[i].number.value * 2 + 1);
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_star_radius);
		}
	}
	w->m_guider_viewer->moveResizeSelection(x, y, size);
}

void update_focus_setup_property(ImagerWindow *w, indigo_property *property) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_INITIAL_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_initial_step);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_FINAL_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_final_step);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_focus_backlash);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_FOCUS_STACK_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_focus_stack);
		}
	}
}


void update_agent_imager_batch_property(ImagerWindow *w, indigo_property *property) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
		if (client_match_item(&property->items[i], AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_exposure_time);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_BATCH_DELAY_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_exposure_delay);
		} else if (client_match_item(&property->items[i], AGENT_IMAGER_BATCH_COUNT_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_frame_count);
		}
	}
}

void update_ccd_frame_property(ImagerWindow *w, indigo_property *property) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
		if (client_match_item(&property->items[i], CCD_FRAME_LEFT_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_roi_x);
		} else if (client_match_item(&property->items[i], CCD_FRAME_TOP_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_roi_y);
		} else if (client_match_item(&property->items[i], CCD_FRAME_WIDTH_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_roi_w);
		} else if (client_match_item(&property->items[i], CCD_FRAME_HEIGHT_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_roi_h);
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

void update_wheel_slot_property(ImagerWindow *w, indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], WHEEL_SLOT_ITEM_NAME)) {
			indigo_property *p = properties.get(property->device, WHEEL_SLOT_NAME_PROPERTY_NAME);
			unsigned int current_filter = (unsigned int)property->items[i].number.value - 1;
			w->set_widget_state(w->m_filter_select, property->state);
			if (p && current_filter < p->count) {
				w->m_filter_select->setCurrentIndex(w->m_filter_select->findText(p->items[current_filter].text.value));
				indigo_debug("[SELECT filter] %s\n", p->items[current_filter].label);
			}
		}
	}
}

void update_agent_imager_stats_property(ImagerWindow *w, indigo_property *property) {
	double exp_elapsed, exp_time = 1;
	double drift_x, drift_y;
	int frames_complete, frames_total;
	static bool exposure_running = false;
	static bool focusing_running = false;
	static bool preview_running = false;
	static int prev_start_state = INDIGO_OK_STATE;
	static int prev_frame = -1;
	double FWHM = 0, HFD = 0;

	indigo_item *exposure_item = properties.get_item(property->device, AGENT_IMAGER_BATCH_PROPERTY_NAME, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME);
	if (exposure_item) exp_time = exposure_item->number.value;

	indigo_property *stats_p;
	indigo_property *start_p;
	if (!strcmp(property->name, AGENT_IMAGER_STATS_PROPERTY_NAME)) {
		stats_p = property;
		start_p = properties.get(property->device, AGENT_START_PROCESS_PROPERTY_NAME);
	} else {
		stats_p = properties.get(property->device, AGENT_IMAGER_STATS_PROPERTY_NAME);
		start_p = property;
	}

	if (stats_p == nullptr || start_p == nullptr) return;

	if (start_p && start_p->state == INDIGO_BUSY_STATE ) {
		for (int i = 0; i < start_p->count; i++) {
			if (!strcmp(start_p->items[i].name, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME)) {
				exposure_running = start_p->items[i].sw.value;
				w->save_blob = exposure_running;
			} else if (!strcmp(start_p->items[i].name, AGENT_IMAGER_START_FOCUSING_ITEM_NAME)) {
				focusing_running = start_p->items[i].sw.value;
			} else if (!strcmp(start_p->items[i].name, AGENT_IMAGER_START_PREVIEW_ITEM_NAME)) {
				preview_running = start_p->items[i].sw.value;
			}
		}
	} else {
		w->save_blob = false;
	}

	//indigo_error("exposure = %d, focusing = %d, preview = %d, stats_p->state = %d, start_p->state = %d", exposure_running, focusing_running, preview_running, stats_p->state, start_p->state);

	for (int i = 0; i < stats_p->count; i++) {
		if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_FWHM_ITEM_NAME)) {
			 FWHM = stats_p->items[i].number.value;
			 char fwhm_str[50];
			 snprintf(fwhm_str, 50, "%.2f", FWHM);
			 w->m_FWHM_label->setText(fwhm_str);
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_HFD_ITEM_NAME)) {
			 HFD = stats_p->items[i].number.value;
			 char hfd_str[50];
			 snprintf(hfd_str, 50, "%.2f", HFD);
			 w->m_HFD_label->setText(hfd_str);
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_PEAK_ITEM_NAME)) {
			int peak = (int)stats_p->items[i].number.value;
			char peak_str[50];
			snprintf(peak_str, 50, "%d", peak);
			w->m_peak_label->setText(peak_str);
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_DRIFT_X_ITEM_NAME)) {
			drift_x = stats_p->items[i].number.value;
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_DRIFT_Y_ITEM_NAME)) {
			drift_y = stats_p->items[i].number.value;
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME)) {
			exp_elapsed = exp_time - stats_p->items[i].number.value;
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_FRAME_ITEM_NAME)) {
			frames_complete = (int)stats_p->items[i].number.value;
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_FRAMES_ITEM_NAME)) {
			frames_total = (int)stats_p->items[i].number.value;
		}
	}
	char drift_str[50];
	snprintf(drift_str, 50, "%.2f, %.2f", drift_x, drift_y);
	w->m_drift_label->setText(drift_str);
	if (exposure_running) {
		w->set_widget_state(w->m_preview_button, INDIGO_OK_STATE);
		w->set_widget_state(w->m_focusing_button, INDIGO_OK_STATE);
		w->set_widget_state(w->m_focusing_preview_button, INDIGO_OK_STATE);
		if (start_p->state == INDIGO_BUSY_STATE) {
			w->m_exposure_button->setIcon(QIcon(":resource/stop.png"));
			w->set_enabled(w->m_exposure_button, true);
			w->set_enabled(w->m_preview_button, false);
			w->set_enabled(w->m_focusing_button, false);
			w->set_enabled(w->m_focusing_preview_button, false);
			w->m_exposure_progress->setRange(0, exp_time);
			w->m_exposure_progress->setValue(exp_elapsed);
			w->m_exposure_progress->setFormat("Exposure: %v of %m seconds elapsed...");
			if (frames_total < 0) {
				w->m_process_progress->setRange(0, frames_complete);
				w->m_process_progress->setValue(frames_complete - 1);
				w->m_process_progress->setFormat("Process: exposure %v complete...");
			} else {
				w->m_process_progress->setRange(0, frames_total);
				w->m_process_progress->setValue(frames_complete - 1);
				w->m_process_progress->setFormat("Process: exposure %v of %m complete...");
			}
			indigo_debug("frames total = %d", frames_total);
		} else if (start_p->state == INDIGO_OK_STATE) {
			w->m_exposure_button->setIcon(QIcon(":resource/record.png"));
			w->set_enabled(w->m_exposure_button, true);
			w->set_enabled(w->m_preview_button, true);
			w->set_enabled(w->m_focusing_button, true);
			w->set_enabled(w->m_focusing_preview_button, true);
			w->m_exposure_progress->setRange(0, 100);
			w->m_exposure_progress->setValue(100);
			w->m_exposure_progress->setFormat("Exposure: Complete");
			w->m_process_progress->setRange(0, 100);
			w->m_process_progress->setValue(100);
			w->m_process_progress->setFormat("Process: Complete");
			exposure_running = false;
		} else {
			w->m_exposure_button->setIcon(QIcon(":resource/record.png"));
			w->set_enabled(w->m_exposure_button, true);
			w->set_enabled(w->m_preview_button, true);
			w->set_enabled(w->m_focusing_button, true);
			w->set_enabled(w->m_focusing_preview_button, true);
			w->m_exposure_progress->setRange(0, 1);
			w->m_exposure_progress->setValue(0);
			w->m_exposure_progress->setFormat("Exposure: Aborted");
			w->m_process_progress->setRange(0, frames_total);
			w->m_process_progress->setValue(frames_complete - 1);
			if (frames_total < 0) {
				w->m_process_progress->setFormat("Process: exposure %v complete");
			} else {
				w->m_process_progress->setFormat("Process: exposure %v of %m complete");
			}
			exposure_running = false;
		}
	} else if (focusing_running || preview_running) {
		if (frames_complete != prev_frame) {
			if (frames_complete == 0) {
				w->m_focus_fwhm_data.clear();
				w->m_focus_hfd_data.clear();
			}
			w->m_focus_fwhm_data.append(FWHM);
			if (w->m_focus_fwhm_data.size() > 100) w->m_focus_fwhm_data.removeFirst();
			w->m_focus_hfd_data.append(HFD);
			if (w->m_focus_hfd_data.size() > 100) w->m_focus_hfd_data.removeFirst();

			if (w->m_focus_display_data) w->m_focus_graph->redraw_data(*(w->m_focus_display_data));
			prev_frame = frames_complete;
		}
		w->set_widget_state(w->m_preview_button, INDIGO_OK_STATE);
		w->set_widget_state(w->m_exposure_button, INDIGO_OK_STATE);
		w->set_widget_state(w->m_focusing_preview_button, INDIGO_OK_STATE);
		if (start_p->state == INDIGO_BUSY_STATE) {
			w->set_enabled(w->m_preview_button, false);
			w->set_enabled(w->m_exposure_button, false);
			w->set_enabled(w->m_focusing_button, true);
			w->set_enabled(w->m_focusing_preview_button, false);
			w->m_focusing_progress->setRange(0, exp_time);
			w->m_focusing_progress->setValue(exp_elapsed);
			w->m_focusing_progress->setFormat("Focusing: %v of %m seconds elapsed...");
		} else if(start_p->state == INDIGO_OK_STATE) {
			w->set_enabled(w->m_preview_button, true);
			w->set_enabled(w->m_exposure_button, true);
			w->set_enabled(w->m_focusing_button, true);
			w->set_enabled(w->m_focusing_preview_button, true);
			w->m_focusing_progress->setRange(0, 100);
			w->m_focusing_progress->setValue(100);
			w->m_focusing_progress->setFormat("Focusing: Complete");
			focusing_running = false;
			preview_running = false;
		} else {
			w->set_enabled(w->m_preview_button, true);
			w->set_enabled(w->m_exposure_button, true);
			w->set_enabled(w->m_focusing_button, true);
			w->set_enabled(w->m_focusing_preview_button, true);
			w->m_focusing_progress->setRange(0, 1);
			w->m_focusing_progress->setValue(0);
			w->m_focusing_progress->setFormat("Focusing: Stopped");
			focusing_running = false;
			preview_running = false;
		}
	} else {
		w->m_exposure_button->setIcon(QIcon(":resource/record.png"));
		w->set_enabled(w->m_preview_button, true);
		w->set_enabled(w->m_exposure_button, true);
		w->set_enabled(w->m_focusing_button, true);
		w->set_enabled(w->m_focusing_preview_button, true);
		focusing_running = false;
		preview_running = false;
		exposure_running = false;
	}

	if (prev_start_state != start_p->state) {
		w->set_widget_state(w->m_exposure_button, start_p->state);
		w->set_widget_state(w->m_focusing_button, start_p->state);
		prev_start_state = start_p->state;
	}
}

void update_ccd_exposure(ImagerWindow *w, indigo_property *property) {
	double exp_elapsed, exp_time;
	static int prev_property_state = INDIGO_OK_STATE;

	if (prev_property_state != property->state) {
		w->set_widget_state(w->m_focusing_preview_button, property->state);
		w->set_widget_state(w->m_preview_button, property->state);
		prev_property_state = property->state;
	}

	if (property->state == INDIGO_BUSY_STATE) {
		exp_time = w->m_exposure_time->value();
		for (int i = 0; i < property->count; i++) {
			if (client_match_item(&property->items[i], CCD_EXPOSURE_ITEM_NAME)) {
				exp_elapsed = exp_time - property->items[i].number.value;
			}
		}
		w->m_preview_button->setIcon(QIcon(":resource/stop.png"));
		w->m_focusing_preview_button->setIcon(QIcon(":resource/stop.png"));
		w->set_enabled(w->m_exposure_button, false);
		w->set_enabled(w->m_focusing_button, false);
		w->m_exposure_progress->setRange(0, exp_time);
		w->m_exposure_progress->setValue(exp_elapsed);
		w->m_exposure_progress->setFormat("Preview: %v of %m seconds elapsed...");
		w->m_process_progress->setRange(0, 1);
		w->m_process_progress->setValue(0);
		w->m_process_progress->setFormat("Preview in progress...");
	} else if(property->state == INDIGO_OK_STATE) {
		w->m_preview_button->setIcon(QIcon(":resource/play.png"));
		w->m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
		w->set_enabled(w->m_exposure_button, true);
		w->set_enabled(w->m_focusing_button, true);
		w->m_exposure_progress->setRange(0, 100);
		w->m_exposure_progress->setValue(100);
		w->m_process_progress->setRange(0,100);
		w->m_process_progress->setValue(100);
		w->m_exposure_progress->setFormat("Preview: Complete");
		w->m_process_progress->setFormat("Process: Complete");
	} else {
		w->m_preview_button->setIcon(QIcon(":resource/play.png"));
		w->m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
		w->set_enabled(w->m_exposure_button, true);
		w->set_enabled(w->m_focusing_button, true);
		w->m_exposure_progress->setRange(0, 1);
		w->m_exposure_progress->setValue(0);
		w->m_process_progress->setRange(0, 1);
		w->m_process_progress->setValue(0);
		w->m_exposure_progress->setFormat("Exposure: Aborted");
		w->m_process_progress->setFormat("Process: Aborted");
	}
}

void update_guider_stats(ImagerWindow *w, indigo_property *property) {
	double ref_x = 0, ref_y = 0;
	double d_ra = 0, d_dec = 0;
	double cor_ra = 0, cor_dec = 0;
	double rmse_ra = 0, rmse_dec = 0, dither_rmse = 0;
	double d_x = 0, d_y = 0;
	int size = 0, frame_count = -1;
	bool is_guiding = false;

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_FRAME_ITEM_NAME)) {
			frame_count = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_REFERENCE_X_ITEM_NAME)) {
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
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_STATS_DITHERING_ITEM_NAME)) {
			dither_rmse = property->items[i].number.value;
		}
	}

	indigo_property *p = properties.get(property->device, AGENT_START_PROCESS_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], AGENT_GUIDER_START_GUIDING_ITEM_NAME) && p->items[i].sw.value) {
				is_guiding = true;
				if (dither_rmse == 0) {
					w->set_guider_label(INDIGO_OK_STATE, " Guiding... ");
				} else {
					w->set_guider_label(INDIGO_BUSY_STATE, " Dithering... ");
				}

				if (frame_count == 0) {
					w->m_drift_data_ra.clear();
					w->m_drift_data_dec.clear();
					w->m_pulse_data_ra.clear();
					w->m_pulse_data_dec.clear();
					w->m_drift_data_x.clear();
					w->m_drift_data_y.clear();
					w->m_guider_graph->redraw_data2(*(w->m_guider_data_1), *(w->m_guider_data_2));
				} else if (frame_count > 0) {
					w->m_drift_data_ra.append(d_ra);
					w->m_drift_data_dec.append(d_dec);
					w->m_pulse_data_ra.append(cor_ra);
					w->m_pulse_data_dec.append(cor_dec);
					w->m_drift_data_x.append(d_x);
					w->m_drift_data_y.append(d_y);

					if (w->m_drift_data_dec.size() > 120) {
						w->m_drift_data_dec.removeFirst();
						w->m_drift_data_ra.removeFirst();
						w->m_pulse_data_ra.removeFirst();
						w->m_pulse_data_dec.removeFirst();
						w->m_drift_data_x.removeFirst();
						w->m_drift_data_y.removeFirst();
					}
					w->m_guider_graph->redraw_data2(*(w->m_guider_data_1), *(w->m_guider_data_2));
				}
				break;
			}
		}
		char time_str[255];
		if (p->state == INDIGO_BUSY_STATE) {
			w->m_guider_viewer->moveReference(ref_x, ref_y);
			if (is_guiding) {
				if (conf.guider_save_log) {
					if (w->m_guide_log == nullptr) {
						char file_name[255];
						char path[PATH_LEN];
						get_date_jd(time_str);
						get_current_output_dir(path);
						snprintf(file_name, sizeof(file_name), "%sAin_guiding_%s.log", path, time_str);
						w->m_guide_log = fopen(file_name, "a+");
						if (w->m_guide_log) {
							get_timestamp(time_str);
							if (frame_count > 0) {
								fprintf(w->m_guide_log, "Log started at %s\n", time_str);
							} else {
								fprintf(w->m_guide_log, "Guiding started at %s\n", time_str);
							}
							fprintf(w->m_guide_log, "Timestamp,X Dif,Y Dif,RA Dif,Dec Dif,Ra Correction,Dec Correction\n");
						}
					}
				} else {
					if (w->m_guide_log) {
						get_timestamp(time_str);
						fprintf(w->m_guide_log, "Log finished at %s\n\n", time_str);
						fclose(w->m_guide_log);
						w->m_guide_log = nullptr;
					}
				}
				if (w->m_guide_log && frame_count > 0) {
					get_timestamp(time_str);
					fprintf(w->m_guide_log, "%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n", time_str, d_x, d_y, d_ra, d_dec, cor_ra, cor_dec);
				}
			}
		} else {
			w->m_guider_viewer->moveReference(0, 0);
			if (conf.guider_save_log) {
				if (w->m_guide_log) {
					get_timestamp(time_str);
					fprintf(w->m_guide_log, "Guiding finished at %s\n\n", time_str);
					fclose(w->m_guide_log);
					w->m_guide_log = nullptr;
				}
			} else {
				if (w->m_guide_log) {
					get_timestamp(time_str);
					fprintf(w->m_guide_log, "Log finished at %s\n\n", time_str);
					fclose(w->m_guide_log);
					w->m_guide_log = nullptr;
				}
			}
		}
	}

	char label_str[50];
	snprintf(label_str, 50, "%+.2f  %+.2f", d_ra, d_dec);
	w->m_guider_rd_drift_label->setText(label_str);

	snprintf(label_str, 50, "%+.2f  %+.2f", d_x, d_y);
	w->m_guider_xy_drift_label->setText(label_str);

	snprintf(label_str, 50, "%+.2f  %+.2f", cor_ra, cor_dec);
	w->m_guider_pulse_label->setText(label_str);

	snprintf(label_str, 50, "%.2f  %.2f", rmse_ra, rmse_dec);
	w->m_guider_rmse_label->setText(label_str);
}

void update_guider_settings(ImagerWindow *w, indigo_property *property) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
		if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guider_exposure);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_DELAY_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guider_delay);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_STEP_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_cal_step);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_BACKLASH_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_dec_backlash);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_ANGLE_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_rotation);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_ra_speed);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_dec_speed);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_min_error);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_min_pulse);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_max_pulse);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_AGG_RA_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_ra_aggr);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_dec_aggr);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_PW_RA_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_ra_pw);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_PW_DEC_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_dec_pw);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_is);
		}
	}
}

void agent_guider_start_process_change(ImagerWindow *w, indigo_property *property) {
	if (property->state==INDIGO_BUSY_STATE) {
		for (int i = 0; i < property->count; i++) {
			indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
			if (client_match_item(&property->items[i], AGENT_GUIDER_START_CALIBRATION_ITEM_NAME) && property->items[i].sw.value) {
				w->set_widget_state(w->m_guider_calibrate_button, property->state);
				w->set_widget_state(w->m_guider_guide_button, INDIGO_OK_STATE);
				w->set_widget_state(w->m_guider_preview_button, INDIGO_OK_STATE);
				w->set_guider_label(INDIGO_BUSY_STATE, " Calibrating... ");
				w->set_enabled(w->m_guider_calibrate_button, true);
				w->set_enabled(w->m_guider_guide_button, false);
				w->set_enabled(w->m_guider_preview_button, false);
			} else if (client_match_item(&property->items[i], AGENT_GUIDER_START_GUIDING_ITEM_NAME) && property->items[i].sw.value) {
				w->set_widget_state(w->m_guider_calibrate_button, INDIGO_OK_STATE);
				w->set_widget_state(w->m_guider_guide_button, property->state);
				w->set_widget_state(w->m_guider_preview_button, INDIGO_OK_STATE);
				w->set_guider_label(INDIGO_OK_STATE, " Guiding... ");
				w->set_enabled(w->m_guider_calibrate_button, false);
				w->set_enabled(w->m_guider_guide_button, true);
				w->set_enabled(w->m_guider_preview_button, false);
			} else if (client_match_item(&property->items[i], AGENT_GUIDER_START_PREVIEW_ITEM_NAME) && property->items[i].sw.value) {
				w->set_widget_state(w->m_guider_calibrate_button, INDIGO_OK_STATE);
				w->set_widget_state(w->m_guider_guide_button, INDIGO_OK_STATE);
				w->set_widget_state(w->m_guider_preview_button, property->state);
				w->set_guider_label(INDIGO_BUSY_STATE, " Preview... ");
				w->set_enabled(w->m_guider_calibrate_button, false);
				w->set_enabled(w->m_guider_guide_button, false);
				w->set_enabled(w->m_guider_preview_button, true);
			}
		}
	} else {
		w->set_widget_state(w->m_guider_preview_button, property->state);
		w->set_widget_state(w->m_guider_calibrate_button, property->state);
		w->set_widget_state(w->m_guider_guide_button, property->state);
		if (property->state == INDIGO_ALERT_STATE) {
			w->set_guider_label(INDIGO_ALERT_STATE, " Process failed ");
		} else {
			w->set_guider_label(INDIGO_IDLE_STATE, " Stopped ");
		}
		w->set_enabled(w->m_guider_preview_button, true);
		w->set_enabled(w->m_guider_calibrate_button, true);
		w->set_enabled(w->m_guider_guide_button, true);
	}
}


void ImagerWindow::on_window_log(indigo_property* property, char *message) {
	char timestamp[255];
	char log_line[512];
	char message_line[512];
	struct timeval tmnow;

	if (!message) return;

	get_time(timestamp);

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
		if (client_match_device_property(property, property->device, SERVER_INFO_PROPERTY_NAME)) {
			indigo_item *item = indigo_get_item(property, SERVER_INFO_VERSION_ITEM_NAME);
			if (item) {
				int version_major;
				int version_minor;
				int build;
				char message[255];
				sscanf(item->text.value, "%d.%d-%d", &version_major, &version_minor, &build);
				if (build < 135) {
					sprintf(message, "WARNING: Some features will not work on '%s' running Indigo %s as Ain requires 2.0-135 or newer!", property->device, item->text.value);
					on_window_log(nullptr, message);
				}
			}
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
	if (client_match_device_property(property, selected_agent, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_format_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
		add_items_to_combobox_filtered(property, "Guider Agent", m_dither_agent_select);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_DITHERING_PROPERTY_NAME)) {
		update_agent_imager_dithering_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_POSITION_PROPERTY_NAME)) {
		update_focuser_poition(this, property, m_focus_position);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_STEPS_PROPERTY_NAME)) {
		update_focuser_poition(this, property, m_focus_steps);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME)) {
		update_imager_selection_property(this, property);
		set_enabled(m_focuser_subframe_select, true);
		QtConcurrent::run([=]() {
			change_focuser_subframe(selected_agent);
		});
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME)) {
		update_focus_setup_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		update_agent_imager_batch_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME)) {
		update_ccd_frame_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		reset_filter_names(property, m_filter_select);
		indigo_property *p = properties.get(property->device, WHEEL_SLOT_PROPERTY_NAME);
		if (p) set_filter_selected(this, p);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_PROPERTY_NAME)) {
		set_filter_selected(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_pause_process_property(property, m_pause_button);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME)) {
		update_cooler_onoff(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_POWER_PROPERTY_NAME)) {
		update_cooler_power(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_TEMPERATURE_PROPERTY_NAME)) {
		update_ccd_temperature(this, property, m_current_temp, m_set_temp, true);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_STATS_PROPERTY_NAME) ||
	    client_match_device_property(property, selected_agent, AGENT_START_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_stats_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_EXPOSURE_PROPERTY_NAME)) {
		indigo_property *p = properties.get(property->device, AGENT_START_PROCESS_PROPERTY_NAME);
		if (!save_blob && p && p->state != INDIGO_BUSY_STATE ) {
			update_ccd_exposure(this, property);
		}
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, CCD_PREVIEW_PROPERTY_NAME)) {
		QtConcurrent::run([=]() {
			static const char *items[] = { CCD_PREVIEW_ENABLED_ITEM_NAME };
			static const bool values[] = { true };
			indigo_change_switch_property(NULL, selected_guider_agent, CCD_PREVIEW_PROPERTY_NAME, 1, items, values);
		});
	}

	if (client_match_device_property(property, selected_guider_agent, CCD_JPEG_SETTINGS_PROPERTY_NAME)) {
		set_enabled(m_guider_save_bw_select, true);
	}

	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_guider_camera_select);
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_guider_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME)) {
		update_guider_selection_property(this, property);
		set_enabled(m_guider_subframe_select, true);
		QtConcurrent::run([=]() {
			change_guider_agent_subframe(selected_guider_agent);
		});
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_STATS_PROPERTY_NAME)) {
		update_guider_stats(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_detection_mode_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_dec_guiding_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME)) {
		update_guider_settings(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_START_PROCESS_PROPERTY_NAME)) {
		agent_guider_start_process_change(this, property);
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
		change_combobox_selection(this, property, m_camera_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_focuser_select);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_POSITION_PROPERTY_NAME)) {
		update_focuser_poition(this, property, m_focus_position);
	}
	if (client_match_device_property(property, selected_agent, FOCUSER_STEPS_PROPERTY_NAME)) {
		update_focuser_poition(this, property, m_focus_steps);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_frame_size_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_format_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
		change_combobox_selection_filtered(this, property, "Guider Agent", m_dither_agent_select);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_DITHERING_PROPERTY_NAME)) {
		update_agent_imager_dithering_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		reset_filter_names(property, m_filter_select);
		indigo_property *p = properties.get(property->device, WHEEL_SLOT_PROPERTY_NAME);
		if (p) set_filter_selected(this, p);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_PROPERTY_NAME)) {
		update_wheel_slot_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME)) {
		update_imager_selection_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME)) {
		update_focus_setup_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		//update_agent_imager_batch_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_EXPOSURE_PROPERTY_NAME)) {
		indigo_property *p = properties.get(property->device, AGENT_START_PROCESS_PROPERTY_NAME);
		if (!save_blob && p && p->state != INDIGO_BUSY_STATE ) {
			update_ccd_exposure(this, property);
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_STATS_PROPERTY_NAME) ||
	    client_match_device_property(property, selected_agent, AGENT_START_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_stats_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME)) {
		update_ccd_frame_property(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_pause_process_property(property, m_pause_button);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME)) {
		update_cooler_onoff(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_POWER_PROPERTY_NAME)) {
		update_cooler_power(this, property);
	}
	if (client_match_device_property(property, selected_agent, CCD_TEMPERATURE_PROPERTY_NAME)) {
		update_ccd_temperature(this, property, m_current_temp, m_set_temp);
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_guider_camera_select);
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_guider_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME)) {
		update_guider_selection_property(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_STATS_PROPERTY_NAME)) {
		update_guider_stats(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_detection_mode_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_dec_guiding_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME)) {
		update_guider_settings(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_START_PROCESS_PROPERTY_NAME)) {
		agent_guider_start_process_change(this, property);
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
	if (client_match_device_property(property, selected_agent, CCD_IMAGE_FORMAT_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_frame_format_select->clear();
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_frame_type_select->clear();
	}
	if (client_match_device_property(property, selected_agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_dither_agent_select->clear();
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_DITHERING_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		set_spinbox_value(m_dither_aggr, 0);
		set_enabled(m_dither_aggr, false);
		set_spinbox_value(m_dither_to, 0);
		set_enabled(m_dither_to, false);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		set_spinbox_value(m_roi_x, 0);
		set_enabled(m_roi_x, false);
		set_spinbox_value(m_roi_y, 0);
		set_enabled(m_roi_y, false);
		set_spinbox_value(m_roi_w, 0);
		set_enabled(m_roi_w, false);
		set_spinbox_value(m_roi_h, 0);
		set_enabled(m_roi_h, false);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_filter_select->clear();
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		set_enabled(m_cooler_onoff, false);
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
		set_enabled(m_set_temp, false);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {

		set_spinbox_value(m_star_x, 0);
		set_enabled(m_star_x, false);

		set_spinbox_value(m_star_y, 0);
		set_enabled(m_star_y, false);

		set_spinbox_value(m_focus_star_radius, 0);
		set_enabled(m_focus_star_radius, false);

		set_enabled(m_focuser_subframe_select, false);
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
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_detection_mode_select->clear();
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		m_dec_guiding_select->clear();
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		set_spinbox_value(m_guider_exposure, 0);
		set_enabled(m_guider_exposure, false);

		set_spinbox_value(m_guider_delay, 0);
		set_enabled(m_guider_delay, false);

		set_spinbox_value(m_guide_cal_step, 0);
		set_enabled(m_guide_cal_step, false);

		set_spinbox_value(m_guide_dec_backlash, 0);
		set_enabled(m_guide_dec_backlash, false);

		set_spinbox_value(m_guide_rotation, 0);
		set_enabled(m_guide_rotation, false);

		set_spinbox_value(m_guide_ra_speed, 0);
		set_enabled(m_guide_ra_speed, false);

		set_spinbox_value(m_guide_dec_speed, 0);
		set_enabled(m_guide_dec_speed, false);

		set_spinbox_value(m_guide_min_error, 0);
		set_enabled(m_guide_min_error, false);

		set_spinbox_value(m_guide_min_pulse, 0);
		set_enabled(m_guide_min_pulse, false);

		set_spinbox_value(m_guide_max_pulse, 0);
		set_enabled(m_guide_max_pulse, false);

		set_spinbox_value(m_guide_ra_aggr, 0);
		set_enabled(m_guide_ra_aggr, false);

		set_spinbox_value(m_guide_dec_aggr, 0);
		set_enabled(m_guide_dec_aggr, false);

		set_spinbox_value(m_guide_ra_pw, 0);
		set_enabled(m_guide_ra_pw, false);

		set_spinbox_value(m_guide_dec_pw, 0);
		set_enabled(m_guide_dec_pw, false);

		set_spinbox_value(m_guide_is, 0);
		set_enabled(m_guide_is, false);
	}

	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {

		set_spinbox_value(m_guide_star_x, 0);
		set_enabled(m_guide_star_x, false);

		set_spinbox_value(m_guide_star_y, 0);
		set_enabled(m_guide_star_y, false);

		set_spinbox_value(m_guide_star_radius, 0);
		set_enabled(m_guide_star_radius, false);

		set_enabled(m_guider_subframe_select, false);
	}

	if (client_match_device_property(property, selected_guider_agent, CCD_JPEG_SETTINGS_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {

		set_enabled(m_guider_save_bw_select, false);
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
