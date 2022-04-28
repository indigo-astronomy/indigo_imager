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

int xisf_read_metadata(uint8_t *xisf_data, int xisf_size, xisf_metadata *metadata) {
	if (!xisf_data || !xisf_size || !metadata) {
		return XISF_INVALIDPARAM;
	}

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
		struct xml_string* node_name = xml_node_name(child);
		char *name = calloc(xml_string_length(node_name) + 1, sizeof(uint8_t));
		xml_string_copy(node_name, name, xml_string_length(node_name));
		if (!strncmp(name, "Image", strlen(name))) {
			node_image = child;
			free(name);
			break;
		}
		free(name);
	}
	if (node_image == NULL) {
		xml_document_free(document, false);
		return XISF_INVALIDDATA;
	}

	int attr = xml_node_attributes(node_image);
	for (int i = 0; i < attr; i++) {
		struct xml_string* attr_name = xml_node_attribute_name(node_image, i);
		char* name = calloc(xml_string_length(attr_name) + 1, sizeof(uint8_t));
		xml_string_copy(attr_name, name, xml_string_length(attr_name));

		struct xml_string* attr_content = xml_node_attribute_content(node_image, i);
		char* content = calloc(xml_string_length(attr_content) + 1, sizeof(uint8_t));
		xml_string_copy(attr_content, content, xml_string_length(attr_content));

		indigo_debug("XISF %s: %s\n", name, content);
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
			metadata->naxisn[0] = width;
			metadata->naxisn[1] = height;
			if (channels == 1) {
				metadata->naxis = 2;
				metadata->naxisn[2] = 0;
			} else {
				metadata->naxis = 3;
				metadata->naxisn[2] = 3;
			}
		} else if (!strncmp(name, "sampleFormat", strlen(name))) {
			if (!strncmp(name, "UInt8", strlen(name))) {
				metadata->bitpix = 8;
			} else if (!strncmp(content, "UInt16", strlen(name))) {
				metadata->bitpix = 16;
			} else if (!strncmp(content, "UInt32", strlen(name))) {
				metadata->bitpix = 32;
			} else if (!strncmp(content, "Float32", strlen(name))) {
				metadata->bitpix = -32;
			} else if (!strncmp(content, "Float64", strlen(name))) {
				metadata->bitpix = -64;
			}
		} else if (!strncmp(name, "colorSpace", strlen(name))) {
			strncpy(metadata->colourspace, content, sizeof(metadata->colourspace));
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
			xml_document_free(document, false);
			indigo_error("Unsupported XISF compression: %s", content);
			return XISF_UNSUPPORTED;
		}
		free(name);
		free(content);
	}
	xml_document_free(document, false);
	return XISF_OK;
}
