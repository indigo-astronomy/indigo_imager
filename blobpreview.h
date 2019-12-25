// Copyright (c) 2019 Rumen G.Bogdanovski
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

#ifndef _BLOBPREVIEW_H
#define _BLOBPREVIEW_H

#include <QImage>
#include <QHash>
#include <indigo/indigo_client.h>

#if !defined(INDIGO_WINDOWS)
#define USE_LIBJPEG
#endif
#if defined(USE_LIBJPEG)
#include <jpeglib.h>
#endif

typedef enum {
	STRETCH_NONE = 0,
	STRETCH_NORMAL = 1,
	STRETCH_HARD = 2,
} preview_stretch;

QImage* create_jpeg_preview(unsigned char *jpg_buffer, unsigned long jpg_size);
QImage* create_fits_preview(unsigned char *fits_buffer, unsigned long fits_size);
QImage* create_raw_preview(unsigned char *raw_image_buffer, unsigned long raw_size);
QImage* create_preview(int width, int height, int pixel_format, char *image_data, int *hist, double white_threshold);
QImage* create_preview(indigo_property *property, indigo_item *item);

class blob_preview_cache: QHash<QString, QImage*> {
public:
	blob_preview_cache(): preview_mutex(PTHREAD_MUTEX_INITIALIZER) {
	};

	~blob_preview_cache() {
		blob_preview_cache::iterator i;
		for (i = begin(); i != end(); ++i) {
			QImage *preview = i.value();
			if (preview != nullptr) delete(preview);
		}
	};

private:
	pthread_mutex_t preview_mutex;
	QString create_key(indigo_property *property, indigo_item *item);
	bool _remove(indigo_property *property, indigo_item *item);

public:
	void set_stretch_level(preview_stretch level);
	bool create(indigo_property *property, indigo_item *item);
	bool obsolete(indigo_property *property, indigo_item *item);
	QImage* get(indigo_property *property, indigo_item *item);
	bool remove(indigo_property *property, indigo_item *item);
};

extern blob_preview_cache preview_cache;

#endif /* _BLOBPREVIEW_H */
