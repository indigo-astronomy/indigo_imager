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


#include <QtConcurrent/QtConcurrent>
#include <QAtomicInt>
#include <indigo/indigo_client.h>
#include "indigoclient.h"
#include "conf.h"

bool processed_device(char *device) {
	if (device == nullptr) return false;
	if (
		strncmp(device, "Imager Agent", 12) &&
		strncmp(device, "Guider Agent", 12) &&
		strncmp(device, "Mount Agent", 11) &&
		strncmp(device, "Astrometry Agent", 16) &&
		strncmp(device, "Configuration agent", 19) &&
		strncmp(device, "Configuration Agent", 19) &&
		strncmp(device, "Server", 6)
	) {
		return false;
	} else {
		return true;
	}
}

bool client_match_item(indigo_item *item, const char *item_name) {
	if (item == nullptr || item_name == nullptr) return false;

	return (bool)(!strncmp(item->name, item_name, INDIGO_NAME_SIZE));
}

bool client_match_device_property(indigo_property *property, const char *device_name, const char *property_name) {
	if (property_name == nullptr && device_name == nullptr) return false;

	if (property_name == nullptr) {
		return (!strncmp(property->device, device_name, INDIGO_NAME_SIZE));
	}

	if (device_name == nullptr) {
		return (!strncmp(property->name, property_name, INDIGO_NAME_SIZE));
	}

	return (!strncmp(property->name, property_name, INDIGO_NAME_SIZE) && !strncmp(property->device, device_name, INDIGO_NAME_SIZE));
}

bool client_match_device_prefix_property(indigo_property *property, const char *device_name_prefix, const char *property_name) {
	if (property_name == nullptr && device_name_prefix == nullptr) return false;

	if (property_name == nullptr) {
		return (!strncmp(property->device, device_name_prefix, strlen(device_name_prefix)));
	}

	if (device_name_prefix == nullptr) {
		return (!strncmp(property->name, property_name, INDIGO_NAME_SIZE));
	}

	return (!strncmp(property->name, property_name, INDIGO_NAME_SIZE) && !strncmp(property->device, device_name_prefix, strlen(device_name_prefix)));
}


bool client_match_device_no_property(indigo_property *property, const char *device_name) {
	if (device_name == nullptr) return false;

	return (property->name[0] == '\0' && !strncmp(property->device, device_name, INDIGO_NAME_SIZE));
}


static indigo_result client_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

void IndigoClient::update_save_blob(indigo_property *property)	 {
	if (client_match_device_prefix_property(property, "Imager Agent", AGENT_START_PROCESS_PROPERTY_NAME)) {
		if(property->state != INDIGO_BUSY_STATE) {
			m_is_exposing = false;
		} else {
			bool sequence_running = false;
			bool batch_running = false;
			for (int i = 0; i < property->count; i++) {
				if (client_match_item(&property->items[i], AGENT_IMAGER_START_SEQUENCE_ITEM_NAME)) {
					sequence_running = property->items[i].sw.value;
				} else if(client_match_item(&property->items[i], AGENT_IMAGER_START_EXPOSURE_ITEM_NAME)) {
					batch_running = property->items[i].sw.value;
				}
			}
			m_is_exposing = sequence_running || batch_running;
		}
	} else if (client_match_device_prefix_property(property, "Imager Agent", AGENT_PAUSE_PROCESS_PROPERTY_NAME)) {
		bool pause_sw = false;
		bool pause_wait_sw = false;
		for (int i = 0; i < property->count; i++) {
			if (client_match_item(&property->items[i], AGENT_PAUSE_PROCESS_WAIT_ITEM_NAME)) {
				pause_wait_sw = property->items[i].sw.value;
			} else if (client_match_item(&property->items[i], AGENT_PAUSE_PROCESS_ITEM_NAME)) {
				pause_sw = property->items[i].sw.value;
			}
		}
		m_is_paused = (pause_wait_sw || pause_sw) ? true : false;
	}
	m_save_blob = m_is_exposing && !m_is_paused;
}

static QAtomicInt guider_downloading(0);
static QAtomicInt imager_downloading(0);
static QAtomicInt imager_downloading_saved_frame(0);
static QAtomicInt else_downloading(0);

static bool download_blob_async(indigo_property *property, QAtomicInt *downloading, bool save_blob = false) {
	if (!downloading->testAndSetAcquire(0, 1)) {
		indigo_debug("Task is already running, skipping...");
		return false;
	}

	QtConcurrent::run([property, downloading, save_blob]() {
		for (int row = 0; row < property->count; row++) {
			indigo_item *blob_item = (indigo_item*)malloc(sizeof(indigo_item));
			memcpy(blob_item, &property->items[row], sizeof(indigo_item));
			blob_item->blob.value = nullptr;
			if (*property->items[row].blob.url && indigo_populate_http_blob_item(blob_item)) {
				property->items[row].blob.value = nullptr;
				indigo_error("BLOB: %s.%s URL received (%s, %ld bytes)...\n", property->device, property->name, blob_item->blob.url, blob_item->blob.size);
				emit(IndigoClient::instance().create_preview(property, blob_item, save_blob));
			} else {
				if (blob_item->blob.value) free(blob_item->blob.value);
				free(blob_item);
			}
		}
		downloading->store(0);
	});

	return true;
}

static void handle_blob_property(indigo_property *property) {
	static char error_message[] = "Error: Download is not fast enough, skipping frame.";
	if (property->state == INDIGO_OK_STATE && property->perm != INDIGO_WO_PERM) {
		if (!strncmp(property->device, "Imager Agent", 12)) {
			if (!strncmp(property->name, CCD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) {
				if (!download_blob_async(property, &imager_downloading, IndigoClient::instance().m_save_blob)) {
					IndigoClient::instance().m_logger->log(property, error_message);
				}
			} else if (!strncmp(property->name, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) {
				if (!download_blob_async(property, &imager_downloading_saved_frame, true)) {
					IndigoClient::instance().m_logger->log(property, error_message);
				}
			} else if (!strncmp(property->name, CCD_PREVIEW_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) {
				// for the time being we are not interested in preview images
			}
		} else if (!strncmp(property->device, "Guider Agent", 12)) {
			if ((!strncmp(property->name, CCD_PREVIEW_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) && (conf.guider_save_bandwidth == 0)) {
				return;
			}
			if ((!strncmp(property->name, CCD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) && (conf.guider_save_bandwidth > 0)) {
				return;
			}
			if (!download_blob_async(property, &guider_downloading)) {
				IndigoClient::instance().m_logger->log(property, error_message);
			}
		} else {
			download_blob_async(property, &else_downloading, IndigoClient::instance().m_save_blob);
		}
	} else if(property->state == INDIGO_BUSY_STATE && property->perm != INDIGO_WO_PERM) {
		for (int row = 0; row < property->count; row++) {
			emit(IndigoClient::instance().obsolete_preview(property, &property->items[row]));
		}
	} else {
		for (int row = 0; row < property->count; row++) {
			emit(IndigoClient::instance().no_preview(property, &property->items[row]));
		}
	}
}

static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(device);

	if (!processed_device(property->device)) return INDIGO_OK;

	IndigoClient::instance().update_save_blob(property);

	if (property->type == INDIGO_BLOB_VECTOR) {
		if (device->version < INDIGO_VERSION_2_0)
			IndigoClient::instance().m_logger->log(property, "BLOB can be used in INDI legacy mode");
		if (IndigoClient::instance().blobs_enabled()) { // Enagle blob and let adapter decide URL or ALSO
				indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB);
		}
		handle_blob_property(property);
	}

	if (message) {
		static char *message_copy;
		message_copy = (char*)malloc(INDIGO_VALUE_SIZE);
		strncpy(message_copy, message, INDIGO_VALUE_SIZE);
		emit(IndigoClient::instance().property_defined(property, message_copy));
	} else {
		emit(IndigoClient::instance().property_defined(property, NULL));
	}
	return INDIGO_OK;
}


static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(client);
	Q_UNUSED(device);

	if (!processed_device(property->device)) return INDIGO_OK;

	IndigoClient::instance().update_save_blob(property);

	if (property->type == INDIGO_BLOB_VECTOR) {
		handle_blob_property(property);
	}

	if (message) {
		static char *message_copy;
		message_copy = (char*)malloc(INDIGO_VALUE_SIZE);
		strncpy(message_copy, message, INDIGO_VALUE_SIZE);
		emit(IndigoClient::instance().property_changed(property, message_copy));
	} else {
		emit(IndigoClient::instance().property_changed(property, NULL));
	}
	return INDIGO_OK;
}


static indigo_result client_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(client);
	Q_UNUSED(device);
	indigo_debug("Deleting property [%s] on device [%s]\n", property->name, property->device);

	if (!processed_device(property->device)) return INDIGO_OK;

	if (property->type == INDIGO_BLOB_VECTOR) {
		for (int row = 0; row < property->count; row++) {
			emit(IndigoClient::instance().remove_preview(property, &property->items[row]));
		}
	}

	indigo_property *property_copy = (indigo_property*)malloc(sizeof(indigo_property));
	memcpy(property_copy, property, sizeof(indigo_property));
	property_copy->count = 0;

	if (message) {
		static char *msg;
		msg = (char*)malloc(INDIGO_VALUE_SIZE);
		strncpy(msg, message, INDIGO_VALUE_SIZE);
		emit(IndigoClient::instance().property_deleted(property_copy, msg));
	} else {
		emit(IndigoClient::instance().property_deleted(property_copy, NULL));
	}
	return INDIGO_OK;
}


static indigo_result client_send_message(indigo_client *client, indigo_device *device, const char *message) {
	Q_UNUSED(client);

	if (!message) return INDIGO_OK;

	char *message_copy;
	message_copy = (char*)malloc(INDIGO_VALUE_SIZE);
	if ((device) && (device->name[0]) && (device->name[0] != '@')) {
		// We have device name
		snprintf(message_copy, INDIGO_VALUE_SIZE, "%s: %s", device->name, message);
	} else {
		snprintf(message_copy, INDIGO_VALUE_SIZE, "%s", message);
	}
	emit(IndigoClient::instance().message_sent(NULL, message_copy));
	return INDIGO_OK;
}


static indigo_result client_detach(indigo_client *client) {
	Q_UNUSED(client);

	return INDIGO_OK;
}


indigo_client client = {
	"INDIGO Client", false, NULL, INDIGO_OK, INDIGO_VERSION_CURRENT, NULL,
	client_attach,
	client_define_property,
	client_update_property,
	client_delete_property,
	client_send_message,
	client_detach
};

void IndigoClient::start(char *name) {
	indigo_start();
	strncpy(client.name, name, INDIGO_NAME_SIZE);
	indigo_attach_client(&client);
}

void IndigoClient::stop() {
	indigo_debug("Shutting down client...\n");
	indigo_detach_client(&client);
	indigo_stop();
}
