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
			w->set_combobox_current_text(combobox, item);
			indigo_debug("[SELECT] %s\n", item.toUtf8().data());
			break;
		}
	}
}

// should be redesigned - crashes sometimes...
static void add_items_to_combobox(ImagerWindow *w, indigo_property *property, QComboBox *items_combobox) {
	w->clear_combobox(items_combobox);
	for (int i = 0; i < property->count; i++) {
		QString item = QString(property->items[i].label);
		w->add_combobox_item(items_combobox, item, QString(property->items[i].name));
		if (property->items[i].sw.value) {
			w->set_combobox_current_text(items_combobox, item);
		}
	}
}

static void add_items_to_combobox_filtered(ImagerWindow *w, indigo_property *property, const char *begins_with, QComboBox *items_combobox) {
	w->clear_combobox(items_combobox);
	w->add_combobox_item(items_combobox, QString("None"), QString(""));
	for (int i = 0; i < property->count; i++) {
		if (strncmp(property->items[i].name, begins_with, strlen(begins_with))) {
			indigo_debug("[DOES NOT MATCH mode] '%s' skipped\n", begins_with);
			continue;
		}
		QString item = QString(property->items[i].label);
		w->add_combobox_item(items_combobox, item, QString(property->items[i].name));
		if (property->items[i].sw.value) {
			w->set_combobox_current_text(items_combobox, item);
		}
	}
}

static void change_combobox_selection_filtered(ImagerWindow *w, indigo_property *property, const char *begins_with, QComboBox *combobox) {
	w->set_widget_state(combobox, property->state);
	bool selected = false;
	for (int i = 0; i < property->count; i++) {
		if (property->items[i].sw.value && !strncmp(property->items[i].name, begins_with, strlen(begins_with))) {
			QString item = QString(property->items[i].label);
			w->set_combobox_current_text(combobox, item);
			indigo_debug("[SELECT] %s\n", item.toUtf8().data());
			selected = true;
			break;
		}
	}
	if (!selected) {
		w->set_combobox_current_index(combobox, 0);
		indigo_debug("[SELECT None]");
	}
}

void reset_filter_names(ImagerWindow *w, indigo_property *property) {
	w->clear_combobox(w->m_filter_select);
	for (int i = 0; i < property->count; i++) {
		QString filter_name = QString(property->items[i].text.value);
		w->add_combobox_item(w->m_filter_select, filter_name, QString(property->items[i].name));
	}
}

void set_filter_selected(ImagerWindow *w, indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], WHEEL_SLOT_ITEM_NAME)) {
			indigo_debug("SELECT: %s = %d\n", property->items[i].name, property->items[i].number.value);
			w->set_combobox_current_index(w->m_filter_select, (int)property->items[i].number.value-1);
			w->set_widget_state(w->m_filter_select, property->state);
		}
	}
}

void update_cooler_onoff(ImagerWindow *w, indigo_property *property) {
	w->set_enabled(w->m_cooler_onoff, true);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_COOLER_ON_ITEM_NAME)) {
			w->set_checkbox_checked(w->m_cooler_onoff, property->items[i].sw.value);
			break;
		}
	}
}

void update_mount_ra_dec(ImagerWindow *w, indigo_property *property, bool update_input=false) {
	indigo_debug("change %s", property->name);
	double ra = 0;
	double dec = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_EQUATORIAL_COORDINATES_RA_ITEM_NAME)) {
			ra = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], MOUNT_EQUATORIAL_COORDINATES_DEC_ITEM_NAME)) {
			dec = property->items[i].number.value;
		}
	}

	if (update_input) {
		w->set_text(w->m_mount_ra_input, indigo_dtos(ra, "%d:%02d:%04.1f"));
		w->set_text(w->m_mount_dec_input, indigo_dtos(dec, "%d:%02d:%04.1f"));
	}

	QString ra_str(indigo_dtos(ra, "%d: %02d:%04.1f"));
	QString dec_str(indigo_dtos(dec, "%d' %02d %04.1f"));
	w->set_lcd(w->m_mount_ra_label, ra_str, property->state);
	w->set_lcd(w->m_mount_dec_label, dec_str, property->state);

	w->set_widget_state(w->m_mount_goto_button, property->state);
	w->set_widget_state(w->m_mount_sync_button, property->state);
	if (property->state == INDIGO_BUSY_STATE) {
		w->set_enabled(w->m_mount_sync_button, false);
	} else {
		w->set_enabled(w->m_mount_sync_button, true);
	}
}

void update_mount_az_alt(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	double az = 0;
	double alt = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_HORIZONTAL_COORDINATES_AZ_ITEM_NAME)) {
			az = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], MOUNT_HORIZONTAL_COORDINATES_ALT_ITEM_NAME)) {
			alt = property->items[i].number.value;
		}
	}

	int state = AIN_OK_STATE;
	if (alt < 10) {
		state = AIN_ALERT_STATE;
	} else if (alt < 30) {
		state = AIN_WARNING_STATE;
	}

#ifdef USE_LCD
	QString az_str(indigo_dtos(az, "%d' %02d %04.1f"));
	w->set_lcd(w->m_mount_az_label, az_str, state);

	QString alt_str(indigo_dtos(alt, "%d' %02d %04.1f"));
	w->set_lcd(w->m_mount_alt_label, alt_str, state);
#else
	QString az_str(indigo_dtos(az, "%d° %02d' %04.1f\""));
	w->set_text(w->m_mount_az_label, az_str);
	w->set_widget_state(w->m_mount_az_label, state);

	QString alt_str(indigo_dtos(alt, "%d° %02d' %04.1f\""));
	w->set_text(w->m_mount_alt_label, alt_str);
	w->set_widget_state(w->m_mount_alt_label, state);
#endif
}

void update_mount_lst(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	QString lst_str;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_LST_TIME_ITEM_NAME)) {
			lst_str = QString(indigo_dtos(property->items[i].number.value, NULL));
		}
	}
#ifdef USE_LCD
	QString col = QLatin1String(":");
	lst_str.replace(lst_str.indexOf(col), col.size(), QLatin1String(": "));
	w->set_lcd(w->m_mount_lst_label, lst_str, property->state);
#else
	w->set_text(w->m_mount_lst_label, lst_str);
	w->set_widget_state(w->m_mount_lst_label, property->state);
#endif
}

void update_mount_side_of_pier(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	bool east = false;
	bool west = false;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_SIDE_OF_PIER_WEST_ITEM_NAME)) {
			if (property->items[i].sw.value) west = true;
		} else if (client_match_item(&property->items[i], MOUNT_SIDE_OF_PIER_EAST_ITEM_NAME)) {
			if (property->items[i].sw.value) east = true;
		}
	}

	if (east) {
		w->set_text(w->m_mount_side_of_pier_label, "Side of pier: East");
		w->set_widget_state(w->m_mount_side_of_pier_label, property->state);
	} else if (west) {
		w->set_text(w->m_mount_side_of_pier_label, "Side of pier: West");
		w->set_widget_state(w->m_mount_side_of_pier_label, property->state);
	} else {
		w->set_text(w->m_mount_side_of_pier_label, "Side of pier: Unknown");
		w->set_widget_state(w->m_mount_side_of_pier_label, INDIGO_ALERT_STATE);
	}
}

void update_mount_park(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	bool parked = false;
	bool unparked = false;

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_PARK_PARKED_ITEM_NAME)) {
			parked = property->items[i].sw.value;
		} else if (client_match_item(&property->items[i], MOUNT_PARK_UNPARKED_ITEM_NAME)) {
			unparked = property->items[i].sw.value;
		}
	}

	w->set_enabled(w->m_mount_park_cbox, true);
	w->set_widget_state(w->m_mount_park_cbox, property->state);
	if (property->state == INDIGO_BUSY_STATE) {
		w->set_text(w->m_mount_park_cbox, "Parking...");
		w->set_checkbox_state(w->m_mount_park_cbox, Qt::PartiallyChecked);
	} else {
		if (parked) {
			w->set_text(w->m_mount_park_cbox, "Parked");
			w->set_checkbox_state(w->m_mount_park_cbox, Qt::Checked);
		} else {
			w->set_text(w->m_mount_park_cbox, "Unparked");
			w->set_checkbox_state(w->m_mount_park_cbox, Qt::Unchecked);
		}
	}
}

void update_mount_track(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	bool on = false;
	bool off = false;

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_TRACKING_ON_ITEM_NAME)) {
			on = property->items[i].sw.value;
		} else if (client_match_item(&property->items[i], MOUNT_TRACKING_OFF_ITEM_NAME)) {
			off = property->items[i].sw.value;
		}
	}

	w->set_enabled(w->m_mount_track_cbox, true);
	w->set_widget_state(w->m_mount_track_cbox, property->state);
	if (property->state == INDIGO_BUSY_STATE) {
		w->set_checkbox_state(w->m_mount_track_cbox, Qt::PartiallyChecked);
	} else {
		if (on) {
			w->set_text(w->m_mount_track_cbox, "Tracking");
			w->set_checkbox_state(w->m_mount_track_cbox, Qt::Checked);
		} else {
			w->set_text(w->m_mount_track_cbox, "Not Tracking");
			w->set_checkbox_state(w->m_mount_track_cbox, Qt::Unchecked);
		}
	}
}

void update_mount_agent_sync_time(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	bool sync_mount = false;
	bool sync_dome = false;

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_SET_HOST_TIME_MOUNT_ITEM_NAME)) {
			sync_mount = property->items[i].sw.value;
		} else if (client_match_item(&property->items[i], AGENT_SET_HOST_TIME_DOME_ITEM_NAME)) {
			sync_dome = property->items[i].sw.value;
		}
	}

	w->set_enabled(w->m_mount_sync_time_cbox, true);
	if (sync_mount) {
		w->set_checkbox_state(w->m_mount_sync_time_cbox, Qt::Checked);
	} else {
		w->set_checkbox_state(w->m_mount_sync_time_cbox, Qt::Unchecked);
	}
}

void update_mount_slew_rates(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);

	w->set_enabled(w->m_mount_guide_rate_cbox, true);
	w->set_enabled(w->m_mount_center_rate_cbox, true);
	w->set_enabled(w->m_mount_find_rate_cbox, true);
	w->set_enabled(w->m_mount_max_rate_cbox, true);
	w->set_checkbox_state(w->m_mount_guide_rate_cbox, Qt::Unchecked);
	w->set_checkbox_state(w->m_mount_center_rate_cbox, Qt::Unchecked);
	w->set_checkbox_state(w->m_mount_find_rate_cbox, Qt::Unchecked);
	w->set_checkbox_state(w->m_mount_max_rate_cbox, Qt::Unchecked);

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], MOUNT_SLEW_RATE_GUIDE_ITEM_NAME)) {
			if (property->items[i].sw.value) w->set_checkbox_state(w->m_mount_guide_rate_cbox, Qt::Checked);
		} else if (client_match_item(&property->items[i], MOUNT_SLEW_RATE_CENTERING_ITEM_NAME)) {
			if (property->items[i].sw.value) w->set_checkbox_state(w->m_mount_center_rate_cbox, Qt::Checked);
		} else if (client_match_item(&property->items[i], MOUNT_SLEW_RATE_FIND_ITEM_NAME)) {
			if (property->items[i].sw.value) w->set_checkbox_state(w->m_mount_find_rate_cbox, Qt::Checked);
		} else if (client_match_item(&property->items[i], MOUNT_SLEW_RATE_MAX_ITEM_NAME)) {
			if (property->items[i].sw.value) w->set_checkbox_state(w->m_mount_max_rate_cbox, Qt::Checked);
		}
	}
}

void update_mount_gps_lon_lat_elev(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	double lon = 0;
	double lat = 0;
	double elev = 0;
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
			lon = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
			lat = property->items[i].number.value;
		} else if (client_match_item(&property->items[i], GEOGRAPHIC_COORDINATES_ELEVATION_ITEM_NAME)) {
			elev = property->items[i].number.value;
		}
	}

	QString lon_str(indigo_dtos(lon, "%d° %02d' %04.1f\""));
	w->set_text(w->m_gps_longitude, lon_str);
	w->set_widget_state(w->m_gps_longitude, property->state);

	QString lat_str(indigo_dtos(lat, "%d° %02d' %04.1f\""));
	w->set_text(w->m_gps_latitude, lat_str);
	w->set_widget_state(w->m_gps_latitude, property->state);

	QString elev_str = QString::number(elev);
	w->set_text(w->m_gps_elevation, elev_str);
	w->set_widget_state(w->m_gps_elevation, property->state);
}

void update_mount_lon_lat(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);
	double lon = 0;
	double lat = 0;
	double target_lon = 0;
	double target_lat = 0;

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME)) {
			lon = property->items[i].number.value;
			target_lon = property->items[i].number.target;
		} else if (client_match_item(&property->items[i], GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME)) {
			lat = property->items[i].number.value;
			target_lat = property->items[i].number.target;
		}
	}

	QString lon_str(indigo_dtos(lon, "%d° %02d' %04.1f\""));
	w->set_text(w->m_mount_longitude, lon_str);
	w->set_widget_state(w->m_mount_longitude, property->state);
	w->set_lineedit_text(w->m_mount_lon_input, indigo_dtos(target_lon, "%d:%02d:%04.1f"));

	QString lat_str(indigo_dtos(lat, "%d° %02d' %04.1f\""));
	w->set_text(w->m_mount_latitude, lat_str);
	w->set_widget_state(w->m_mount_latitude, property->state);
	w->set_lineedit_text(w->m_mount_lat_input, indigo_dtos(target_lat, "%d:%02d:%04.1f"));
}

void update_mount_gps_utc(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], UTC_TIME_ITEM_NAME)) {
			w->set_text(w->m_gps_utc, property->items[i].text.value);
		}
	}

	w->set_widget_state(w->m_gps_utc, property->state);
}

void update_mount_utc(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);

	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], UTC_TIME_ITEM_NAME)) {
			w->set_text(w->m_mount_utc, property->items[i].text.value);
		}
	}

	w->set_widget_state(w->m_mount_utc, property->state);
}

void update_mount_gps_status(ImagerWindow *w, indigo_property *property) {
	indigo_debug("change %s", property->name);

	for (int i = 0; i < property->count; i++) {
		if (property->items[i].light.value != INDIGO_IDLE_STATE) {
			w->set_text(w->m_gps_status, property->items[i].label);
			if (property->items[i].light.value == INDIGO_OK_STATE) {
				w->set_widget_state(w->m_gps_status, AIN_OK_STATE);
			} else {
				w->set_widget_state(w->m_gps_status, property->items[i].light.value);
			}
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
			w->set_text(current_temp, temperature);
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
			w->set_text(w->m_cooler_pwr, power);
			w->set_text(w->m_cooler_pwr, power);
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

void update_agent_imager_gain_offset_property(ImagerWindow *w, indigo_property *property) {
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], CCD_GAIN_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_imager_gain);
		} else if (client_match_item(&property->items[i], CCD_OFFSET_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_imager_offset);
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
	w->move_resize_focuser_selection(x, y, size);
}

void update_guider_selection_property(ImagerWindow *w, indigo_property *property) {
	double x = 0, y = 0;
	int size = 0;
	int count = 1;
	double edge_clipping = 0;
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
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME)) {
			count = property->items[i].number.value;
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_star_count);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME)) {
			edge_clipping = property->items[i].number.value;
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_edge_clipping);
		}
	}

	QList<QPointF> s_list;
	if (count > 1) {
		for (int i = 2; i <= count; i++) {
			char name[INDIGO_NAME_SIZE];

			sprintf(name, "%s_%d", AGENT_GUIDER_SELECTION_X_ITEM_NAME, i);
			indigo_item *item = indigo_get_item(property, name);
			double x = 0;
			if (item) x = item->number.value;

			sprintf(name, "%s_%d", AGENT_GUIDER_SELECTION_Y_ITEM_NAME, i);
			item = indigo_get_item(property, name);
			double y = 0;
			if (item) y = item->number.value;

			if (x > 0 && y > 0) {
				QPointF sel(x, y);
				s_list.append(sel);
			}
		}
		w->move_resize_guider_extra_selection(s_list, size);
	}
	w->move_resize_guider_extra_selection(s_list, size);
	w->move_resize_guider_selection(x, y, size);
	w->resize_guider_edge_clipping(edge_clipping);
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


static void update_agent_imager_pause_process_property(ImagerWindow *w, indigo_property *property, QPushButton* pause_button) {
	indigo_debug("Set %s", property->name);
	for (int i = 0; i < property->count; i++) {
		if (client_match_item(&property->items[i], AGENT_PAUSE_PROCESS_ITEM_NAME)) {
			if(property->state == INDIGO_BUSY_STATE) {
				w->set_text(pause_button, "Resume");
				set_busy(pause_button);
			} else {
				w->set_text(pause_button, "Pause");
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
				w->m_filter_select->setCurrentText(p->items[current_filter].text.value);
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
			 w->set_text(w->m_FWHM_label, fwhm_str);
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_HFD_ITEM_NAME)) {
			 HFD = stats_p->items[i].number.value;
			 char hfd_str[50];
			 snprintf(hfd_str, 50, "%.2f", HFD);
			 w->set_text(w->m_HFD_label, hfd_str);
		} else if (client_match_item(&stats_p->items[i], AGENT_IMAGER_STATS_PEAK_ITEM_NAME)) {
			int peak = (int)stats_p->items[i].number.value;
			char peak_str[50];
			snprintf(peak_str, 50, "%d", peak);
			w->set_text(w->m_peak_label, peak_str);
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
	snprintf(drift_str, 50, "%+.2f, %+.2f", drift_x, drift_y);
	w->set_text(w->m_drift_label, drift_str);
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

	if (property == start_p) {
		w->set_widget_state(w->m_exposure_button, start_p->state);
		w->set_widget_state(w->m_focusing_button, start_p->state);
	}
}

void update_ccd_exposure(ImagerWindow *w, indigo_property *property) {
	double exp_elapsed, exp_time;

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
	w->set_widget_state(w->m_focusing_preview_button, property->state);
	w->set_widget_state(w->m_preview_button, property->state);
}

void update_guider_stats(ImagerWindow *w, indigo_property *property) {
	double ref_x = 0, ref_y = 0;
	double d_ra = 0, d_dec = 0;
	double cor_ra = 0, cor_dec = 0;
	double rmse_ra = 0, rmse_dec = 0, dither_rmse = 0;
	double d_x = 0, d_y = 0;
	int size = 0, frame_count = -1;
	bool is_guiding_process_on = false;
	bool is_dithering = false;

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
				w->m_guider_process = 1;
				if (dither_rmse == 0) {
					w->set_guider_label(INDIGO_OK_STATE, " Guiding... ");
				} else {
					is_dithering = true;
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
		if (p->state == INDIGO_BUSY_STATE) {
			w->move_guider_reference(ref_x, ref_y);
			if (w->m_guider_process && w->m_guide_log && conf.guider_save_log && frame_count > 1) {
				char time_str[255];
				get_timestamp(time_str);
				fprintf(w->m_guide_log, "\"%s\", %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %d\n", time_str, d_x, d_y, d_ra, d_dec, cor_ra, cor_dec, is_dithering);
				fflush(w->m_guide_log);
			}
		} else {
			w->move_guider_reference(0, 0);
		}
	}

	char label_str[50];
	snprintf(label_str, 50, "%+.2f  %+.2f", d_ra, d_dec);
	w->set_text(w->m_guider_rd_drift_label, label_str);

	snprintf(label_str, 50, "%+.2f  %+.2f", d_x, d_y);
	w->set_text(w->m_guider_xy_drift_label, label_str);

	snprintf(label_str, 50, "%+.2f  %+.2f", cor_ra, cor_dec);
	w->set_text(w->m_guider_pulse_label, label_str);

	snprintf(label_str, 50, "%.2f  %.2f", rmse_ra, rmse_dec);
	w->set_text(w->m_guider_rmse_label, label_str);
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
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_i_gain_ra);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_i_gain_dec);
		} else if (client_match_item(&property->items[i], AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME)) {
			configure_spinbox(w, &property->items[i], property->perm, w->m_guide_is);
		}
	}
}

void log_guide_header(ImagerWindow *w, char *device_name) {
	char time_str[255];
	if (w->m_guide_log == nullptr || device_name == nullptr) return;

	get_timestamp(time_str);
	fprintf(w->m_guide_log, "\nGuiding started at %s\n", time_str);
	indigo_property *p = properties.get(device_name, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME);
	if (p) {
		char method[INDIGO_VALUE_SIZE] = {0};
		for (int i = 0; i < p->count; i++ ) {
			if (p->items[i].sw.value) {
				strncpy(method, p->items[i].label, INDIGO_VALUE_SIZE);
				break;
			}
		}
		p = properties.get(device_name, AGENT_GUIDER_SELECTION_PROPERTY_NAME);
		if (p) {
			int radius = 0;
			int star_count = 0;
			int edge_clipping = 0;
			for (int i = 0; i < p->count; i++) {
				if (client_match_item(&p->items[i], AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME)) {
					radius = (int)round(p->items[i].number.value);
				} else if (client_match_item(&p->items[i], AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME)) {
					star_count = (int)p->items[i].number.value;
				} else if (client_match_item(&p->items[i], AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME)) {
					edge_clipping = (int)p->items[i].number.value;
				}
			}
			fprintf(w->m_guide_log, "Method: '%s', Parameters: Radius = %d px, Star count = %d, Edge clipping = %d px\n", method, radius, star_count, edge_clipping);
		}

		p = properties.get(device_name, AGENT_GUIDER_SETTINGS_PROPERTY_NAME);
		double exposure = 0;
		double delay = 0;
		double min_err = 0;
		double min_pulse = 0;
		double max_pulse = 0;
		double ra_aggr = 0;
		double dec_aggr = 0;
		double i_gain_ra = 0;
		double i_gain_dec = 0;
		double stack = 0;
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME)) {
				exposure = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_DELAY_ITEM_NAME)) {
				delay = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM_NAME)) {
				min_err = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM_NAME)) {
				min_pulse = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM_NAME)) {
				max_pulse = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_AGG_RA_ITEM_NAME)) {
				ra_aggr = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM_NAME)) {
				dec_aggr = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM_NAME)) {
				i_gain_ra = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM_NAME)) {
				i_gain_dec = p->items[i].number.value;
			} else if (client_match_item(&p->items[i], AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME)) {
				stack = p->items[i].number.value;
			}
		}
		fprintf(
			w->m_guide_log,
			"Guider Settings: Exp = %.3f s, Delay = %.3f s, Min Error = %.3f px, Min Pulse = %.3f s, Max Pulse = %.3f s\n",
			exposure,
			delay,
			min_err,
			min_pulse,
			max_pulse
		);
		fprintf(
			w->m_guide_log,
			"PI Settings: RA Aggr = %.3f %%, Dec Aggr = %.3f %%, RA I Gain = %.3f, Dec I Gain = %.3f, Stack = %.0f\n",
			ra_aggr,
			dec_aggr,
			i_gain_ra,
			i_gain_dec,
			stack
		);
	}
	fprintf(w->m_guide_log, "Timestamp, X Dif, Y Dif, RA Dif, Dec Dif, Ra Correction, Dec Correction, Dithering\n");
	fflush(w->m_guide_log);
}

void agent_guider_start_process_change(ImagerWindow *w, indigo_property *property) {
	char time_str[255];
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
				if (w->m_guide_log && conf.guider_save_log && w->m_guider_process == 0) {
					w->m_guider_process = 1;
					log_guide_header(w, property->device);
				}
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
			if (w->m_guide_log && conf.guider_save_log && w->m_guider_process) {
				w->m_guider_process = 0;
				get_timestamp(time_str);
				fprintf(w->m_guide_log, "Process failed at %s\n", time_str);
				fflush(w->m_guide_log);
			}
		} else {
			w->set_guider_label(INDIGO_IDLE_STATE, " Stopped ");
			if (w->m_guide_log && conf.guider_save_log && w->m_guider_process == 1) {
				w->m_guider_process = 0;
				get_timestamp(time_str);
				fprintf(w->m_guide_log, "Guiding finished at %s\n", time_str);
				fflush(w->m_guide_log);
			}
		}
		w->set_enabled(w->m_guider_preview_button, true);
		w->set_enabled(w->m_guider_calibrate_button, true);
		w->set_enabled(w->m_guider_guide_button, true);
		w->move_guider_reference(0, 0);
	}
}


void ImagerWindow::on_window_log(indigo_property* property, char *message) {
	char timestamp[255];
	char log_line[512];

	if (!message) return;

	get_time(timestamp);

	if (property) {
		switch (property->state) {
		case INDIGO_ALERT_STATE:
			mLog->setTextColor(QColor::fromRgb(224, 0, 0));
			break;
		case INDIGO_BUSY_STATE:
			mLog->setTextColor(QColor::fromRgb(255, 165, 0));
			break;
		default:
			mLog->setTextColor(Qt::white);
			break;
		}
		snprintf(log_line, 512, "%s %s.%s: %s", timestamp, property->device, property->name, message);
	} else {
		mLog->setTextColor(Qt::white);
		snprintf(log_line, 512, "%s %s", timestamp, message);
	}
	indigo_log("[message] %s\n", log_line);
	mLog->append(log_line);
	mLog->verticalScrollBar()->setValue(mLog->verticalScrollBar()->maximum());
}

void condigure_guider_overlays(ImagerWindow *w, char *device, indigo_property *property) {
	if (device == nullptr) return;

	indigo_property *p = nullptr;
	if (property) {
		p = property;
	} else {
		p = properties.get(device, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME);
	}

	if (p) {
		if (indigo_get_switch(p, AGENT_GUIDER_DETECTION_SELECTION_ITEM_NAME)) {
			w->show_guider_selection(true);
			w->show_guider_extra_selection(true);
			w->show_guider_reference(true);
			w->show_guider_edge_clipping(false);
		} else if (indigo_get_switch(p, AGENT_GUIDER_DETECTION_DONUTS_ITEM_NAME)){
			w->show_guider_selection(false);
			w->show_guider_extra_selection(false);
			w->show_guider_reference(false);
			w->show_guider_edge_clipping(true);
		} else {
			w->show_guider_selection(false);
			w->show_guider_extra_selection(false);
			w->show_guider_reference(false);
			w->show_guider_edge_clipping(false);
		}
	}
}

void ImagerWindow::property_define(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE] = {0};
	char selected_guider_agent[INDIGO_VALUE_SIZE] = {0};
	char selected_mount_agent[INDIGO_VALUE_SIZE] = {0};
	static pthread_mutex_t l_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (!strncmp(property->device, "Server", 6)) {
		if (client_match_device_property(property, property->device, "LOAD")) {
			if (properties.get(property->device, property->name)) return;

			// load indigo_agent_image and indigo_agent_guider
			bool imager_not_loaded = true;
			bool guider_not_loaded = true;
			bool mount_not_loaded = true;

			indigo_property *p = properties.get(property->device, "DRIVERS");
			if (p) {
				imager_not_loaded = !indigo_get_switch(p, "indigo_agent_imager");
				guider_not_loaded = !indigo_get_switch(p, "indigo_agent_guider");
				mount_not_loaded = !indigo_get_switch(p, "indigo_agent_mount");
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
				if (mount_not_loaded) {
					static const char *items[] = { "DRIVER" };
					static const char *values[] = { "indigo_agent_mount" };
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
				if (build < 143) {
					sprintf(message, "WARNING: Some features will not work on '%s' running Indigo %s as Ain requires 2.0-143 or newer!", property->device, item->text.value);
					on_window_log(nullptr, message);
				}
			}
		}
		return;
	}
	if(!strncmp(property->device, "Imager Agent", 12)) {
		QString name = QString(property->device);
		add_combobox_item(m_agent_imager_select, name, name);
	}
	if(!strncmp(property->device, "Guider Agent", 12)) {
		QString name = QString(property->device);
		add_combobox_item(m_agent_guider_select, name, name);
	}
	if(!strncmp(property->device, "Mount Agent", 11)) {
		QString name = QString(property->device);
		add_combobox_item(m_agent_mount_select, name, name);
	}
	if (
		(!get_selected_imager_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) &&
		(!get_selected_guider_agent(selected_guider_agent) || strncmp(property->device, "Guider Agent", 12)) &&
		(!get_selected_mount_agent(selected_mount_agent) || strncmp(property->device, "Mount Agent", 11))
	) {
		return;
	}
	if (client_match_device_property(property, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_camera_select);
		if (indigo_get_switch(property, "NONE")) {
			m_exposure_progress->setRange(0, 1);
			m_exposure_progress->setValue(0);
			m_exposure_progress->setFormat("Exposure: Idle");
			m_process_progress->setRange(0, 1);
			m_process_progress->setValue(0);
			m_process_progress->setFormat("Process: Idle");
			m_focusing_progress->setRange(0, 1);
			m_focusing_progress->setValue(0);
			m_focusing_progress->setFormat("Focusing: Idle");
			set_enabled(m_preview_button, true);
			set_widget_state(m_preview_button, INDIGO_OK_STATE);
			set_enabled(m_exposure_button, true);
			set_widget_state(m_exposure_button, INDIGO_OK_STATE);
			m_exposure_button->setIcon(QIcon(":resource/record.png"));
			set_enabled(m_focusing_button, true);
			set_widget_state(m_focusing_button, INDIGO_OK_STATE);
			set_enabled(m_focusing_preview_button, true);
			set_widget_state(m_focusing_preview_button, INDIGO_OK_STATE);
		}
	}
	if (client_match_device_property(property, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_focuser_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_frame_size_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_IMAGE_FORMAT_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_frame_format_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME)) {
		add_items_to_combobox_filtered(this, property, "Guider Agent", m_dither_agent_select);
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
		reset_filter_names(this, property);
		indigo_property *p = properties.get(property->device, WHEEL_SLOT_PROPERTY_NAME);
		if (p) set_filter_selected(this, p);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_PROPERTY_NAME)) {
		set_filter_selected(this, property);
	}
	if (client_match_device_property(property, selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		update_agent_imager_pause_process_property(this, property, m_pause_button);
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
	if (client_match_device_property(property, selected_agent, CCD_GAIN_PROPERTY_NAME) ||
	    client_match_device_property(property, selected_agent, CCD_OFFSET_PROPERTY_NAME)) {
		update_agent_imager_gain_offset_property(this, property);
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, CCD_PREVIEW_PROPERTY_NAME)) {
		QtConcurrent::run([=]() {
			change_agent_ccd_peview(selected_guider_agent, (bool)conf.guider_save_bandwidth);
		});
	}
	if (client_match_device_property(property, selected_guider_agent, CCD_JPEG_SETTINGS_PROPERTY_NAME)) {
		set_enabled(m_guider_save_bw_select, true);
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_guider_camera_select);
		if (indigo_get_switch(property, "NONE")) {
			set_enabled(m_guider_calibrate_button, true);
			set_widget_state(m_guider_calibrate_button, INDIGO_OK_STATE);
			set_enabled(m_guider_guide_button, true);
			set_widget_state(m_guider_guide_button, INDIGO_OK_STATE);
			set_enabled(m_guider_preview_button, true);
			set_widget_state(m_guider_preview_button, INDIGO_OK_STATE);
		}
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_guider_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME)) {
		update_guider_selection_property(this, property);
		set_enabled(m_guider_subframe_select, true);
		QtConcurrent::run([=]() {
			change_guider_agent_subframe(selected_guider_agent);
		});
		condigure_guider_overlays(this, property->device, nullptr);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_STATS_PROPERTY_NAME)) {
		update_guider_stats(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_detection_mode_select);
		condigure_guider_overlays(this, property->device, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_dec_guiding_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME)) {
		update_guider_settings(this, property);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_START_PROCESS_PROPERTY_NAME)) {
		agent_guider_start_process_change(this, property);
	}

	// Mount agent
	if (client_match_device_property(property, selected_mount_agent, FILTER_MOUNT_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_mount_select);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
		update_mount_ra_dec(this, property, true);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME)) {
		update_mount_az_alt(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_LST_TIME_PROPERTY_NAME)) {
		update_mount_lst(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_PARK_PROPERTY_NAME)) {
		update_mount_park(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_TRACKING_PROPERTY_NAME)) {
		update_mount_track(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_SLEW_RATE_PROPERTY_NAME)) {
		update_mount_slew_rates(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_SIDE_OF_PIER_PROPERTY_NAME)) {
		update_mount_side_of_pier(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, FILTER_GPS_LIST_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_mount_gps_select);
	}
	QString geographic_coords_property = QString("GPS_") + QString(GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, geographic_coords_property.toUtf8().constData())) {
		update_mount_gps_lon_lat_elev(this, property);
	}
	QString utc_time_property = QString("GPS_") + QString(UTC_TIME_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, utc_time_property.toUtf8().constData())) {
		update_mount_gps_utc(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, GPS_STATUS_PROPERTY_NAME)) {
		update_mount_gps_status(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, AGENT_SITE_DATA_SOURCE_PROPERTY_NAME)) {
		add_items_to_combobox(this, property, m_mount_coord_source_select);
		if(indigo_get_switch(property, AGENT_SITE_DATA_SOURCE_HOST_ITEM_NAME)) {
			set_enabled(m_mount_lon_input, true);
			set_enabled(m_mount_lat_input, true);
		} else {
			set_enabled(m_mount_lon_input, false);
			set_enabled(m_mount_lat_input, false);
		}
	}
	if (client_match_device_property(property, selected_mount_agent, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
		update_mount_lon_lat(this, property);
	}
	utc_time_property = QString("MOUNT_") + QString(UTC_TIME_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, utc_time_property.toUtf8().constData())) {
		update_mount_utc(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, AGENT_SET_HOST_TIME_PROPERTY_NAME)) {
		update_mount_agent_sync_time(this, property);
	}
}

void ImagerWindow::on_property_define(indigo_property* property, char *message) {
	property_define(property, message);
	properties.create(property);
}


void ImagerWindow::on_property_change(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE] = {0};
	char selected_guider_agent[INDIGO_VALUE_SIZE] = {0};
	char selected_mount_agent[INDIGO_VALUE_SIZE] = {0};

	if (
		(!get_selected_imager_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) &&
		(!get_selected_guider_agent(selected_guider_agent) || strncmp(property->device, "Guider Agent", 12)) &&
		(!get_selected_mount_agent(selected_mount_agent) || strncmp(property->device, "Mount Agent", 11))
	) {
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
		add_items_to_combobox(this, property, m_frame_format_select);
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
		reset_filter_names(this, property);
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
		update_agent_imager_pause_process_property(this, property, m_pause_button);
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
	if (client_match_device_property(property, selected_agent, CCD_GAIN_PROPERTY_NAME) ||
	    client_match_device_property(property, selected_agent, CCD_OFFSET_PROPERTY_NAME)) {
		update_agent_imager_gain_offset_property(this, property);
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
		condigure_guider_overlays(this, property->device, property);
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

	// Mount Agent
	if (client_match_device_property(property, selected_mount_agent, FILTER_MOUNT_LIST_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_mount_select);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME)) {
		update_mount_ra_dec(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME)) {
		update_mount_az_alt(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_LST_TIME_PROPERTY_NAME)) {
		update_mount_lst(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_PARK_PROPERTY_NAME)) {
		update_mount_park(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_TRACKING_PROPERTY_NAME)) {
		update_mount_track(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_SLEW_RATE_PROPERTY_NAME)) {
		update_mount_slew_rates(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_SIDE_OF_PIER_PROPERTY_NAME)) {
		update_mount_side_of_pier(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, FILTER_GPS_LIST_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_mount_gps_select);
	}
	QString geographic_coords_property = QString("GPS_") + QString(GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, geographic_coords_property.toUtf8().constData())) {
		update_mount_gps_lon_lat_elev(this, property);
	}
	QString utc_time_property = QString("GPS_") + QString(UTC_TIME_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, utc_time_property.toUtf8().constData())) {
		update_mount_gps_utc(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, GPS_STATUS_PROPERTY_NAME)) {
		update_mount_gps_status(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, AGENT_SITE_DATA_SOURCE_PROPERTY_NAME)) {
		change_combobox_selection(this, property, m_mount_coord_source_select);
		if(indigo_get_switch(property, AGENT_SITE_DATA_SOURCE_HOST_ITEM_NAME)) {
			set_enabled(m_mount_lon_input, true);
			set_enabled(m_mount_lat_input, true);
		} else {
			set_enabled(m_mount_lon_input, false);
			set_enabled(m_mount_lat_input, false);
		}
	}
	if (client_match_device_property(property, selected_mount_agent, GEOGRAPHIC_COORDINATES_PROPERTY_NAME)) {
		update_mount_lon_lat(this, property);
	}
	utc_time_property = QString("MOUNT_") + QString(UTC_TIME_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, utc_time_property.toUtf8().constData())) {
		update_mount_utc(this, property);
	}
	if (client_match_device_property(property, selected_mount_agent, AGENT_SET_HOST_TIME_PROPERTY_NAME)) {
		update_mount_agent_sync_time(this, property);
	}

	properties.create(property);
}

void ImagerWindow::property_delete(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE] = {0};
	char selected_guider_agent[INDIGO_VALUE_SIZE] = {0};
	char selected_mount_agent[INDIGO_VALUE_SIZE] = {0};

	indigo_debug("[REMOVE REMOVE REMOVE REMOVE REMOVE] %s.%s\n", property->device, property->name);
	if (
		(!get_selected_imager_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) &&
		(!get_selected_guider_agent(selected_guider_agent) || strncmp(property->device, "Guider Agent", 12)) &&
		(!get_selected_mount_agent(selected_mount_agent) || strncmp(property->device, "Mount Agent", 11))
	) {
		return;
	}
	indigo_debug("[REMOVE REMOVE REMOVE REMOVE] %s.%s\n", property->device, property->name);

	if (client_match_device_property(property, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_camera_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_focuser_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		clear_combobox(m_frame_size_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_IMAGE_FORMAT_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		clear_combobox(m_frame_format_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		clear_combobox(m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		clear_combobox(m_dither_agent_select);
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
		clear_combobox(m_filter_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		set_enabled(m_cooler_onoff, false);
		set_checkbox_checked(m_cooler_onoff, false);
	}
	if (client_match_device_property(property, selected_agent, CCD_COOLER_POWER_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("REMOVE %s", property->name);
		set_text(m_cooler_pwr, "");
	}
	if (client_match_device_property(property, selected_agent, CCD_TEMPERATURE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {
		indigo_debug("REMOVE %s", property->name);
		set_text(m_current_temp, "");
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
	if (client_match_device_property(property, selected_agent, CCD_GAIN_PROPERTY_NAME) ||
	    client_match_device_property(property, selected_agent, CCD_OFFSET_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_agent)) {

		set_spinbox_value(m_imager_gain, 0);
		set_enabled(m_imager_gain, false);

		set_spinbox_value(m_imager_offset, 0);
		set_enabled(m_imager_offset, false);
	}

	// Guider Agent
	if (client_match_device_property(property, selected_guider_agent, FILTER_CCD_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_guider_camera_select);
	}
	if (client_match_device_property(property, selected_guider_agent, FILTER_GUIDER_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_guider_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		clear_combobox(m_detection_mode_select);
	}
	if (client_match_device_property(property, selected_guider_agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		clear_combobox(m_dec_guiding_select);
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

		set_spinbox_value(m_guide_i_gain_ra, 0);
		set_enabled(m_guide_i_gain_ra, false);

		set_spinbox_value(m_guide_i_gain_dec, 0);
		set_enabled(m_guide_i_gain_dec, false);

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

		set_spinbox_value(m_guide_star_count, 0);
		set_enabled(m_guide_star_count, false);

		set_spinbox_value(m_guide_edge_clipping, 0);
		set_enabled(m_guide_edge_clipping, false);

		set_enabled(m_guider_subframe_select, false);

		show_guider_selection(false);
		show_guider_extra_selection(false);
		show_guider_reference(false);
		show_guider_edge_clipping(false);
		move_resize_guider_selection(0, 0, 1);
		move_guider_reference(0, 0);
		resize_guider_edge_clipping(0);
		set_guider_label(INDIGO_IDLE_STATE, " Stopped ");
	}
	if (client_match_device_property(property, selected_guider_agent, CCD_JPEG_SETTINGS_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_guider_agent)) {
		set_enabled(m_guider_save_bw_select, false);
	}

	// Mount Agent
	if (client_match_device_property(property, selected_mount_agent, FILTER_MOUNT_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_mount_select);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_EQUATORIAL_COORDINATES_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_lcd(m_mount_ra_label, "0: 00:00.0", INDIGO_IDLE_STATE);
		set_lcd(m_mount_dec_label, "0' 00 00.0", INDIGO_IDLE_STATE);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
#ifdef USE_LCD
		QString zero_str(indigo_dtos(0, "%d' %02d %04.1f"));
		set_lcd(m_mount_az_label, zero_str, INDIGO_IDLE_STATE);
		set_lcd(m_mount_alt_label, zero_str, INDIGO_IDLE_STATE);
#else
		QString zero_str(indigo_dtos(0, "%d° %02d' %04.1f\""));
		set_text(m_mount_az_label, zero_str);
		set_widget_state(m_mount_az_label, INDIGO_IDLE_STATE);
		set_text(m_mount_alt_label, zero_str);
		set_widget_state(m_mount_alt_label, INDIGO_IDLE_STATE);
#endif
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_HORIZONTAL_COORDINATES_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
#ifdef USE_LCD
		QString zero_str(indigo_dtos(0, "%d: %02d:%04.1f"));
		set_lcd(m_mount_lst_label, zero_str, INDIGO_IDLE_STATE);
#else
		QString zero_str(indigo_dtos(0, "%d:%02d:%04.1f"));
		set_text(m_mount_lst_label, zero_str);
		set_widget_state(m_mount_lst_label, INDIGO_IDLE_STATE);
#endif
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_PARK_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_checkbox_state(m_mount_park_cbox, Qt::Unchecked);
		set_text(m_mount_park_cbox, "Park");
		set_enabled(m_mount_park_cbox, false);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_TRACKING_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_checkbox_state(m_mount_track_cbox, Qt::Unchecked);
		set_text(m_mount_track_cbox, "Tracking");
		set_enabled(m_mount_track_cbox, false);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_SLEW_RATE_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_enabled(m_mount_guide_rate_cbox, false);
		set_enabled(m_mount_center_rate_cbox, false);
		set_enabled(m_mount_find_rate_cbox, false);
		set_enabled(m_mount_max_rate_cbox, false);
		set_checkbox_state(m_mount_guide_rate_cbox, Qt::Unchecked);
		set_checkbox_state(m_mount_center_rate_cbox, Qt::Unchecked);
		set_checkbox_state(m_mount_find_rate_cbox, Qt::Unchecked);
		set_checkbox_state(m_mount_max_rate_cbox, Qt::Unchecked);
	}
	if (client_match_device_property(property, selected_mount_agent, MOUNT_SIDE_OF_PIER_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_text(m_mount_side_of_pier_label, "Side of pier: Unknown");
		set_widget_state(m_mount_side_of_pier_label, INDIGO_IDLE_STATE);
	}
	if (client_match_device_property(property, selected_mount_agent, FILTER_GPS_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		clear_combobox(m_mount_gps_select);
	}
	QString geographic_coords_property = QString("GPS_") + QString(GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, geographic_coords_property.toUtf8().constData()) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		QString zero_str(indigo_dtos(0, "%d:%02d:%04.1f"));
		set_text(m_gps_latitude, zero_str);
		set_widget_state(m_gps_latitude, INDIGO_IDLE_STATE);
		set_text(m_gps_longitude, zero_str);
		set_widget_state(m_gps_longitude, INDIGO_IDLE_STATE);
		set_text(m_gps_elevation, "0");
		set_widget_state(m_gps_elevation, INDIGO_IDLE_STATE);
	}
	QString utc_time_property = QString("GPS_") + QString(UTC_TIME_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, utc_time_property.toUtf8().constData()) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_text(m_gps_utc, "");
		set_widget_state(m_gps_utc, INDIGO_IDLE_STATE);
	}
	if (client_match_device_property(property, selected_mount_agent, GPS_STATUS_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_text(m_gps_status, "Unknown");
		set_widget_state(m_gps_status, INDIGO_IDLE_STATE);
	}
	if (client_match_device_property(property, selected_mount_agent,  GEOGRAPHIC_COORDINATES_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		QString zero_str(indigo_dtos(0, "%d:%02d:%04.1f"));
		set_text(m_mount_latitude, zero_str);
		set_widget_state(m_mount_latitude, INDIGO_IDLE_STATE);
		set_text(m_mount_longitude, zero_str);
		set_widget_state(m_mount_longitude, INDIGO_IDLE_STATE);
		set_lineedit_text(m_mount_lon_input, "");
		set_enabled(m_mount_lon_input, false);
		set_lineedit_text(m_mount_lat_input, "");
		set_enabled(m_mount_lat_input, false);
	}
	utc_time_property = QString("MOUNT_") + QString(UTC_TIME_PROPERTY_NAME);
	if (client_match_device_property(property, selected_mount_agent, utc_time_property.toUtf8().constData()) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_text(m_mount_utc, "");
		set_widget_state(m_mount_utc, INDIGO_IDLE_STATE);
	}
	if (client_match_device_property(property, selected_mount_agent, AGENT_SET_HOST_TIME_PROPERTY_NAME) ||
	    client_match_device_no_property(property, selected_mount_agent)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		set_checkbox_state(m_mount_sync_time_cbox, Qt::Unchecked);
		set_enabled(m_mount_sync_time_cbox, false);
	}
}

void ImagerWindow::on_property_delete(indigo_property* property, char *message) {

	if (
		(strncmp(property->device, "Imager Agent", 12)) &&
		(strncmp(property->device, "Guider Agent", 12)) &&
		(strncmp(property->device, "Mount Agent", 11))
	) {
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
			remove_combobox_item(m_agent_imager_select, index);
			if (selected_index == index) {
				set_combobox_current_index(m_agent_imager_select, 0);
				on_agent_selected(0);
			}
			indigo_debug("[REMOVE imager agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[NOT FOUND imager agent] %s\n", name.toUtf8().data());
		}

		selected_index = m_agent_guider_select->currentIndex();
		index = m_agent_guider_select->findText(name);
		if (index >= 0) {
			remove_combobox_item(m_agent_guider_select, index);
			if (selected_index == index) {
				set_combobox_current_index(m_agent_guider_select, 0);
				on_guider_agent_selected(0);
			}
			indigo_debug("[REMOVE guider agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[NOT FOUND guider agent] %s\n", name.toUtf8().data());
		}

		selected_index = m_agent_mount_select->currentIndex();
		index = m_agent_mount_select->findText(name);
		if (index >= 0) {
			remove_combobox_item(m_agent_mount_select, index);
			if (selected_index == index) {
				set_combobox_current_index(m_agent_mount_select, 0);
				on_mount_agent_selected(0);
			}
			indigo_debug("[REMOVE mount agent] %s\n", name.toUtf8().data());
		} else {
			indigo_debug("[NOT FOUND mount agent] %s\n", name.toUtf8().data());
		}
	}
	properties.remove(property);
	free(property);
}
