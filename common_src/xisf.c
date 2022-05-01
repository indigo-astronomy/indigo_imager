// Copyright (c) 2022 Rumen G.Bogdanovski
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <xml.h>
#include <xisf.h>
#include <zlib.h>
#include <lz4.h>

static int xisf_metadata_init(xisf_metadata *metadata) {
	metadata->bitpix = 0;
	metadata->width = 0;
	metadata->height = 0;
	metadata->channels = 0;
	metadata->big_endian = false;            // default is little endian
	metadata->normal_pixel_storage = false;  // planar is default
	metadata->data_offset = 0;
	metadata->data_size = 0;
	metadata->uncompressed_data_size = 0;
	metadata->shuffle_size = 0;
	metadata->compression[0] = '\0';
	metadata->color_space[0] = '\0';
	metadata->bayer_pattern[0] = '\0';
	metadata->camera_name[0] = '\0';
	metadata->image_type[0] = '\0';
	metadata->observation_time[0] = '\0';
	metadata->exposure_time = -1;
	metadata->sensor_temperature = -1;
}

static void un_shuffle(uint8_t *output, const uint8_t *input, size_t size, size_t item_size) {
	if (size > 0) {
		if (item_size > 0) {
			if (input != NULL) {
				if (output != NULL) {
					size_t items = size / item_size;
					const uint8_t* s = input;
					for (size_t j = 0; j < item_size; ++j) {
						uint8_t* u = output + j;
						for (size_t i = 0; i < items; ++i, ++s, u += item_size) {
							*u = *s;
						}
					}
					memcpy(output + items * item_size, s, size % item_size);
				}
			}
		}
	}
}

int xisf_read_metadata(uint8_t *xisf_data, int xisf_size, xisf_metadata *metadata) {
	if (!xisf_data || !xisf_size || !metadata) {
		return XISF_INVALIDPARAM;
	}

	xisf_metadata_init(metadata);

	xisf_header *header = (xisf_header*)xisf_data;

	if (strncmp(header->signature,"XISF0100", 8)) {
		return XISF_NOT_XISF;
	}

	uint32_t xml_offset = 0;
	while (strncmp((char*)xisf_data + xml_offset, "<xisf", 5)) {
		xml_offset++;
		if (xml_offset > header->xml_length) {
			return XISF_NOT_XISF;
		}
	}

	struct xml_document* document = xml_parse_document(xisf_data + xml_offset, header->xml_length);

	if (!document) {
		return XISF_INVALIDDATA;
	}

	struct xml_node* xisf_root = xml_document_root(document);
	struct xml_node* node_image = NULL;

	int nodes = xml_node_children(xisf_root);
	for (int i = 0; i < nodes; i++) {
		struct xml_node* child = xml_node_child(xisf_root, i);
		struct xml_string* node_name_s = xml_node_name(child);
		char *node_name = calloc(xml_string_length(node_name_s) + 1, sizeof(uint8_t));
		xml_string_copy(node_name_s, node_name, xml_string_length(node_name_s));
		if (!strcmp(node_name, "Image")) {
			node_image = child;
			free(node_name);
			break;
		}
		free(node_name);
	}
	if (node_image == NULL) {
		xml_document_free(document, false);
		return XISF_INVALIDDATA;
	}

	int attr = xml_node_attributes(node_image);
	for (int i = 0; i < attr; i++) {
		struct xml_string* attr_name_s = xml_node_attribute_name(node_image, i);
		char* name = calloc(xml_string_length(attr_name_s) + 1, sizeof(uint8_t));
		xml_string_copy(attr_name_s, name, xml_string_length(attr_name_s));

		struct xml_string* attr_content_s = xml_node_attribute_content(node_image, i);
		char* content = calloc(xml_string_length(attr_content_s) + 1, sizeof(uint8_t));
		xml_string_copy(attr_content_s, content, xml_string_length(attr_content_s));

		indigo_error("XISF Image %s: %s\n", name, content);
		if (!strncmp(name, "geometry", strlen(name))) {
			int width = 0, height = 0, channels = 0;
			int scanned = sscanf(content, "%d:%d:%d", &width, &height, &channels);
			if (scanned != 3) {
				xml_document_free(document, false);
				return XISF_INVALIDDATA;
			}
			if (channels != 1 && channels != 3) {
				xml_document_free(document, false);
				return XISF_UNSUPPORTED;
			}
			metadata->width = width;
			metadata->height = height;
			metadata->channels = channels;
		} else if (!strncmp(name, "sampleFormat", strlen(name))) {
			if (!strncmp(content, "UInt8", strlen(content))) {
				metadata->bitpix = 8;
			} else if (!strncmp(content, "UInt16", strlen(content))) {
				metadata->bitpix = 16;
			} else if (!strncmp(content, "UInt32", strlen(content))) {
				metadata->bitpix = 32;
			} else if (!strncmp(content, "Float32", strlen(content))) {
				metadata->bitpix = -32;
			} else if (!strncmp(content, "Float64", strlen(content))) {
				metadata->bitpix = -64;
			}
		} else if (!strncmp(name, "pixelStorage", strlen(name))) {
			if (!strncmp(content, "Normal", strlen(content))) {
				metadata->normal_pixel_storage = true;
			} else if (!strncmp(content, "Planar", strlen(content))) {
				metadata->normal_pixel_storage = false;
			}
		} else if (!strncmp(name, "byteOrder", strlen(name))) {
			if (!strncmp(content, "big", strlen(content))) {
				metadata->big_endian = true;
			} else if (!strncmp(content, "little", strlen(content))) {
				metadata->big_endian = false;
			}
		} else if (!strncmp(name, "colorSpace", strlen(name))) {
			strncpy(metadata->color_space, content, sizeof(metadata->color_space));
		} else if (!strncmp(name, "imageType", strlen(name))) {
			strncpy(metadata->image_type, content, sizeof(metadata->image_type));
		} else if (!strncmp(name, "location", strlen(name))) {
			char location[100] = {0};
			int data_offset = 0;
			int data_size = 0;
			int scanned = sscanf(content, "%30[^:]:%d:%d", location, &data_offset, &data_size);
			if (scanned != 3) {
				xml_document_free(document, false);
				return XISF_INVALIDDATA;
			}
			if (strncmp(location, "attachment", strlen(location))) {
				xml_document_free(document, false);
				return XISF_UNSUPPORTED;
			}
			metadata->data_offset = data_offset;
			metadata->data_size = data_size;
		} else if (!strncmp(name, "compression", strlen(name))) {
			char compression[30] = {0};
			int data_size = 0;
			int shuffle_size = 0;
			int scanned = sscanf(content, "%30[^:]:%d:%d", compression, &data_size, &shuffle_size);
			if (scanned != 2 && scanned != 3) {
				xml_document_free(document, false);
				return XISF_INVALIDDATA;
			}
			strncpy(metadata->compression, compression, sizeof(metadata->compression));
			metadata->uncompressed_data_size = data_size;
			metadata->shuffle_size = shuffle_size;
		}
		free(name);
		free(content);

		nodes = xml_node_children(node_image);
		for (int j = 0; j < nodes; j++) {
			struct xml_node* child = xml_node_child(node_image, j);
			struct xml_string* node_name_s = xml_node_name(child);
			char *node_name = calloc(xml_string_length(node_name_s) + 1, sizeof(uint8_t));
			xml_string_copy(node_name_s, node_name, xml_string_length(node_name_s));

			if (!strcmp(node_name, "ColorFilterArray")) {
				int attr = xml_node_attributes(child);
				for (int i = 0; i < attr; i++) {
					struct xml_string* attr_name_s = xml_node_attribute_name(child, i);
					char* attr_name = calloc(xml_string_length(attr_name_s) + 1, sizeof(uint8_t));
					xml_string_copy(attr_name_s, attr_name, xml_string_length(attr_name_s));

					struct xml_string* attr_content_s = xml_node_attribute_content(child, i);
					char* attr_content = calloc(xml_string_length(attr_content_s) + 1, sizeof(uint8_t));
					xml_string_copy(attr_content_s, attr_content, xml_string_length(attr_content_s));

					if (!strcmp(attr_name, "pattern")) {
						strncpy(metadata->bayer_pattern, attr_content, sizeof(metadata->bayer_pattern));
					}

					free(attr_name);
					free(attr_content);
				}
			} else if (!strcmp(node_name, "Property")) {
				int attr = xml_node_attributes(child);
				char id[255];
				char value[255];
				for (int i = 0; i < attr; i++) {
					struct xml_string* attr_name_s = xml_node_attribute_name(child, i);
					char* attr_name = calloc(xml_string_length(attr_name_s) + 1, sizeof(uint8_t));
					xml_string_copy(attr_name_s, attr_name, xml_string_length(attr_name_s));

					struct xml_string* attr_content_s = xml_node_attribute_content(child, i);
					char* attr_content = calloc(xml_string_length(attr_content_s) + 1, sizeof(uint8_t));
					xml_string_copy(attr_content_s, attr_content, xml_string_length(attr_content_s));

					if (!strcmp(attr_name, "id")) {
						strncpy(id, attr_content, sizeof(id));
					}
					if (!strcmp(attr_name, "value")) {
						strncpy(value, attr_content, sizeof(value));
					}

					free(attr_name);
					free(attr_content);
				}

				struct xml_string* node_content_s = xml_node_content(child);
				char* node_content = calloc(xml_string_length(node_content_s) + 1, sizeof(uint8_t));
				xml_string_copy(node_content_s, node_content, xml_string_length(node_content_s));

				if (!strcmp(id, "Instrument:Camera:Name")) {
					strncpy(metadata->camera_name, node_content, sizeof(metadata->camera_name));
				} else if (!strcmp(id, "Instrument:ExposureTime")) {
					metadata->exposure_time = atof(value);
				} else if (!strcmp(id, "Instrument:Sensor:Temperature")) {
					metadata->sensor_temperature = atof(value);
				} else if (!strcmp(id, "Observation:Time:Start")) {
					strncpy(metadata->observation_time, value, sizeof(metadata->observation_time));
				} else if (!strcmp(id, "PCL:CFASourcePattern") && (metadata->bayer_pattern[0] == '\0') && !strcmp(metadata->color_space, "Gray")) {
					// Pixinsight does not follow its own specs. It writes PCL:CFASourcePattern. It writes it even with debayered images!!!
					strncpy(metadata->bayer_pattern, node_content, sizeof(metadata->bayer_pattern));
				}

				free(node_content);
			}
			free(node_name);
		}
	}
	xml_document_free(document, false);
	return XISF_OK;
}

int xisf_decompress(uint8_t *xisf_data, xisf_metadata *metadata, uint8_t *decompressed_data) {
	indigo_error("XISF decompress: %s %d %d", metadata->compression, metadata->uncompressed_data_size, metadata->shuffle_size);
	size_t uncompressed_data_size = metadata->uncompressed_data_size;
	if (!strcmp(metadata->compression, "zlib")) {
		int err = uncompress(decompressed_data, &uncompressed_data_size, xisf_data + metadata->data_offset, metadata->data_size);
		if (err == Z_OK) {
			return XISF_OK;
		} else {
			return XISF_INVALIDDATA;
		}
	} else if (!strcmp(metadata->compression, "zlib+sh")) {
		char *shuffled_data = (char*)malloc(uncompressed_data_size);
		int err = uncompress(shuffled_data, &uncompressed_data_size, xisf_data + metadata->data_offset, metadata->data_size);
		if (err != Z_OK) {
			free(shuffled_data);
			return XISF_INVALIDDATA;
		}
		un_shuffle(decompressed_data, shuffled_data, uncompressed_data_size, metadata->shuffle_size);
		free(shuffled_data);
		return XISF_OK;
	} else if (!strcmp(metadata->compression, "lz4") || !strcmp(metadata->compression, "lz4hc")) {
		int result = LZ4_decompress_safe(xisf_data + metadata->data_offset, decompressed_data, metadata->data_size, uncompressed_data_size);
		if (result > 0) {
			return XISF_OK;
		} else {
			return XISF_INVALIDDATA;
		}
	} else if (!strcmp(metadata->compression, "lz4+sh") || !strcmp(metadata->compression, "lz4hc+sh")) {
		char *shuffled_data = (char*)malloc(uncompressed_data_size);
		int result = LZ4_decompress_safe(xisf_data + metadata->data_offset, shuffled_data, metadata->data_size, uncompressed_data_size);
		if (result <= 0) {
			free(shuffled_data);
			return XISF_INVALIDDATA;
		}
		un_shuffle(decompressed_data, shuffled_data, uncompressed_data_size, metadata->shuffle_size);
		free(shuffled_data);
		return XISF_OK;
	}
	return XISF_UNSUPPORTED;
}
