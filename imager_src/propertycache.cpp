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

#include "propertycache.h"

property_cache properties;

QString property_cache::create_key(indigo_property *property) {
	QString key(property->device);
	key.append(".");
	key.append(property->name);
	return key;
}

QString property_cache::create_key(char *device_name, char *property_name) {
	QString key(device_name);
	key.append(".");
	key.append(property_name);
	return key;
}

bool property_cache::_remove(indigo_property *property) {
	bool ret = false;
	QString key = create_key(property);
	if (key.endsWith(".")) {
		property_cache::iterator i = begin();
		while (i != end()) {
			QString k = i.key();
			if (k.startsWith(key)) {
				indigo_property *p = i.value();
				indigo_debug("property: %s(%s -> %s) == %p\n", __FUNCTION__, key.toUtf8().constData(), k.toUtf8().constData(), p);
				i = erase(i);
				ret = true;
			} else {
				++i;
			}
		}
	} else {
		if (contains(key)) {
			indigo_property *p = value(key);
			indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), p);
		} else {
			indigo_debug("property: %s(%s) - not cached\n", __FUNCTION__, key.toUtf8().constData());
		}
		ret = (bool)QHash::remove(key);
	}
	return ret;
}

bool property_cache::create(indigo_property *property) {
	pthread_mutex_lock(&property_mutex);
	indigo_debug("property: %s() == %p\n", __FUNCTION__, property);
	if (property != nullptr) {
		QString key = create_key(property);
		indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), property);
		_remove(property);
		insert(key, property);
		pthread_mutex_unlock(&property_mutex);
		return true;
	}
	pthread_mutex_unlock(&property_mutex);
	return false;
}


indigo_property* property_cache::get(indigo_property *property) {
	pthread_mutex_lock(&property_mutex);
	QString key = create_key(property);
	if (contains(key)) {
		indigo_property *p = value(key);
		indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), p);
		pthread_mutex_unlock(&property_mutex);
		return p;
	}
	indigo_debug("property: %s(%s) - no cache\n", __FUNCTION__, key.toUtf8().constData());
	pthread_mutex_unlock(&property_mutex);
	return nullptr;
}


indigo_property* property_cache::get(char *device_name, char *property_name) {
	pthread_mutex_lock(&property_mutex);
	QString key = create_key(device_name, property_name);
	if (contains(key)) {
		indigo_property *p = value(key);
		indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), p);
		pthread_mutex_unlock(&property_mutex);
		return p;
	}
	indigo_debug("property: %s(%s) - no cache\n", __FUNCTION__, key.toUtf8().constData());
	pthread_mutex_unlock(&property_mutex);
	return nullptr;
}

indigo_item* property_cache::get_item(char *device_name, char *property_name, char *item_name) {
	pthread_mutex_lock(&property_mutex);
	QString key = create_key(device_name, property_name);
	if (contains(key)) {
		indigo_property *p = value(key);
		indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), p);
		pthread_mutex_unlock(&property_mutex);
		for (int i = 0; i< p->count; i++) {
			if (!strcmp(p->items[i].name, item_name)) {
				return &(p->items[i]);
			}
		}
	}
	indigo_debug("property: %s(%s) - no cache\n", __FUNCTION__, key.toUtf8().constData());
	pthread_mutex_unlock(&property_mutex);
	return nullptr;
}

indigo_item* property_cache::get_item(indigo_property *property, char *item_name) {
	pthread_mutex_lock(&property_mutex);
	pthread_mutex_unlock(&property_mutex);
	for (int i = 0; i< property->count; i++) {
		if (!strcmp(property->items[i].name, item_name)) {
			return &(property->items[i]);
		}
	}
	indigo_debug("property: %s - no item %s\n", __FUNCTION__, item_name);
	pthread_mutex_unlock(&property_mutex);
	return nullptr;
}


bool property_cache::remove(indigo_property *property) {
	pthread_mutex_lock(&property_mutex);
	bool success = _remove(property);
	pthread_mutex_unlock(&property_mutex);
	return success;
}
