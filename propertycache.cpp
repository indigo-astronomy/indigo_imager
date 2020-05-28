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

bool property_cache::_remove(indigo_property *property) {
	QString key = create_key(property);
	if (contains(key)) {
		indigo_property *p = value(key);
		indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), p);
		if (p != nullptr)
			indigo_release_property(p);
	} else {
		indigo_debug("property: %s(%s) - not cached\n", __FUNCTION__, key.toUtf8().constData());
	}
	return (bool)QHash::remove(key);
}

bool property_cache::create(indigo_property *property) {
	pthread_mutex_lock(&property_mutex);
	QString key = create_key(property);
	_remove(property);
	indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), property);
	if (property != nullptr) {
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


bool property_cache::remove(indigo_property *property) {
	pthread_mutex_lock(&property_mutex);
	bool success = _remove(property);
	pthread_mutex_unlock(&property_mutex);
	return success;
}
