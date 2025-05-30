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

#include <libgen.h>
#include <imagerwindow.h>
#include <propertycache.h>
#include <utils.h>
#include <logger.h>
#include <conf.h>

void ImagerWindow::change_config_agent_load(const char *agent, const char *config, bool unload) const {
	indigo_change_switch_property_1(nullptr, agent, AGENT_CONFIG_SETUP_PROPERTY_NAME, AGENT_CONFIG_SETUP_UNLOAD_DRIVERS_ITEM_NAME, unload);
	indigo_change_switch_property_1(nullptr, agent, AGENT_CONFIG_LOAD_PROPERTY_NAME, config, true);
}

void ImagerWindow::change_config_agent_save(const char *agent, const char *config, bool autosave) const {
	indigo_change_switch_property_1(nullptr, agent, AGENT_CONFIG_SETUP_PROPERTY_NAME, AGENT_CONFIG_SETUP_AUTOSAVE_ITEM_NAME, autosave);
	indigo_change_text_property_1_raw(nullptr, agent, AGENT_CONFIG_SAVE_PROPERTY_NAME, AGENT_CONFIG_SAVE_NAME_ITEM_NAME, config);
}

void ImagerWindow::change_config_agent_delete(const char *agent, const char *config) const {
	indigo_change_text_property_1_raw(nullptr, agent, AGENT_CONFIG_DELETE_PROPERTY_NAME, AGENT_CONFIG_DELETE_NAME_ITEM_NAME, config);
}

void ImagerWindow::change_ccd_frame_property(const char *agent) const {
	static const char *items[] = {
		CCD_FRAME_LEFT_ITEM_NAME,
		CCD_FRAME_TOP_ITEM_NAME,
		CCD_FRAME_WIDTH_ITEM_NAME,
		CCD_FRAME_HEIGHT_ITEM_NAME
	};
	static double values[4];
	values[0] = (double)m_roi_x->value();
	values[1] = (double)m_roi_y->value();
	values[2] = (double)m_roi_w->value();
	values[3] = (double)m_roi_h->value();
	indigo_change_number_property(nullptr, agent, CCD_FRAME_PROPERTY_NAME, 4, items, values);
}

void ImagerWindow::change_ccd_exposure_property(const char *agent, QDoubleSpinBox *exp_time) const {
	static double value;
	value = (double)exp_time->value();
	indigo_change_number_property_1(nullptr, agent, CCD_EXPOSURE_PROPERTY_NAME, CCD_EXPOSURE_ITEM_NAME, value);
}

void ImagerWindow::change_solver_exposure_settings_property(const char *agent, QDoubleSpinBox *exp_time) const {
	static double value;
	value = (double)exp_time->value();
	indigo_change_number_property_1(
		nullptr,
		agent,
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_PROPERTY_NAME,
		AGENT_PLATESOLVER_EXPOSURE_SETTINGS_EXPOSURE_ITEM_NAME,
		value
	);
}

void ImagerWindow::change_ccd_abort_exposure_property(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, CCD_ABORT_EXPOSURE_PROPERTY_NAME, CCD_ABORT_EXPOSURE_ITEM_NAME, true);
}

void ImagerWindow::change_ccd_mode_property(const char *agent, QComboBox *frame_size_select) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, frame_size_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, CCD_MODE_PROPERTY_NAME, selected_mode, true);
}

void ImagerWindow::change_focuser_reverse_property(const char *agent) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, m_focuser_reverse_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, FOCUSER_REVERSE_MOTION_PROPERTY_NAME, selected_mode, true);
}

void ImagerWindow::change_ccd_image_format_property(const char *agent) const {
	static char selected_format[INDIGO_NAME_SIZE];
	strncpy(selected_format, m_frame_format_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, CCD_IMAGE_FORMAT_PROPERTY_NAME, selected_format, true);
}

void ImagerWindow::change_guider_ditherung_strategy_property(const char *agent) const {
	static char selected_strategy[INDIGO_NAME_SIZE];
	strncpy(selected_strategy, m_dither_strategy_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_DITHERING_STRATEGY_PROPERTY_NAME, selected_strategy, true);
}

void ImagerWindow::change_ccd_upload_property(const char *agent, const char *item_name) const {
	static char item[INDIGO_NAME_SIZE];
	strncpy(item, item_name, INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, CCD_UPLOAD_MODE_PROPERTY_NAME, item, true);
}

void ImagerWindow::change_ccd_localmode_property(const char *agent, const QString &object_name) {
	QString object;
	if (object_name.trimmed().isEmpty() || object_name.trimmed() == DEFAULT_OBJECT_NAME) {
		m_object_name_str = QString(DEFAULT_OBJECT_NAME);
		object = "";
	} else {
		m_object_name_str = object_name.trimmed();
		object = m_object_name_str;
	}
	indigo_property *p = properties.get(agent, CCD_LOCAL_MODE_PROPERTY_NAME);
	if (p == nullptr) return;

	if (indigo_get_item(p, CCD_LOCAL_MODE_OBJECT_ITEM_NAME) != nullptr) {
		indigo_debug("LOCAL_MODE property has OBJECT item");
		static const char *items[] = {
			CCD_LOCAL_MODE_OBJECT_ITEM_NAME,
			CCD_LOCAL_MODE_PREFIX_ITEM_NAME
		};

		char object_cstr[INDIGO_VALUE_SIZE];
		strncpy(object_cstr, m_object_name_str.toUtf8().constData(), INDIGO_VALUE_SIZE);
		char *values[] {
			object_cstr,
			"%o_%-D_%F_%C_%M"
		};
		indigo_change_text_property(nullptr, agent, CCD_LOCAL_MODE_PROPERTY_NAME, 2, items, (const char **)values);
	} else {
		indigo_debug("LOCAL_MODE property does not have OBJECT item");
		m_remote_object_name = object;
		add_fits_keyword_string(agent, "OBJECT", m_object_name_str.toUtf8().constData());

		char value[INDIGO_VALUE_SIZE];
		snprintf(value, INDIGO_VALUE_SIZE, "%s_%%-D_%%F_%%C_%%M", (char *)m_object_name_str.toUtf8().constData());
		indigo_debug("Setting prefix to: %s", value);

		indigo_change_text_property_1_raw(nullptr, agent, CCD_LOCAL_MODE_PROPERTY_NAME, CCD_LOCAL_MODE_PREFIX_ITEM_NAME, value);
	}
}

void ImagerWindow::init_ccd_localmode_property(const char *agent) {
	char value[INDIGO_VALUE_SIZE];
	snprintf(value, INDIGO_VALUE_SIZE, "%%o_%%-D_%%F_%%C_%%M");
	indigo_debug("Setting prefix to: %s", value);

	indigo_change_text_property_1_raw(nullptr, agent, CCD_LOCAL_MODE_PROPERTY_NAME, CCD_LOCAL_MODE_PREFIX_ITEM_NAME, value);
}

void ImagerWindow::add_fits_keyword_string(const char *agent, const char *keyword, const QString &value) const {
	if (value.isEmpty()) {
		indigo_change_text_property_1_raw(nullptr, agent, CCD_REMOVE_FITS_HEADERS_PROPERTY_NAME, CCD_REMOVE_FITS_HEADER_KEYWORD_ITEM_NAME, keyword);
	} else {
		static const char *items[] = {
			CCD_SET_FITS_HEADER_KEYWORD_ITEM_NAME,
			CCD_SET_FITS_HEADER_VALUE_ITEM_NAME
		};
		QString object = QString("'") + value.trimmed() + QString("'");
		char *values[] {
			(char *)keyword,
			object.toUtf8().data()
		};
		indigo_change_text_property(nullptr, agent, CCD_SET_FITS_HEADER_PROPERTY_NAME, 2, items, (const char **)values);
	}
}

void ImagerWindow::request_file_download(const char *agent, const char *file_name) const {
	indigo_debug("Requested remote: %s", file_name);
	indigo_change_text_property_1_raw(nullptr, agent, AGENT_IMAGER_DOWNLOAD_FILE_PROPERTY_NAME, AGENT_IMAGER_DOWNLOAD_FILE_ITEM_NAME, file_name);
}

void ImagerWindow::request_file_remove(const char *agent, const char *file_name) const {
	indigo_debug("Requested delete: %s", file_name);
	indigo_change_text_property_1_raw(nullptr, agent, AGENT_IMAGER_DELETE_FILE_PROPERTY_NAME, AGENT_IMAGER_DELETE_FILE_ITEM_NAME, file_name);
}

void ImagerWindow::set_related_solver_agent(const char *related_agent) const {
	// Select related agent
	char selected_solver_agent[INDIGO_NAME_SIZE] = {0};
	char related_agent_trimmed[INDIGO_NAME_SIZE] = {0};
	char agent_prefix[INDIGO_NAME_SIZE] = {0};

	get_selected_solver_agent(selected_solver_agent);

	strncpy(related_agent_trimmed, related_agent, INDIGO_NAME_SIZE);
	strncpy(agent_prefix, related_agent, INDIGO_NAME_SIZE);

	char *service = strrchr(related_agent_trimmed, '@');
	if (service != nullptr) {
		*(service - 1) = '\0';
	}

	set_agent_releated_agent(selected_solver_agent, related_agent_trimmed, true);
}

void ImagerWindow::set_related_agent(const char *agent, const char *related_agent) const {
	char agent_trimmed[INDIGO_NAME_SIZE] = {0};
	char related_agent_trimmed[INDIGO_NAME_SIZE] = {0};
	char old_agent[INDIGO_NAME_SIZE] = {0};
	char agent_prefix[INDIGO_NAME_SIZE] = {0};

	strncpy(agent_trimmed, agent, INDIGO_NAME_SIZE);
	strncpy(related_agent_trimmed, related_agent, INDIGO_NAME_SIZE);
	strncpy(agent_prefix, related_agent, INDIGO_NAME_SIZE);

	char *service1 = strrchr(related_agent_trimmed, '@');
	char *service2 = strrchr(agent_trimmed, '@');
	if (service1 != nullptr && service2 != nullptr && !strcmp(service1, service2)) {
		*(service1 - 1) = '\0';
		*(service2 - 1) = '\0';
	}

	char *service3 = strchr(agent_prefix, '@');
	if (service3 != nullptr) {
		*(service3 - 1) = '\0';
	}

	bool change = true;
	old_agent[0] = '\0';
	indigo_property *p = properties.get(agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (p->items[i].sw.value && !strncmp(p->items[i].name, agent_prefix, strlen(agent_prefix))) {
				strncpy(old_agent, p->items[i].name, INDIGO_NAME_SIZE);
				if (!strcmp(old_agent, related_agent_trimmed)) {
					change = false;
					break;
				}
			}
		}
		if (change) {
			indigo_debug("[SET RELATED AGENT] %s '%s' chnaged '%s' -> '%s' (prefix '%s')\n", __FUNCTION__, agent, old_agent, related_agent_trimmed, agent_prefix);
			change_related_agent(agent, old_agent, related_agent_trimmed);
		}
	}
}

void ImagerWindow::set_base_agent_relations() const {
	char selected_mount_agent[INDIGO_NAME_SIZE] = {0};
	char selected_imager_agent[INDIGO_NAME_SIZE] = {0};
	char selected_guider_agent[INDIGO_NAME_SIZE] = {0};
	char selected_solver_agent[INDIGO_NAME_SIZE] = {0};
	get_selected_mount_agent(selected_mount_agent);
	get_selected_imager_agent(selected_imager_agent);
	get_selected_guider_agent(selected_guider_agent);
	get_selected_solver_agent(selected_solver_agent);

	set_related_agent(selected_mount_agent, selected_imager_agent);
	set_related_agent(selected_imager_agent, selected_mount_agent);

	set_related_agent(selected_imager_agent, selected_guider_agent);
	set_related_agent(selected_guider_agent, selected_imager_agent);

	set_related_agent(selected_solver_agent, selected_mount_agent);
	set_related_agent(selected_mount_agent, selected_solver_agent);

	set_related_agent(selected_mount_agent, selected_guider_agent);
	set_related_agent(selected_guider_agent, selected_mount_agent);
}

void ImagerWindow::change_related_agent(const char *agent, const char *old_agent, const char *new_agent) const {
	indigo_debug("[RELATED AGENT] %s '%s': %s -> %s\n", __FUNCTION__, agent, old_agent, new_agent);
	if (old_agent[0] != '\0') {
		indigo_change_switch_property_1(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, old_agent, false);
	}
	if (new_agent[0] != '\0') {
		indigo_change_switch_property_1(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, new_agent, true);
	}
}

void ImagerWindow::change_agent_guider_dithering_property(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_DITHERING_AMOUNT_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_DITHERING_TIME_LIMIT_ITEM_NAME
	};
	static double values[2];
	values[0] = (double)m_dither_aggr->value();
	values[1] = (double)m_dither_to->value();

	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 2, items, values);
}

void ImagerWindow::change_agent_imager_dithering_property(const char *agent) const {
	static double value = (double)m_dither_skip->value();
	indigo_change_number_property_1(nullptr, agent, AGENT_IMAGER_BATCH_PROPERTY_NAME, AGENT_IMAGER_BATCH_FRAMES_TO_SKIP_BEFORE_DITHER_ITEM_NAME, value);
}

void ImagerWindow::change_focuser_temperature_compensation_steps(const char *agent) const {
	double value = (double)m_focuser_temperature_compensation_steps->value();
	indigo_change_number_property_1(nullptr, agent, FOCUSER_COMPENSATION_PROPERTY_NAME, FOCUSER_COMPENSATION_ITEM_NAME, value);
}

void ImagerWindow::change_agent_gain_property(const char *agent, QSpinBox *ccd_gain) const {
	double value = (double)ccd_gain->value();
	indigo_change_number_property_1(nullptr, agent, CCD_GAIN_PROPERTY_NAME, CCD_GAIN_ITEM_NAME, value);
}

void ImagerWindow::change_agent_offset_property(const char *agent, QSpinBox *ccd_offset) const {
	double value = (double)ccd_offset->value();
	indigo_change_number_property_1(nullptr, agent, CCD_OFFSET_PROPERTY_NAME, CCD_OFFSET_ITEM_NAME, value);
}

void ImagerWindow::change_agent_lens_profile_property(const char *agent, QDoubleSpinBox *guider_focal_lenght) const {
	double value = (double)guider_focal_lenght->value();
	indigo_change_number_property_1(nullptr, agent, CCD_LENS_PROPERTY_NAME, CCD_LENS_FOCAL_LENGTH_ITEM_NAME, value);
}

void ImagerWindow::change_agent_binning_property(const char *agent) const {
	static const char *items[] = {
		CCD_BIN_HORIZONTAL_ITEM_NAME,
		CCD_BIN_VERTICAL_ITEM_NAME
	};
	static double values[2];
	values[0] = (double)m_imager_bin_x->value();
	values[1] = (double)m_imager_bin_y->value();

	indigo_change_number_property(nullptr, agent, CCD_BIN_PROPERTY_NAME, 2, items, values);
}

void ImagerWindow::change_ccd_frame_type_property(const char *agent) const {
	static char selected_type[INDIGO_NAME_SIZE];
	strncpy(selected_type, m_frame_type_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, CCD_FRAME_TYPE_PROPERTY_NAME, selected_type, true);
}

void ImagerWindow::change_agent_batch_property(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME,
		AGENT_IMAGER_BATCH_DELAY_ITEM_NAME,
		AGENT_IMAGER_BATCH_COUNT_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_exposure_time->value();
	values[1] = (double)m_exposure_delay->value();
	values[2] = (double)m_frame_count->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_BATCH_PROPERTY_NAME, 3, items, values);
}

void ImagerWindow::change_agent_batch_property_for_focus(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_focuser_exposure_time->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_BATCH_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_star_selection(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_SELECTION_X_ITEM_NAME,
		AGENT_IMAGER_SELECTION_Y_ITEM_NAME,
		AGENT_IMAGER_SELECTION_RADIUS_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_star_x->value();
	values[1] = (double)m_star_y->value();
	values[2] = (double)m_focus_star_radius->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME, 3, items, values);
}

void ImagerWindow::change_agent_star_count(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_SELECTION_STAR_COUNT_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_focus_star_count->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_guider_agent_star_selection(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SELECTION_X_ITEM_NAME,
		AGENT_GUIDER_SELECTION_Y_ITEM_NAME,
		AGENT_GUIDER_SELECTION_RADIUS_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_guide_star_x->value();
	values[1] = (double)m_guide_star_y->value();
	values[2] = (double)m_guide_star_radius->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME, 3, items, values);
}

void ImagerWindow::clear_imager_agent_star_selection(const char *agent) const {
	if (m_has_clear_focuser_selection) {
		indigo_change_switch_property_1(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_GUIDER_CLEAR_SELECTION_ITEM_NAME, true);
	} else {
		static const char *items[] = {
			AGENT_IMAGER_SELECTION_X_ITEM_NAME,
			AGENT_IMAGER_SELECTION_Y_ITEM_NAME,
		};
		static double values[2] = {0};
		indigo_change_number_property(nullptr, agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME, 2, items, values);
	}
}

void ImagerWindow::clear_guider_agent_star_selection(const char *agent) const {
	if (m_has_clear_guider_selection) {
		indigo_change_switch_property_1(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_GUIDER_CLEAR_SELECTION_ITEM_NAME, true);
	} else {
		const int max_stars = 25;

		char item_names[max_stars * 2][INDIGO_NAME_SIZE];
		static char *items[max_stars * 2];

		indigo_property *p = properties.get((char*)agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME);
		if (p == nullptr) return;
		indigo_item *item = indigo_get_item(p, AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME);
		if (item == nullptr) return;
		int count = item->number.value;

		if (count > max_stars) {
			indigo_error("Too many selected stars. Should be < %s", max_stars);
			count = max_stars * 2;
		} else {
			count *= 2;
		}

		snprintf(item_names[0], INDIGO_VALUE_SIZE, AGENT_GUIDER_SELECTION_X_ITEM_NAME);
		items[0] = item_names[0];
		snprintf(item_names[1], INDIGO_VALUE_SIZE, AGENT_GUIDER_SELECTION_Y_ITEM_NAME);
		items[1] = item_names[1];
		int index=2;
		for (int i = 2; i < count; i += 2) {
			snprintf(item_names[i], INDIGO_VALUE_SIZE, "%s_%d", AGENT_GUIDER_SELECTION_X_ITEM_NAME, index);
			items[i] = item_names[i];
			snprintf(item_names[i + 1], INDIGO_VALUE_SIZE, "%s_%d", AGENT_GUIDER_SELECTION_Y_ITEM_NAME, index);
			items[i + 1] = item_names[i + 1];
			index++;
		}

		static double values[max_stars * 2] = {0};

		indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME, count, (const char**)items, values);
	}
}

void ImagerWindow::change_guider_agent_star_count(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SELECTION_STAR_COUNT_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_guide_star_count->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_guider_agent_subframe(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SELECTION_SUBFRAME_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_guider_subframe_select->currentIndex() * 5; /* 0, 5, 10 slection radii */
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_guider_agent_edge_clipping(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SELECTION_EDGE_CLIPPING_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_guide_edge_clipping->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_focuser_subframe(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_SELECTION_SUBFRAME_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_focuser_subframe_select->currentIndex() * 5; /* 0, 5, 10 slection radii */
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_SELECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_jpeg_settings_property(const char *agent, const int jpeg_quality, const double black_threshold, const double white_threshold, const double target_bg,  const double clipping_point) {
	static const char *items[] = {
		CCD_JPEG_SETTINGS_QUALITY_ITEM_NAME,
		CCD_JPEG_SETTINGS_TARGET_BACKGROUND_ITEM_NAME,
		CCD_JPEG_SETTINGS_CLIPPING_POINT_ITEM_NAME,
		// these are obsolete - kept for compatibility
		CCD_JPEG_SETTINGS_BLACK_ITEM_NAME,
		CCD_JPEG_SETTINGS_WHITE_ITEM_NAME,
		CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM_NAME,
		CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM_NAME
	};
	static double values[7];
	values[0] = (double)jpeg_quality;
	values[1] = target_bg;
	values[2] = clipping_point;
	// these are obsolete - kept for compatibility
	values[3] = -1;
	values[4] = -1;
	values[5] = black_threshold;
	values[6] = white_threshold;

	indigo_change_number_property(nullptr, agent, CCD_JPEG_SETTINGS_PROPERTY_NAME, 7, items, values);
}

void ImagerWindow::change_focus_estimator_property(const char *agent) const {
	static char selected_estimator[INDIGO_NAME_SIZE];
	strncpy(selected_estimator, m_focus_estimator_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, AGENT_IMAGER_FOCUS_ESTIMATOR_PROPERTY_NAME, selected_estimator, true);
}

void ImagerWindow::change_detection_mode_property(const char *agent) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, m_detection_mode_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME, selected_mode, true);
}

void ImagerWindow::change_dec_guiding_property(const char *agent) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, m_dec_guiding_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME, selected_mode, true);
}

void ImagerWindow::change_agent_focus_params_property(const char *agent, bool set_backlash) const {
	static const char *items[] = {
		AGENT_IMAGER_FOCUS_ITERATIVE_INITIAL_ITEM_NAME,
		AGENT_IMAGER_FOCUS_ITERATIVE_FINAL_ITEM_NAME,
		AGENT_IMAGER_FOCUS_UCURVE_STEP_ITEM_NAME,
		AGENT_IMAGER_FOCUS_UCURVE_SAMPLES_ITEM_NAME,
		AGENT_IMAGER_FOCUS_STACK_ITEM_NAME,
		AGENT_IMAGER_FOCUS_REPEAT_ITEM_NAME,
		AGENT_IMAGER_FOCUS_DELAY_ITEM_NAME,
		AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME
	};
	static double values[8];
	values[0] = (double)m_initial_step->value();
	values[1] = (double)m_final_step->value();
	values[2] = (double)m_ucurve_step->value();
	values[3] = (double)m_ucurve_samples->value();
	values[4] = (double)m_focus_stack->value();
	values[5] = 0;
	values[6] = 0;
	values[7] = (double)m_focus_backlash->value();
	if (set_backlash) {
		indigo_change_number_property(nullptr, agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME, 8, items, values);
	} else {
		indigo_change_number_property(nullptr, agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME, 7, items, values);
	}
}

void ImagerWindow::change_agent_focuser_bl_overshoot(const char *agent) const {
	double value = m_focus_bl_overshoot->value();
	indigo_change_number_property_1(nullptr, agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME, AGENT_IMAGER_FOCUS_BACKLASH_OVERSHOOT_ITEM_NAME, value);
}

void ImagerWindow::change_agent_start_exposure_property(const char *agent) const {
	change_agent_start_process(agent, AGENT_IMAGER_START_EXPOSURE_ITEM_NAME);
}

void ImagerWindow::change_agent_start_sequence_property(const char *agent) const {
	indigo_property *p = properties.get(agent, AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (!strcmp(p->items[i].label, AIN_SEQUENCE_NAME)) {
				indigo_change_switch_property_1(
					nullptr,
					agent,
					AGENT_SCRIPTING_EXECUTE_SCRIPT_PROPERTY_NAME,
					p->items[i].name,
					true
				);
				indigo_debug("Sequence started %s", p->items[i].name);
				return;
			}
		}
	}
	indigo_debug("Sequence not found");
}

void ImagerWindow::change_agent_start_focusing_property(const char *agent) const {
	change_agent_start_process(agent, AGENT_IMAGER_START_FOCUSING_ITEM_NAME) ;
}

void ImagerWindow::change_agent_start_preview_property(const char *agent) const {
	change_agent_start_process(agent, AGENT_IMAGER_START_PREVIEW_ITEM_NAME);
}


void ImagerWindow::change_focuser_position_property(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, FOCUSER_ON_POSITION_SET_PROPERTY_NAME, FOCUSER_ON_POSITION_SET_GOTO_ITEM_NAME, true);
	static double position;
	position = (double)m_focus_position->value();
	indigo_change_number_property_1(nullptr, agent, FOCUSER_POSITION_PROPERTY_NAME, FOCUSER_POSITION_ITEM_NAME, position);
}


void ImagerWindow::change_focuser_steps_property(const char *agent) const {
	static const char *items[] = {
		FOCUSER_STEPS_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_focus_steps->value();
	indigo_change_number_property(nullptr, agent, FOCUSER_STEPS_PROPERTY_NAME, 1, items, values);
}


void ImagerWindow::change_focuser_focus_in_property(const char *agent) const {
	static const char *items[] = {
		FOCUSER_DIRECTION_MOVE_INWARD_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, FOCUSER_DIRECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_focuser_focus_out_property(const char *agent) const {
	static const char *items[] = {
		FOCUSER_DIRECTION_MOVE_OUTWARD_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, FOCUSER_DIRECTION_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_pause_process_property(const char *agent, bool wait_exposure, bool pause) const {
	//indigo_error("change_agent_pause_process_property: %s -> %s", agent, pause ? "true" : "false");
	if(wait_exposure) {
		indigo_change_switch_property_1(nullptr, agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME, AGENT_PAUSE_PROCESS_WAIT_ITEM_NAME, pause);
	} else {
		indigo_change_switch_property_1(nullptr, agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME, AGENT_PAUSE_PROCESS_ITEM_NAME, pause);
	}
}

void ImagerWindow::change_agent_abort_process_property(const char *agent) const {
	static const char *items[] = {
		AGENT_ABORT_PROCESS_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_ABORT_PROCESS_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_ccd_peview(const char *agent, bool enable) const {
	if (enable) {
		indigo_change_switch_property_1(nullptr, agent, CCD_PREVIEW_PROPERTY_NAME, CCD_PREVIEW_ENABLED_ITEM_NAME, true);
	} else {
		indigo_change_switch_property_1(nullptr, agent, CCD_PREVIEW_PROPERTY_NAME, CCD_PREVIEW_DISABLED_ITEM_NAME, true);
	}
}

void ImagerWindow::change_wheel_slot_property(const char *agent) const {
	static char selected_filter[INDIGO_NAME_SIZE];
	strncpy(selected_filter, m_filter_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_change_switch_property_1(nullptr, agent, AGENT_WHEEL_FILTER_PROPERTY_NAME, selected_filter, true);
}

void ImagerWindow::change_cooler_onoff_property(const char *agent) const {
	if (m_cooler_onoff->isChecked()) {
		static const char *items[] = {
			CCD_COOLER_ON_ITEM_NAME
		};
		static bool values[] = {
			true
		};
		indigo_change_switch_property(nullptr, agent, CCD_COOLER_PROPERTY_NAME, 1, items, values);
	} else {
		static const char *items[] = {
			CCD_COOLER_OFF_ITEM_NAME
		};
		static bool values[] = {
			true
		};
		indigo_change_switch_property(nullptr, agent, CCD_COOLER_PROPERTY_NAME, 1, items, values);
	}
}

void ImagerWindow::change_ccd_temperature_property(const char *agent) const {
	static const char *items[] = {
		CCD_TEMPERATURE_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_set_temp->value();
	indigo_change_number_property(nullptr, agent, CCD_TEMPERATURE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_start_guide_property(const char *agent) const {
	change_agent_start_process(agent, AGENT_GUIDER_START_GUIDING_ITEM_NAME);
}

void ImagerWindow::change_agent_start_calibrate_property(const char *agent) const {
	change_agent_start_process(agent, AGENT_GUIDER_START_CALIBRATION_ITEM_NAME);
}

void ImagerWindow::change_guider_agent_exposure(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_EXPOSURE_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_DELAY_ITEM_NAME
	};
	static double values[2];
	values[0] = (double)m_guider_exposure->value();
	values[1] = (double)m_guider_delay->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 2, items, values);
}

void ImagerWindow::change_guider_agent_callibration(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_STEP_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_BACKLASH_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_ANGLE_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_SPEED_RA_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_SPEED_DEC_ITEM_NAME
	};
	static double values[5];
	values[0] = (double)m_guide_cal_step->value();
	values[1] = (double)m_guide_dec_backlash->value();
	values[2] = (double)m_guide_rotation->value();
	values[3] = (double)m_guide_ra_speed->value();
	values[4] = (double)m_guide_dec_speed->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 5, items, values);
}

void ImagerWindow::change_guider_agent_pulse_min_max(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_MIN_ERR_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_MIN_PULSE_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_MAX_PULSE_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_guide_min_error->value();
	values[1] = (double)m_guide_min_pulse->value();
	values[2] = (double)m_guide_max_pulse->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 3, items, values);
}

void ImagerWindow::change_guider_agent_aggressivity(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_AGG_RA_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_AGG_DEC_ITEM_NAME
	};
	static double values[2];
	values[0] = (double)m_guide_ra_aggr->value();
	values[1] = (double)m_guide_dec_aggr->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 2, items, values);
}

void ImagerWindow::change_guider_agent_i(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_I_GAIN_RA_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_I_GAIN_DEC_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_guide_i_gain_ra->value();
	values[1] = (double)m_guide_i_gain_dec->value();
	values[2] = (double)m_guide_is->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 3, items, values);
}

void ImagerWindow::change_guider_agent_reverse_dec(const char *agent) const {
	if (m_guider_reverse_dec_cbox->isChecked()) {
		indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY_NAME, AGENT_GUIDER_FLIP_REVERSES_DEC_ENABLED_ITEM_NAME, true);
	} else {
		indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_FLIP_REVERSES_DEC_PROPERTY_NAME, AGENT_GUIDER_FLIP_REVERSES_DEC_DISABLED_ITEM_NAME, true);
	}
}

void ImagerWindow::change_guider_agent_apply_dec_backlash(const char *agent) const {
	if (m_guider_apply_backlash_cbox->isChecked()) {
		indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY_NAME, AGENT_GUIDER_APPLY_DEC_BACKLASH_ENABLED_ITEM_NAME, true);
	} else {
		indigo_change_switch_property_1(nullptr, agent, AGENT_GUIDER_APPLY_DEC_BACKLASH_PROPERTY_NAME, AGENT_GUIDER_APPLY_DEC_BACKLASH_DISABLED_ITEM_NAME, true);
	}
}

void ImagerWindow::change_mount_agent_equatorial(const char *agent, bool sync) const {
	static const char *items[] = {
		AGENT_MOUNT_TARGET_COORDINATES_RA_ITEM_NAME,
		AGENT_MOUNT_TARGET_COORDINATES_DEC_ITEM_NAME
	};
	static double values[2];
	values[0] = indigo_stod((char*)m_mount_ra_input->text().trimmed().toStdString().c_str());
	values[1] = indigo_stod((char*)m_mount_dec_input->text().trimmed().toStdString().c_str());
	indigo_change_number_property(nullptr, agent, AGENT_MOUNT_TARGET_COORDINATES_PROPERTY_NAME, 2, items, values);

	if (sync) {
		indigo_change_switch_property_1(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_MOUNT_START_SYNC_ITEM_NAME, true);
	} else {
		indigo_change_switch_property_1(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, AGENT_MOUNT_START_SLEW_ITEM_NAME, true);
	}
}

void ImagerWindow::change_mount_agent_location(const char *agent, QString property_prefix) const {
	static const char *items[] = {
		GEOGRAPHIC_COORDINATES_LONGITUDE_ITEM_NAME,
		GEOGRAPHIC_COORDINATES_LATITUDE_ITEM_NAME
	};
	static double values[2];
	values[0] = indigo_stod((char*)m_mount_lon_input->text().trimmed().toStdString().c_str());
	values[1] = indigo_stod((char*)m_mount_lat_input->text().trimmed().toStdString().c_str());
	QString coords_property = QString(GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
	if (!property_prefix.isEmpty()) {
		coords_property = property_prefix + QString("_") + QString(GEOGRAPHIC_COORDINATES_PROPERTY_NAME);
	}
	indigo_change_number_property(nullptr, agent, coords_property.toUtf8().constData(), 2, items, values);
}

void ImagerWindow::change_mount_agent_abort(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, MOUNT_ABORT_MOTION_PROPERTY_NAME, MOUNT_ABORT_MOTION_ITEM_NAME, true);
}

void ImagerWindow::change_solver_agent_abort(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, AGENT_PLATESOLVER_ABORT_PROPERTY_NAME, AGENT_PLATESOLVER_ABORT_ITEM_NAME, true);
}

void ImagerWindow::change_solver_agent_hints_property(const char *agent) const {
	static const char *items[] = {
		AGENT_PLATESOLVER_HINTS_RA_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_DEC_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_RADIUS_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_SCALE_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_PARITY_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_DEPTH_ITEM_NAME,
		AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM_NAME
	};
	static double values[8];
	values[0] = indigo_stod((char*)m_solver_ra_hint->text().trimmed().toStdString().c_str());
	values[1] = indigo_stod((char*)m_solver_dec_hint->text().trimmed().toStdString().c_str());
	values[2] = (double)m_solver_radius_hint->value();
	values[3] = (double)m_solver_scale_hint->value() / 3600.0;
	values[4] = (double)m_solver_parity_hint->value();
	values[5] = (double)m_solver_ds_hint->value();
	values[6] = (double)m_solver_depth_hint->value();
	values[7] = (double)m_solver_tlimit_hint->value();

	indigo_change_number_property(nullptr, agent, AGENT_PLATESOLVER_HINTS_PROPERTY_NAME, 8, items, values);
}


void ImagerWindow::clear_solver_agent_releated_agents(const char *agent) const {
	const int max_agents = 32;

	static char *item_names[max_agents];
	static bool values[max_agents] = {false};

	indigo_property *p = properties.get((char*)agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME);
	if (p == nullptr) return;

	int count = p->count;

	if (count > max_agents) {
		count = max_agents;
	}

	for (int i = 0; i < count; i++) {
		item_names[i] = p->items[i].name;
	}

	indigo_change_switch_property(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, count, (const char **)item_names, values);
}

void ImagerWindow::set_agent_releated_agent(const char *agent, const char *related_agent, bool select) const {
	if (!strncmp(related_agent, "Imager Agent", 12) || !strncmp(related_agent, "Guider Agent", 12)) {
		const int max_agents = 32;

		static char *item_names[max_agents];
		static bool values[max_agents] = {false};

		indigo_property *p = properties.get((char*)agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME);
		if (p == nullptr) return;

		int count = p->count;

		if (count > max_agents) {
			count = max_agents;
		}

		int item_count = 0;
		for (int i = 0; i < count; i++) {
			if (!strncmp(p->items[i].name, "Imager Agent", 12) || !strncmp(p->items[i].name, "Guider Agent", 12)) {
				if (!strncmp(related_agent, p->items[i].name, INDIGO_NAME_SIZE)) {
					values[item_count] = true;
				} else {
					values[item_count] = false;
				}
				item_names[item_count] = p->items[i].name;
				item_count++;
			}
		}
		indigo_change_switch_property(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, item_count, (const char **)item_names, values);
	} else {
		indigo_change_switch_property_1(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, related_agent, select);
	}
}

void ImagerWindow::disable_auto_solving(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, AGENT_PLATESOLVER_SOLVE_IMAGES_PROPERTY_NAME, AGENT_PLATESOLVER_SOLVE_IMAGES_DISABLED_ITEM_NAME, true);
	//indigo_change_switch_property_1(nullptr, agent, AGENT_PLATESOLVER_SYNC_PROPERTY_NAME, AGENT_PLATESOLVER_SYNC_DISABLED_ITEM_NAME, true);
}

void ImagerWindow::change_agent_start_process(const char *agent, const char *item) const {
	indigo_change_switch_property_1(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, item, true);
}

bool ImagerWindow::get_solver_relations(char *selected_mount_agent, char* selected_solver_agent, char *selected_image_agent, char *selected_solver_source, QComboBox *solver_source_select) {
	char domain_name[INDIGO_NAME_SIZE];

	get_selected_solver_agent(selected_solver_agent);
	get_indigo_device_domain(domain_name, selected_solver_agent);
	// if() do checks

	QString solver_source = solver_source_select->currentText();
	if (solver_source == "None" || solver_source == "") return false;
	strncpy(selected_image_agent, solver_source.toUtf8().constData(), INDIGO_NAME_SIZE);
	strncpy(selected_solver_source, selected_image_agent, INDIGO_NAME_SIZE);
	add_indigo_device_domain(selected_image_agent, domain_name);
	m_last_solver_source = QString(selected_solver_source);

	get_selected_mount_agent(selected_mount_agent);
	remove_indigo_device_domain(selected_mount_agent, 1);

	indigo_log("[SELECTED] %s image_agent = '%s'\n", __FUNCTION__, selected_image_agent);
	indigo_log("[SELECTED] %s solver_source = '%s'\n", __FUNCTION__, selected_solver_source);
	indigo_log("[SELECTED] %s mount_agent = '%s'\n", __FUNCTION__, selected_mount_agent);
	indigo_log("[SELECTED] %s solver_agent = '%s'\n", __FUNCTION__, selected_solver_agent);
	indigo_log("[SELECTED] %s domain_name = '%s'\n", __FUNCTION__, domain_name);

	return true;
}

void ImagerWindow::trigger_solve() {
	static char selected_image_agent[INDIGO_NAME_SIZE];
	static char selected_mount_agent[INDIGO_NAME_SIZE];
	static char selected_solver_agent[INDIGO_NAME_SIZE];
	static char selected_solver_source[INDIGO_NAME_SIZE];
	static int image_size = 0;
	static unsigned char *image_data = nullptr;
	static QString solver_source;
	QString file_name;

	solver_source = m_solver_source_select1->currentData().toString();
	if (
		!get_solver_relations(
			selected_mount_agent,
			selected_solver_agent,
			selected_image_agent,
			selected_solver_source,
			m_solver_source_select1
		)
	) {
		return;
	}

	indigo_property *p = properties.get(selected_solver_agent, AGENT_PLATESOLVER_WCS_PROPERTY_NAME);
	if (p && p->state == INDIGO_BUSY_STATE ) {
		QtConcurrent::run([&]() {
			m_property_mutex.lock();
			change_solver_agent_abort(selected_solver_agent);
			m_property_mutex.unlock();
		});
		return;
	}

	if (solver_source == AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME) {
		char path[PATH_LEN];
		strncpy(path, m_image_path, PATH_LEN);
		QString qlocation(dirname(path));
		if (m_image_path[0] == '\0') qlocation = QDir::toNativeSeparators(QDir::homePath());
		file_name = QFileDialog::getOpenFileName(
			this,
			tr("Upload Image"),
			qlocation,
			QString("FITS (*.fit *.FIT *.fits *.FITS *.fts *.FTS );;Indigo RAW (*.raw *.RAW);;FITS / Indigo RAW (*.fit *FIT *.fits *.FITS *.fts *.FTS *.raw *.RAW);;JPEG / (*.jpg *.JPG *.jpeg *.JPEG *.jpe *.JPE);;All Files (*)"),
			&m_selected_filter
		);
		if(file_name.isNull()) {
			return;
		} else {
			if(!open_image(file_name, &image_size, &image_data)) {
				return;
			}
		}
	}

	QtConcurrent::run([&]() {
		m_property_mutex.lock();
		if (solver_source == AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME) {
			indigo_property *p = properties.get(selected_solver_agent, AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME);
			indigo_item *image_item = indigo_get_item(p, AGENT_PLATESOLVER_IMAGE_ITEM_NAME);
			if (image_item) {
				indigo_result res = indigo_change_blob_property_1(
					nullptr,
					selected_solver_agent,
					AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME,
					AGENT_PLATESOLVER_IMAGE_ITEM_NAME,
					image_data,
					image_size,
					"",
					image_item->blob.url
				);
				if (res != INDIGO_OK) {
					Logger::instance().log(nullptr, "Failed to upload file for solving");
				} else {
					Logger::instance().log(nullptr, "File uploaded for solving");
					update_solver_widgets_at_start(selected_image_agent, selected_solver_agent);
				}
			}
		} else {
			set_agent_releated_agent(selected_solver_agent, selected_mount_agent, true);
			set_agent_releated_agent(selected_solver_agent, selected_solver_source, true);

			change_solver_exposure_settings_property(selected_solver_agent, m_solver_exposure1);
			change_agent_start_process(selected_solver_agent, AGENT_PLATESOLVER_START_SOLVE_ITEM_NAME);

			update_solver_widgets_at_start(selected_image_agent, selected_solver_agent);
		}
		indigo_safe_free(image_data);
		image_data = nullptr;
		image_size = 0;
		m_property_mutex.unlock();
	});
}

void ImagerWindow::trigger_solve_and_sync(bool recenter) {
	static char selected_image_agent[INDIGO_NAME_SIZE];
	static char selected_mount_agent[INDIGO_NAME_SIZE];
	static char selected_solver_agent[INDIGO_NAME_SIZE];
	static char selected_solver_source[INDIGO_NAME_SIZE];
	static bool recenter_cache;

	recenter_cache = recenter;

	if (
		!get_solver_relations(
			selected_mount_agent,
			selected_solver_agent,
			selected_image_agent,
			selected_solver_source,
			m_solver_source_select2
		)
	) {
		return;
	}

	QtConcurrent::run([&]() {
		m_property_mutex.lock();

		set_agent_releated_agent(selected_solver_agent, selected_mount_agent, true);
		set_agent_releated_agent(selected_solver_agent, selected_solver_source, true);

		change_solver_exposure_settings_property(selected_solver_agent, m_solver_exposure2);

		if (recenter_cache) {
			change_agent_start_process(selected_solver_agent, AGENT_PLATESOLVER_START_CENTER_ITEM_NAME);
		} else {
			change_agent_start_process(selected_solver_agent, AGENT_PLATESOLVER_START_SYNC_ITEM_NAME);
		}

		update_solver_widgets_at_start(selected_image_agent, selected_solver_agent);
		m_property_mutex.unlock();
	});
}

void ImagerWindow::change_solver_goto_settings(const char *agent) const {
	static const char *items[] = {
		AGENT_PLATESOLVER_GOTO_SETTINGS_RA_ITEM_NAME,
		AGENT_PLATESOLVER_GOTO_SETTINGS_DEC_ITEM_NAME
	};
	static double values[2];
	values[0] = indigo_stod((char*)m_mount_ra_input->text().trimmed().toStdString().c_str());
	values[1] = indigo_stod((char*)m_mount_dec_input->text().trimmed().toStdString().c_str());
	indigo_change_number_property(nullptr, agent, AGENT_PLATESOLVER_GOTO_SETTINGS_PROPERTY_NAME, 2, items, values);
}

void ImagerWindow::trigger_precise_goto() {
	static char selected_image_agent[INDIGO_NAME_SIZE];
	static char selected_mount_agent[INDIGO_NAME_SIZE];
	static char selected_solver_agent[INDIGO_NAME_SIZE];
	static char selected_solver_source[INDIGO_NAME_SIZE];

	if (
		!get_solver_relations(
			selected_mount_agent,
			selected_solver_agent,
			selected_image_agent,
			selected_solver_source,
			m_solver_source_select2
		)
	) {
		return;
	}

	QtConcurrent::run([&]() {
		m_property_mutex.lock();

		set_agent_releated_agent(selected_solver_agent, selected_mount_agent, true);
		set_agent_releated_agent(selected_solver_agent, selected_solver_source, true);

		change_solver_exposure_settings_property(selected_solver_agent, m_solver_exposure2);
		change_solver_goto_settings(selected_solver_agent);
		change_agent_start_process(selected_solver_agent, AGENT_PLATESOLVER_START_PRECISE_GOTO_ITEM_NAME);

		update_solver_widgets_at_start(selected_image_agent, selected_solver_agent);
		m_property_mutex.unlock();
	});
}

void ImagerWindow::trigger_polar_alignment(bool recalculate) {
	static char selected_image_agent[INDIGO_NAME_SIZE];
	static char selected_mount_agent[INDIGO_NAME_SIZE];
	static char selected_solver_agent[INDIGO_NAME_SIZE];
	static char selected_solver_source[INDIGO_NAME_SIZE];
	static bool recalculate_cache;

	recalculate_cache = recalculate;

	if (
		!get_solver_relations(
			selected_mount_agent,
			selected_solver_agent,
			selected_image_agent,
			selected_solver_source,
			m_solver_source_select3
		)
	) {
		return;
	}

	QtConcurrent::run([&]() {
		m_property_mutex.lock();

		set_agent_releated_agent(selected_solver_agent, selected_mount_agent, true);
		set_agent_releated_agent(selected_solver_agent, selected_solver_source, true);

		change_solver_exposure_settings_property(selected_solver_agent, m_solver_exposure3);

		if (recalculate_cache) {
			change_agent_start_process(selected_solver_agent, AGENT_PLATESOLVER_START_RECALCULATE_PA_ERROR_ITEM_NAME);
		} else {
			change_agent_start_process(selected_solver_agent, AGENT_PLATESOLVER_START_CALCULATE_PA_ERROR_ITEM_NAME);
		}

		update_solver_widgets_at_start(selected_image_agent, selected_solver_agent);
		m_property_mutex.unlock();
	});
}

void ImagerWindow::change_solver_agent_pa_settings(const char *agent) const {
	static const char *items[] = {
		AGENT_PLATESOLVER_PA_SETTINGS_EXPOSURE_ITEM_NAME,
		AGENT_PLATESOLVER_PA_SETTINGS_HA_MOVE_ITEM_NAME,
		AGENT_PLATESOLVER_PA_SETTINGS_COMPENSATE_REFRACTION_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_solver_exposure3->value();
	values[1] = (double)m_pa_move_ha->value();
	values[2] = (double)m_pa_refraction_cbox->isChecked();
	indigo_change_number_property(nullptr, agent, AGENT_PLATESOLVER_PA_SETTINGS_PROPERTY_NAME, 3, items, values);
}

#define MAX_ITEMS 256

void ImagerWindow::change_scripting_agent_sequence(const char *agent, QString sequence) const {
	static char items[MAX_ITEMS][INDIGO_NAME_SIZE] = {0};
	static char values[MAX_ITEMS][INDIGO_VALUE_SIZE] = {0};
	static char *items_ptr[MAX_ITEMS];
	static char *values_ptr[MAX_ITEMS];

	indigo_property *p = properties.get((char*)agent, AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME);
	if (p) {
		// Do Not run new scripts by default
		indigo_change_switch_property_1(
			nullptr,
			agent,
			AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY_NAME,
			AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM_NAME,
			false
		);
		// Load sequencer code in the scripting agent
		indigo_change_text_property_1_raw(
			nullptr,
			agent,
			AGENT_SCRIPTING_RUN_SCRIPT_PROPERTY_NAME,
			AGENT_SCRIPTING_RUN_SCRIPT_ITEM_NAME,
			m_sequencer_code.toUtf8().constData()
		);
	}

	p = properties.get((char*)agent, AGENT_SCRIPTING_ON_LOAD_SCRIPT_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (!strncmp(p->items[i].label, AIN_SEQUENCE_NAME, INDIGO_NAME_SIZE)) {
				// Delete Ain Sequence
				indigo_change_text_property_1(
					nullptr,
					agent,
					AGENT_SCRIPTING_DELETE_SCRIPT_PROPERTY_NAME,
					AGENT_SCRIPTING_DELETE_SCRIPT_NAME_ITEM_NAME,
					AIN_SEQUENCE_NAME
				);
				break;
			}
		}
		static const char *items[] = {
			AGENT_SCRIPTING_ADD_SCRIPT_NAME_ITEM_NAME,
			AGENT_SCRIPTING_ADD_SCRIPT_ITEM_NAME
		};
		// Add new AinSequence
		static char *script;
		script = (char*)indigo_safe_malloc_copy(sequence.length() + 1, sequence.toUtf8().data());
		static char *values[2];
		values[0] = AIN_SEQUENCE_NAME;
		values[1] = script;
		indigo_change_text_property(nullptr, agent, AGENT_SCRIPTING_ADD_SCRIPT_PROPERTY_NAME, 2, items, (const char**)values);
		indigo_safe_free(script);

		// Wait for AinSequence to appear
		bool found = false;
		const int MAX_RETRYS = 20;
		const int SLEEP_TIME = 100000;  // 0.1 sec
		int retrys = 0;
		do {
			for (int i = 0; i < p->count; i++) {
				if (!strncmp(p->items[i].label, AIN_SEQUENCE_NAME, INDIGO_NAME_SIZE)) {
					found = true;
					break;
				}
			}
			if (!found) {
				indigo_debug("Waiting for %s to appear #%d", AIN_SEQUENCE_NAME, retrys);
				indigo_usleep(SLEEP_TIME);
			}
			retrys++;
		} while (!found || retrys > MAX_RETRYS);
	}
}

void ImagerWindow::change_rotator_position_property(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_GOTO_ITEM_NAME, true);
	static double position;
	position = (double)m_rotator_position->value();
	indigo_change_number_property_1(nullptr, agent, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, position);
}

void ImagerWindow::change_rotator_sync_property(const char *agent) const {
	indigo_change_switch_property_1(nullptr, agent, ROTATOR_ON_POSITION_SET_PROPERTY_NAME, ROTATOR_ON_POSITION_SET_SYNC_ITEM_NAME, true);
	static double position;
	position = (double)m_rotator_position->value();
	indigo_change_number_property_1(nullptr, agent, ROTATOR_POSITION_PROPERTY_NAME, ROTATOR_POSITION_ITEM_NAME, position);
}
