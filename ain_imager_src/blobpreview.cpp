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

#include <math.h>
#include <fits.h>
#include <debayer.h>
#include <pixelformat.h>
#include <imagepreview.h>
#include <blobpreview.h>
#include <QPainter>

blob_preview_cache preview_cache;

QString blob_preview_cache::create_key(indigo_property *property, indigo_item *item) {
	QString key(property->device);
	key.append(".");
	key.append(property->name);
	key.append(".");
	key.append(item->name);
	return key;
}

bool blob_preview_cache::_remove(indigo_property *property, indigo_item *item) {
	QString key = create_key(property, item);
	if (contains(key)) {
		preview_image *preview = value(key);
		indigo_debug("preview: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), preview);
		if (preview != nullptr) {
			delete(preview);
		}
	} else {
		indigo_debug("preview: %s(%s) - no preview\n", __FUNCTION__, key.toUtf8().constData());
	}
	return (bool)QHash::remove(key);
}

bool blob_preview_cache::_remove(QString &key) {
	if (contains(key)) {
		preview_image *preview = value(key);
		indigo_debug("preview: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), preview);
		if (preview != nullptr) {
			delete(preview);
		}
	} else {
		indigo_debug("preview: %s(%s) - no preview\n", __FUNCTION__, key.toUtf8().constData());
	}
	return (bool)QHash::remove(key);
}


bool blob_preview_cache::obsolete(indigo_property *property, indigo_item *item) {
	QString key = create_key(property, item);
	if (contains(key)) {
		preview_image *preview = value(key);
		indigo_debug("preview: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), preview);
		if (preview != nullptr) {
			QPainter painter(preview);
			painter.setPen(QColor(241, 183, 1));
			QFont ft = painter.font();
			ft.setPixelSize(preview->height()/15);
			painter.setFont(ft);
			painter.drawText(preview->width()/20, preview->height()/20, preview->width(), preview->height(), Qt::AlignTop & Qt::AlignLeft, "\u231b Busy...");
			return true;
		}
	} else {
		indigo_debug("preview: %s(%s) - no preview\n", __FUNCTION__, key.toUtf8().constData());
	}
	return false;
}


bool blob_preview_cache::create(indigo_property *property, indigo_item *item, const stretch_config_t sconfig) {
	pthread_mutex_lock(&preview_mutex);
	QString key = create_key(property, item);
	_remove(property, item);
	preview_image *preview = create_preview(property, item, sconfig);
	//indigo_debug("preview: %s(%s) == %p, %.5f\n", __FUNCTION__, key.toUtf8().constData(), stretch->clip_white);
	if (preview != nullptr) {
		insert(key, preview);
		pthread_mutex_unlock(&preview_mutex);
		return true;
	}
	pthread_mutex_unlock(&preview_mutex);
	return false;
}

bool blob_preview_cache::recreate(QString &key, indigo_item *item, const stretch_config_t sconfig) {
	pthread_mutex_lock(&preview_mutex);
	preview_image *preview = _get(key);
	if (preview != nullptr) {
		//indigo_debug("recreate preview: %s(%s) == %p, %.5f\n", __FUNCTION__, key.toUtf8().constData(), stretch->clip_white);
		preview_image *new_preview = create_preview(item, sconfig);
		_remove(key);
		insert(key, new_preview);
		pthread_mutex_unlock(&preview_mutex);
		return true;
	}
	pthread_mutex_unlock(&preview_mutex);
	return false;
}

bool blob_preview_cache::recreate(QString &key, const stretch_config_t sconfig) {
	pthread_mutex_lock(&preview_mutex);
	preview_image *preview = _get(key);
	if (preview != nullptr) {
		//indigo_debug("recreate preview: %s(%s) == %p, %.5f\n", __FUNCTION__, key.toUtf8().constData(), stretch->clip_white);
		int width = preview->width();
		int height = preview->height();
		int pix_format = preview->m_pix_format;
		preview_image *new_preview = create_preview(width, height, pix_format, preview->m_raw_data, sconfig);
		_remove(key);
		insert(key, new_preview);
		pthread_mutex_unlock(&preview_mutex);
		return true;
	}
	pthread_mutex_unlock(&preview_mutex);
	return false;
}

bool blob_preview_cache::stretch(QString &key, const stretch_config_t sconfig) {
	pthread_mutex_lock(&preview_mutex);
	preview_image *preview = _get(key);
	if (preview != nullptr) {
		stretch_preview(preview, sconfig);
		pthread_mutex_unlock(&preview_mutex);
		return true;
	}
	pthread_mutex_unlock(&preview_mutex);
	return false;
}

preview_image* blob_preview_cache::get(indigo_property *property, indigo_item *item) {
	pthread_mutex_lock(&preview_mutex);
	QString key = create_key(property, item);
	if (contains(key)) {
		preview_image *preview = value(key);
		indigo_debug("preview: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), preview);
		pthread_mutex_unlock(&preview_mutex);
		return preview;
	}
	indigo_debug("preview: %s(%s) - no preview\n", __FUNCTION__, key.toUtf8().constData());
	pthread_mutex_unlock(&preview_mutex);
	return nullptr;
}

preview_image* blob_preview_cache::get(QString &key) {
	pthread_mutex_lock(&preview_mutex);
	if (contains(key)) {
		preview_image *preview = value(key);
		indigo_debug("preview: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), preview);
		pthread_mutex_unlock(&preview_mutex);
		return preview;
	}
	indigo_debug("preview: %s(%s) - no preview\n", __FUNCTION__, key.toUtf8().constData());
	pthread_mutex_unlock(&preview_mutex);
	return nullptr;
}

preview_image* blob_preview_cache::_get(QString &key) {
	if (contains(key)) {
		preview_image *preview = value(key);
		indigo_debug("preview: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), preview);
		return preview;
	}
	indigo_debug("preview: %s(%s) - no preview\n", __FUNCTION__, key.toUtf8().constData());
	return nullptr;
}


bool blob_preview_cache::remove(indigo_property *property, indigo_item *item) {
	pthread_mutex_lock(&preview_mutex);
	bool success = _remove(property, item);
	pthread_mutex_unlock(&preview_mutex);
	return success;
}
