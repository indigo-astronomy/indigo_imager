#include "imagerwindow.h"

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

void ImagerWindow::change_ccd_exposure_property(const char *agent) const {
	static const char *items[] = {
		CCD_EXPOSURE_ITEM_NAME,
	};
	static double values[1];
	values[0] = (double)m_exposure_time->value();
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

void ImagerWindow::change_agent_batch_property_for_focusing(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME,
		AGENT_IMAGER_BATCH_DELAY_ITEM_NAME,
		AGENT_IMAGER_BATCH_COUNT_ITEM_NAME
	};
	static double values[3];
	values[0] = (double)m_focuser_exposure_time->value();
	values[1] = 0;
	values[2] = -1;
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_BATCH_PROPERTY_NAME, 3, items, values);
}

void ImagerWindow::change_agent_star_selection(const char *agent) const {
	static const char *items[] = {
		AGENT_IMAGER_SELECTED_STAR_X_ITEM_NAME,
		AGENT_IMAGER_SELECTED_STAR_Y_ITEM_NAME
	};
	static double values[2];
	values[0] = (double)m_star_x->value();
	values[1] = (double)m_star_y->value();
	indigo_change_number_property(nullptr, agent, AGENT_IMAGER_SELECTED_STAR_PROPERTY_NAME, 2, items, values);
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
