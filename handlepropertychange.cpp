
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "imagerwindow.h"
#include "indigoclient.h"
#include "propertycache.h"

static void add_devices_to_combobox(indigo_property *property, QComboBox *devices_combobox) {
	int current_index = devices_combobox->currentIndex();
	for (int i = 0; i < property->count; i++) {
		QString item_name = QString(property->items[i].name);
		QString domain = QString(property->device);
		domain.remove(0, domain.indexOf(" @ "));
		QString device = QString(property->items[i].label) + domain;
		if (devices_combobox->findText(device) < 0) {
			devices_combobox->addItem(device, QString(property->device));
			indigo_debug("[ADD device] %s\n", device.toUtf8().data());
			if (property->items[i].sw.value && current_index < 0) {
				devices_combobox->setCurrentIndex(devices_combobox->findText(device));
			}
		} else {
			indigo_debug("[DUPLICATE device] %s\n", device.toUtf8().data());
		}
	}
}

static void change_devices_combobox_slection(indigo_property *property, QComboBox *devices_combobox) {
	for (int i = 0; i < property->count; i++) {
		//QString item_name = QString(property->items[i].name);
		if (property->items[i].sw.value) {
			QString domain = QString(property->device);
			domain.remove(0, domain.indexOf(" @ "));
			QString device = QString(property->items[i].label) + domain;
			devices_combobox->setCurrentIndex(devices_combobox->findText(device));
			indigo_debug("[ADD device] %s\n", device.toUtf8().data());
			break;
		}
	}
}

static void remove_devices_from_combobox(char *device_name, char *property_name, QComboBox *devices_combobox) {
	/*
	indigo_property *p = properties.get(device_name, property_name);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			QString device = QString(p->device);
			int index = devices_combobox->findData(device);
			if (index >= 0) {
				devices_combobox->removeItem(index);
				indigo_debug("[REMOVE device] %s at index\n", device.toUtf8().data(), index);
			} else {
				indigo_debug("[No device] %s\n", device.toUtf8().data());
			}
		}
	}
	*/
	int index;
	QString device = QString(device_name);
	do {
		index = devices_combobox->findData(device);
		if (index >= 0) {
			devices_combobox->removeItem(index);
			indigo_debug("[REMOVE device] %s at index\n", device.toUtf8().data(), index);
		}
	} while (index >= 0);
}

static void add_items_to_combobox(indigo_property *property, QComboBox *items_combobox) {
	items_combobox->clear();
	for (int i = 0; i < property->count; i++) {
		QString mode = QString(property->items[i].label);
		if (items_combobox->findText(mode) < 0) {
			items_combobox->addItem(mode, QString(property->items[i].name));
			indigo_debug("[ADD mode] %s\n", mode.toUtf8().data());
			if (property->items[i].sw.value) {
				items_combobox->setCurrentIndex(items_combobox->findText(mode));
			}
		} else {
			indigo_debug("[DUPLICATE mode] %s\n", mode.toUtf8().data());
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
			//if (property->items[i].sw.value) {
			//	m_filter_select->setCurrentIndex(m_filter_select->findText(filter_name));
			//}
		} else {
			indigo_debug("[DUPLICATE mode] %s\n", filter_name.toUtf8().data());
		}
	}
}

static void set_filter_selected(indigo_property *property, QComboBox *filter_select) {
	if (property->count == 1) {
		indigo_debug("SELECT: %s = %d\n", property->items[0].name, property->items[0].number.value);
		filter_select->setCurrentIndex((int)property->items[0].number.value-1);
	}
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

void ImagerWindow::on_property_define(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE];

	if (!get_selected_agent(selected_agent) || strncmp(property->device, "Imager Agent",12)) {
		return;
	}

	if (client_match_device_property(property, nullptr, FILTER_CCD_LIST_PROPERTY_NAME)) {
		add_devices_to_combobox(property, m_camera_select);
	}
	if (client_match_device_property(property, nullptr, FILTER_WHEEL_LIST_PROPERTY_NAME)) {
		add_devices_to_combobox(property, m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_size_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		add_items_to_combobox(property, m_frame_type_select);
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		indigo_debug("Set %s", property->name);
		for (int i = 0; i < property->count; i++) {
			indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
			if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME)) {
				m_exposure_time->setValue(property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_DELAY_ITEM_NAME)) {
				m_exposure_delay->setValue((int)property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME)) {
				m_frame_count->setValue((int)property->items[i].number.value);
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME)) {
		indigo_debug("Set %s", property->name);
		for (int i = 0; i < property->count; i++) {
			indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
			if (!strcmp(property->items[i].name, CCD_FRAME_LEFT_ITEM_NAME)) {
				m_roi_x->setValue((int)property->items[i].number.value);
				m_roi_x->setEnabled(true);
			} else if (!strcmp(property->items[i].name, CCD_FRAME_TOP_ITEM_NAME)) {
				m_roi_y->setValue((int)property->items[i].number.value);
				m_roi_y->setEnabled(true);
			} else if (!strcmp(property->items[i].name, CCD_FRAME_WIDTH_ITEM_NAME)) {
				m_roi_w->setValue((int)property->items[i].number.value);
				m_roi_w->setEnabled(true);
			} else if (!strcmp(property->items[i].name, CCD_FRAME_HEIGHT_ITEM_NAME)) {
				m_roi_h->setValue((int)property->items[i].number.value);
				m_roi_h->setEnabled(true);
			}
		}
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
		indigo_debug("Set %s", property->name);
		for (int i = 0; i < property->count; i++) {
			indigo_debug("Set %s = %f", property->items[i].name, property->items[i].sw.value);
			if (!strcmp(property->items[i].name, AGENT_PAUSE_PROCESS_ITEM_NAME)) {
				if(property->state == INDIGO_BUSY_STATE) {
					m_pause_button->setText("Resume");
				} else {
					m_pause_button->setText("Pause");
				}
			}
		}
	}
	properties.create(property);
}


void ImagerWindow::on_property_change(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE];
	if (!get_selected_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) {
		return;
	}

	if (client_match_device_property(property, nullptr, FILTER_CCD_LIST_PROPERTY_NAME)) {
		change_devices_combobox_slection(property, m_camera_select);
	}
	if (client_match_device_property(property, nullptr, FILTER_WHEEL_LIST_PROPERTY_NAME)) {
		change_devices_combobox_slection(property, m_wheel_select);
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value) {
				m_frame_size_select->setCurrentIndex(m_frame_size_select->findText(property->items[i].label));
				indigo_debug("[SELECT mode] %s\n", property->items[i].label);
				break;
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value) {
				m_frame_type_select->setCurrentIndex(m_frame_type_select->findText(property->items[i].label));
				indigo_debug("[SELECT mode] %s\n", property->items[i].label);
				break;
			}
		}
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_NAME_PROPERTY_NAME)) {
		reset_filter_names(property, m_filter_select);
		indigo_property *p = properties.get(property->device, WHEEL_SLOT_PROPERTY_NAME);
		if (p) set_filter_selected(p, m_filter_select);
	}
	if (client_match_device_property(property, selected_agent, WHEEL_SLOT_PROPERTY_NAME)) {
		if (property->count == 1) {
			indigo_property *p = properties.get(property->device, WHEEL_SLOT_NAME_PROPERTY_NAME);
			unsigned int current_filter = (unsigned int)property->items[0].number.value - 1;
			if (p && current_filter < p->count) {
				m_filter_select->setCurrentIndex(m_filter_select->findText(p->items[current_filter].text.value));
				indigo_debug("[SELECT mode] %s\n", p->items[current_filter].label);
			}
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME)) {
				m_exposure_time->setValue(property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_DELAY_ITEM_NAME)) {
				m_exposure_delay->setValue(property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME)) {
				m_frame_count->setValue((int)property->items[i].number.value);
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_EXPOSURE_PROPERTY_NAME) && m_preview) {
		double exp_elapsed, exp_time;
		if (property->state == INDIGO_BUSY_STATE) {
			exp_time = m_exposure_time->value();
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, CCD_EXPOSURE_ITEM_NAME)) {
					exp_elapsed = exp_time - property->items[i].number.value;
				}
			}
			m_exposure_progress->setMaximum(exp_time);
			m_exposure_progress->setValue(exp_elapsed);
			m_exposure_progress->setFormat("Preview: %v of %m seconds elapsed...");
			m_process_progress->setMaximum(1);
			m_process_progress->setValue(0);
			m_process_progress->setFormat("Preview in progress...");
		} else if(property->state == INDIGO_OK_STATE) {
			m_exposure_progress->setMaximum(100);
			m_exposure_progress->setValue(100);
			m_process_progress->setMaximum(100);
			m_process_progress->setValue(100);
			m_exposure_progress->setFormat("Preview: Complete");
			m_process_progress->setFormat("Process: Complete");

		} else {
			m_exposure_progress->setMaximum(1);
			m_exposure_progress->setValue(0);
			m_process_progress->setMaximum(1);
			m_process_progress->setValue(0);
			m_exposure_progress->setFormat("Exposure: Failed");
			m_process_progress->setFormat("Process: Failed");
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_STATS_PROPERTY_NAME)) {
		double exp_elapsed, exp_time;
		int frames_complete, frames_total;

		if (property->state == INDIGO_BUSY_STATE) {
			exp_time = m_exposure_time->value();
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME)) {
					exp_elapsed = exp_time - property->items[i].number.value;
				} else if (!strcmp(property->items[i].name, AGENT_IMAGER_STATS_FRAME_ITEM_NAME)) {
					frames_complete = (int)property->items[i].number.value;
				} else if (!strcmp(property->items[i].name, AGENT_IMAGER_STATS_FRAMES_ITEM_NAME)) {
					frames_total = (int)property->items[i].number.value;
				}
			}
			m_exposure_progress->setMaximum(exp_time);
			m_exposure_progress->setValue(exp_elapsed);
			m_exposure_progress->setFormat("Exposure: %v of %m seconds elapsed...");
			m_process_progress->setMaximum(frames_total);
			m_process_progress->setValue(frames_complete);
			m_process_progress->setFormat("Process: exposure %v of %m in progress...");
		} else if (property->state == INDIGO_OK_STATE) {
			m_exposure_progress->setMaximum(100);
			m_exposure_progress->setValue(100);
			m_exposure_progress->setFormat("Exposure: Complete");
			m_process_progress->setMaximum(100);
			m_process_progress->setValue(100);
			m_process_progress->setFormat("Process: Complete");
		} else {
			m_exposure_progress->setMaximum(1);
			m_exposure_progress->setValue(0);
			m_exposure_progress->setFormat("Exposure: Failed");
			m_process_progress->setValue(m_process_progress->value()-1);
			m_process_progress->setFormat("Process: %v exposures of %m competed");
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_PROPERTY_NAME)) {
		indigo_debug("Change %s", property->name);
		for (int i = 0; i < property->count; i++) {
			indigo_debug("Change %s = %f", property->items[i].name, property->items[i].number.value);
			if (!strcmp(property->items[i].name, CCD_FRAME_LEFT_ITEM_NAME)) {
				m_roi_x->setValue((int)property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, CCD_FRAME_TOP_ITEM_NAME)) {
				m_roi_y->setValue((int)property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, CCD_FRAME_WIDTH_ITEM_NAME)) {
				m_roi_w->setValue((int)property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, CCD_FRAME_HEIGHT_ITEM_NAME)) {
				m_roi_h->setValue((int)property->items[i].number.value);
			}

		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		indigo_debug("change %s", property->name);
		for (int i = 0; i < property->count; i++) {
			indigo_debug("change %s = %f", property->items[i].name, property->items[i].sw.value);
			if (!strcmp(property->items[i].name, AGENT_PAUSE_PROCESS_ITEM_NAME)) {
				if(property->state == INDIGO_BUSY_STATE) {
					m_pause_button->setText("Resume");
				} else {
					m_pause_button->setText("Pause");
				}
			}
		}
	}
	properties.create(property);
}


void ImagerWindow::on_property_delete(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE];
	indigo_debug("[REMOVE REMOVE REMOVE REMOVE REMOVE] %s.%s\n", property->device, property->name);
	if (!get_selected_agent(selected_agent) || strncmp(property->device, "Imager Agent", 12)) {
		free(property);
		return;
	}
	indigo_debug("[REMOVE REMOVE REMOVE REMOVE] %s.%s\n", property->device, property->name);

	if (client_match_device_property(property, nullptr, FILTER_CCD_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, property->device)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		remove_devices_from_combobox(property->device, FILTER_CCD_LIST_PROPERTY_NAME, m_camera_select);
	}
	if (client_match_device_property(property, nullptr, FILTER_WHEEL_LIST_PROPERTY_NAME) ||
	    client_match_device_no_property(property, property->device)) {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		remove_devices_from_combobox(property->device, FILTER_WHEEL_LIST_PROPERTY_NAME, m_wheel_select);
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
	properties.remove(property);
	free(property);
}
