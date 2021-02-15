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

#include <imagerwindow.h>
#include <propertycache.h>

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
	static const char *items[] = {
		CCD_EXPOSURE_ITEM_NAME,
	};
	static double values[1];
	values[0] = (double)exp_time->value();
	indigo_change_number_property(nullptr, agent, CCD_EXPOSURE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_ccd_abort_exposure_property(const char *agent) const {
	static const char *items[] = {
		CCD_ABORT_EXPOSURE_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, CCD_ABORT_EXPOSURE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_ccd_mode_property(const char *agent) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, m_frame_size_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	static const char * items[] = {
		selected_mode
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, CCD_MODE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_ccd_image_format_property(const char *agent) const {
	static char selected_format[INDIGO_NAME_SIZE];
	strncpy(selected_format, m_frame_format_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	static const char * items[] = {
		selected_format
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, CCD_IMAGE_FORMAT_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_ccd_upload_property(const char *agent, const char *item_name) const {
	static const char * items[] = {
		item_name
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, CCD_UPLOAD_MODE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_related_dither_agent(const char *agent, const char *old_agent, const char *new_agent) const {
	if (old_agent[0] != '\0') {
		static const char * items[] = {
			old_agent
		};
		static bool values[] = {
			false
		};
		indigo_change_switch_property(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, 1, items, values);
	}
	if (new_agent[0] != '\0') {
		static const char * items[] = {
			new_agent
		};
		static bool values[] = {
			true
		};
		indigo_change_switch_property(nullptr, agent, FILTER_RELATED_AGENT_LIST_PROPERTY_NAME, 1, items, values);
	}
}

void ImagerWindow::change_agent_imager_dithering_property(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_DITHERING_AGGRESSIVITY_ITEM_NAME,
		AGENT_IMAGER_DITHERING_TIME_LIMIT_ITEM_NAME
	};
	static double values[2];
	values[0] = (double)m_dither_aggr->value();
	values[1] = (double)m_dither_to->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_DITHERING_PROPERTY_NAME, 2, items, values);
}

void ImagerWindow::change_ccd_frame_type_property(const char *agent) const {
	static char selected_type[INDIGO_NAME_SIZE];
	strncpy(selected_type, m_frame_type_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	static const char * items[] = {
		selected_type
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, CCD_FRAME_TYPE_PROPERTY_NAME, 1, items, values);
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


void ImagerWindow::clear_guider_agent_star_selection(const char *agent) const {
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

	static double values[max_stars] = {0};

	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SELECTION_PROPERTY_NAME, count, (const char**)items, values);
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

void ImagerWindow::change_jpeg_settings_property(const char *agent, const int jpeg_quality, const double black_threshold, const double white_threshold) {
	static const char *items[] = {
		CCD_JPEG_SETTINGS_BLACK_ITEM_NAME,
		CCD_JPEG_SETTINGS_WHITE_ITEM_NAME,
		CCD_JPEG_SETTINGS_QUALITY_ITEM_NAME,
		CCD_JPEG_SETTINGS_BLACK_TRESHOLD_ITEM_NAME,
		CCD_JPEG_SETTINGS_WHITE_TRESHOLD_ITEM_NAME
	};
	static double values[5];
	values[0] = -1;
	values[1] = -1;
	values[2] = (double)jpeg_quality;
	values[3] = black_threshold;
	values[4] = white_threshold;
	indigo_change_number_property(nullptr, agent, CCD_JPEG_SETTINGS_PROPERTY_NAME, 5, items, values);
}

void ImagerWindow::change_detection_mode_property(const char *agent) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, m_detection_mode_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	static const char * items[] = {
		selected_mode
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_GUIDER_DETECTION_MODE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_dec_guiding_property(const char *agent) const {
	static char selected_mode[INDIGO_NAME_SIZE];
	strncpy(selected_mode, m_dec_guiding_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	static const char * items[] = {
		selected_mode
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_GUIDER_DEC_MODE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_focus_params_property(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_FOCUS_INITIAL_ITEM_NAME,
		AGENT_IMAGER_FOCUS_FINAL_ITEM_NAME,
		AGENT_IMAGER_FOCUS_BACKLASH_ITEM_NAME,
		AGENT_IMAGER_FOCUS_STACK_ITEM_NAME
	};
	static double values[4];
	values[0] = (double)m_initial_step->value();
	values[1] = (double)m_final_step->value();
	values[2] = (double)m_focus_backlash->value();
	values[3] = (double)m_focus_stack->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_FOCUS_PROPERTY_NAME, 4, items, values);
}

void ImagerWindow::change_agent_start_exposure_property(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_START_EXPOSURE_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_start_focusing_property(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_START_FOCUSING_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_start_preview_property(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_START_PREVIEW_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, 1, items, values);
}


void ImagerWindow::change_focuser_position_property(const char *agent) const {
	static const char *items[] = {
		FOCUSER_POSITION_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_focus_position->value();
	indigo_change_number_property(nullptr, agent, FOCUSER_POSITION_PROPERTY_NAME, 1, items, values);
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

void ImagerWindow::change_agent_pause_process_property(const char *agent) const {
	static const char *items[] = {
		AGENT_PAUSE_PROCESS_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME, 1, items, values);
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

void ImagerWindow::change_wheel_slot_property(const char *agent) const {
	static const char *items[] = {
		WHEEL_SLOT_ITEM_NAME
	};
	static double values[1];
	values[0] = (double)m_filter_select->currentIndex() + 1;
	indigo_change_number_property(nullptr, agent, WHEEL_SLOT_PROPERTY_NAME, 1, items, values);
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
	static const char *items[] = {
		AGENT_GUIDER_START_GUIDING_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::change_agent_start_calibrate_property(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_START_CALIBRATION_ITEM_NAME
	};
	static bool values[] = {
		true
	};
	indigo_change_switch_property(nullptr, agent, AGENT_START_PROCESS_PROPERTY_NAME, 1, items, values);
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

void ImagerWindow::change_guider_agent_pi(const char *agent) const {
	static const char *items[] = {
		AGENT_GUIDER_SETTINGS_PW_RA_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_PW_DEC_ITEM_NAME,
		AGENT_GUIDER_SETTINGS_STACK_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_guide_ra_pw->value();
	values[1] = (double)m_guide_dec_pw->value();
	values[2] = (double)m_guide_is->value();
	indigo_change_number_property(nullptr, agent, AGENT_GUIDER_SETTINGS_PROPERTY_NAME, 3, items, values);
}
