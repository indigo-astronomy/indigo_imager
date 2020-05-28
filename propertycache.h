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

#ifndef _PROPERTYCACHE_H
#define _PROPERTYCACHE_H

#include <QHash>
#include <indigo/indigo_client.h>

class property_cache: QHash<QString, indigo_property*> {
public:
	property_cache(): property_mutex(PTHREAD_MUTEX_INITIALIZER) {
	};

	~property_cache() {
		property_cache::iterator i = begin();
		while (i != end()) {
			indigo_property *property = i.value();
			QString key = i.key();
			if (property != nullptr) indigo_release_property(property);
			indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), property);
			i = erase(i);
		}
	};

private:
	pthread_mutex_t property_mutex;
	QString create_key(indigo_property *property);
	bool _remove(indigo_property *property);

public:
	bool create(indigo_property *property);
	indigo_property* get(indigo_property *property);
	bool remove(indigo_property *property);
};

extern property_cache properties;

#endif /* _PROPERTYCACHE_H */
