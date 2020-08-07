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


#include <indigo/indigo_client.h>
#include "indigoclient.h"


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


bool client_match_device_no_property(indigo_property *property, const char *device_name) {
	if (device_name == nullptr) return false;

	return (property->name[0] == '\0' && !strncmp(property->device, device_name, INDIGO_NAME_SIZE));
}


static indigo_result client_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}


static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(device);
	if(strncmp(property->device, "Imager Agent", 12) && strncmp(property->device, "Server", 6)) return INDIGO_OK;

	if (property->type == INDIGO_BLOB_VECTOR) {
		if (device->version < INDIGO_VERSION_2_0)
			IndigoClient::instance().m_logger->log(property, "BLOB can be used in INDI legacy mode");
		if (IndigoClient::instance().blobs_enabled()) { // Enagle blob and let adapter decide URL or ALSO
				indigo_enable_blob(client, property, INDIGO_ENABLE_BLOB);
		}
		if (property->state == INDIGO_OK_STATE) {
			for (int row = 0; row < property->count; row++) {
				// cache item to pass it with create_preview() signal
				indigo_item *blob_item = (indigo_item*)malloc(sizeof(indigo_item));
				memcpy(blob_item, &property->items[row], sizeof(indigo_item));
				blob_item->blob.value = nullptr;
				if (*property->items[row].blob.url && indigo_populate_http_blob_item(blob_item)) {
					property->items[row].blob.value = nullptr;
					emit(IndigoClient::instance().create_preview(property, blob_item));
				} else {
					if (blob_item->blob.value) free(blob_item->blob.value);
					free(blob_item);
				}
			}
		} else if(property->state == INDIGO_BUSY_STATE) {
			for (int row = 0; row < property->count; row++) {
				emit(IndigoClient::instance().obsolete_preview(property, &property->items[row]));
			}
		} else {
			for (int row = 0; row < property->count; row++) {
				emit(IndigoClient::instance().remove_preview(property, &property->items[row]));
			}
		}
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
	if(strncmp(property->device, "Imager Agent", 12) && strncmp(property->device, "Server", 6)) return INDIGO_OK;

	if (property->type == INDIGO_BLOB_VECTOR) {
		if (property->state == INDIGO_OK_STATE) {
			for (int row = 0; row < property->count; row++) {
				// cache item to pass it with create_preview() signal
				indigo_item *blob_item = (indigo_item*)malloc(sizeof(indigo_item));
				memcpy(blob_item, &property->items[row], sizeof(indigo_item));
				blob_item->blob.value = nullptr;
				if (*property->items[row].blob.url && indigo_populate_http_blob_item(blob_item)) {
					property->items[row].blob.value = nullptr;
					indigo_log("Image %s.%s URL received (%s, %ld bytes)...\n", property->device, property->name, blob_item->blob.url, blob_item->blob.size);
					emit(IndigoClient::instance().create_preview(property, blob_item));
				} else {
					if (blob_item->blob.value) free(blob_item->blob.value);
					free(blob_item);
				}
			}
		} else if(property->state == INDIGO_BUSY_STATE) {
			for (int row = 0; row < property->count; row++) {
				emit(IndigoClient::instance().obsolete_preview(property, &property->items[row]));
			}
		} else {
			for (int row = 0; row < property->count; row++) {
				emit(IndigoClient::instance().remove_preview(property, &property->items[row]));
			}
		}
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
	if(strncmp(property->device, "Imager Agent", 12) && strncmp(property->device, "Server", 6)) return INDIGO_OK;

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
