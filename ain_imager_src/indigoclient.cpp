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
#include <indigo/indigo_client.h>
#include "indigoclient.h"
#include "conf.h"
#include <chrono>

bool processed_device(char *device) {
	if (device == nullptr) return false;
	if (
		strncmp(device, "Imager Agent", 12) &&
		strncmp(device, "Guider Agent", 12) &&
		strncmp(device, "Mount Agent", 11) &&
		strncmp(device, "Astrometry Agent", 16) &&
		strncmp(device, "Configuration agent", 19) &&
		strncmp(device, "Configuration Agent", 19) &&
		strncmp(device, "Scripting Agent", 15) &&
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

bool IndigoClient::is_exposing(const char* device) {
	if (m_is_exposing.contains(device)) {
		return m_is_exposing.value(device);
	}
	return false;
}

void IndigoClient::remove_is_exposing_entry(const char* device) {
	m_is_exposing.remove(device);
}

void IndigoClient::update_save_blob(indigo_property *property)	 {
	if (client_match_device_prefix_property(property, "Imager Agent", AGENT_START_PROCESS_PROPERTY_NAME)) {
		bool is_exposing = false;
		if(property->state != INDIGO_BUSY_STATE) {
			is_exposing = false;
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
			is_exposing = sequence_running || batch_running;
		}
		m_is_exposing.insert(property->device, is_exposing);
	}
}


static bool download_blob_async(indigo_property *property, QAtomicInt *downloading, bool save_blob = false) {
	const int max_concurrent_downloads = 1;
	IndigoClient &client = IndigoClient::instance();

	int prev = downloading->fetchAndAddRelaxed(1);
	if (prev >= max_concurrent_downloads) {
		downloading->fetchAndAddRelaxed(-1);
		indigo_error("Maximum concurrent downloads (%d) reached, skipping...", max_concurrent_downloads);
		return false;
	}
	if (prev > 0 && prev < max_concurrent_downloads) {
		indigo_log("BLOB: %d Concurrent downloads (limit %d)", prev + 1, max_concurrent_downloads);
	}

	if (downloading == &client.imager_downloading) {
		emit(client.imager_download_started());
	}

	QtConcurrent::run([property, downloading, save_blob]() {
		IndigoClient &client = IndigoClient::instance();
		for (int row = 0; row < property->count; row++) {
			indigo_item *blob_item = (indigo_item*)malloc(sizeof(indigo_item));
			memcpy(blob_item, &property->items[row], sizeof(indigo_item));
			blob_item->blob.value = nullptr;

			auto t0 = std::chrono::high_resolution_clock::now();
			if (*property->items[row].blob.url && indigo_populate_http_blob_item(blob_item)) {
				auto t1 = std::chrono::high_resolution_clock::now();
				auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
				property->items[row].blob.value = nullptr;
				indigo_error("BLOB: %s.%s (%s, %ld bytes) downloaded in %ld ms", property->device, property->name, blob_item->blob.url, blob_item->blob.size, ms);
				emit(client.create_preview(property, blob_item, save_blob));
			} else {
				if (blob_item->blob.value) free(blob_item->blob.value);
				free(blob_item);
			}
		}

		int remaining = downloading->fetchAndAddRelaxed(-1) - 1; // fetchAndAddRelaxed returns previous value rhis is why -1

		if (remaining <= 0 && downloading == &client.imager_downloading) {
			emit(client.imager_download_completed());
		}

		if (remaining < 0) {
			// should not happen, but just in case
			downloading->store(0);
		}
	});

	return true;
}

static void handle_blob_property(indigo_property *property) {
	IndigoClient &client = IndigoClient::instance();
	static char error_message[] = "Error: Download is not fast enough, skipping frame.";
	if (property->state == INDIGO_OK_STATE && property->perm != INDIGO_WO_PERM) {
		indigo_error("######## Blob OK state");
		if (!strncmp(property->device, "Imager Agent", 12)) {
			if (!strncmp(property->name, CCD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE) && (conf.guider_save_bandwidth == 0)) {
				if (!download_blob_async(property, &client.imager_downloading, client.is_exposing(property->device))) {
					client.m_logger->log(property, error_message);
				}
			} else if (!strncmp(property->name, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) {
				if (!download_blob_async(property, &client.imager_downloading_saved_frame, true)) {
					client.m_logger->log(property, error_message);
				}
			} else if (!strncmp(property->name, CCD_PREVIEW_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE) && (conf.guider_save_bandwidth > 0)) {
				indigo_error("######## Preview image to save bandwidth");
				if (!download_blob_async(property, &client.imager_downloading, client.is_exposing(property->device))) {
					client.m_logger->log(property, error_message);
				}
			}
		} else if (!strncmp(property->device, "Guider Agent", 12)) {
			if ((!strncmp(property->name, CCD_PREVIEW_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) && (conf.guider_save_bandwidth == 0)) {
				return;
			}
			if ((!strncmp(property->name, CCD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) && (conf.guider_save_bandwidth > 0)) {
				return;
			}
			if (!download_blob_async(property, &client.guider_downloading)) {
				indigo_error(error_message);
			}
		} else {
			download_blob_async(property, &client.other_downloading, client.is_exposing(property->device));
		}
	} else if(property->state == INDIGO_BUSY_STATE && property->perm != INDIGO_WO_PERM) {
		for (int row = 0; row < property->count; row++) {
			emit(client.obsolete_preview(property, &property->items[row]));
		}
	} else {
		for (int row = 0; row < property->count; row++) {
			emit(client.no_preview(property, &property->items[row]));
		}
	}
}

static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(device);

	if (!processed_device(property->device)) return INDIGO_OK;

	IndigoClient &client_instance = IndigoClient::instance();

	client_instance.update_save_blob(property);

	if (property->type == INDIGO_BLOB_VECTOR) {
		//reset download flags
		if (!strncmp(property->device, "Imager Agent", 12)) {
			if (!strncmp(property->name, CCD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) {
				client_instance.imager_downloading.store(0);
			} else if (!strncmp(property->name, AGENT_IMAGER_DOWNLOAD_IMAGE_PROPERTY_NAME, INDIGO_NAME_SIZE)) {
				client_instance.imager_downloading_saved_frame.store(0);
			}
		} else if (!strncmp(property->device, "Guider Agent", 12)) {
			client_instance.guider_downloading.store(0);
		} else {
			client_instance.other_downloading.store(0);
		}
		if (device->version < INDIGO_VERSION_2_0)
			client_instance.m_logger->log(property, "BLOB can be used in INDI legacy mode");
		if (client_instance.blobs_enabled()) { // Enagle blob and let adapter decide URL or ALSO
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

	if (client_match_device_prefix_property(property, "Imager Agent", AGENT_START_PROCESS_PROPERTY_NAME)) {
		IndigoClient::instance().remove_is_exposing_entry(property->device);
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

void IndigoClient::start(const char *name) {
	indigo_start();
	strncpy(client.name, name, INDIGO_NAME_SIZE);
	indigo_attach_client(&client);
}

void IndigoClient::stop() {
	indigo_debug("Shutting down client...\n");
	indigo_detach_client(&client);
	indigo_stop();
}
