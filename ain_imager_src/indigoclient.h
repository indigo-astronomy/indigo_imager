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


#ifndef INDIGOCLIENT_H
#define INDIGOCLIENT_H

#include <QObject>
#include <QAtomicInt>
#include <indigo/indigo_bus.h>
#include "logger.h"

extern bool client_match_device_property(indigo_property *property, const char *device_name, const char *property_name);
extern bool client_match_device_no_property(indigo_property *property, const char *device_name);
extern bool client_match_item(indigo_item *item, const char *item_name);

class IndigoClient : public QObject
{
	Q_OBJECT
public:
	static IndigoClient& instance();
	bool m_blobs_enabled;

public:
	IndigoClient() : guider_downloading(0), imager_downloading(0), imager_downloading_saved_frame(0), other_downloading(0) {
		m_logger = &Logger::instance();
		m_blobs_enabled = false;
		m_save_blob = false;
		m_is_exposing = true;
		m_is_paused = false;
	}

	~IndigoClient() {
		stop();
	}

	void enable_blobs(bool enable) {
		m_blobs_enabled = enable;
	};

	bool blobs_enabled() {
		return m_blobs_enabled;
	};

	void start(char *name);
	void stop();
	void update_save_blob(indigo_property *property);
	bool m_save_blob;
	bool m_is_exposing;
	bool m_is_paused;

	QAtomicInt guider_downloading;
	QAtomicInt imager_downloading;
	QAtomicInt imager_downloading_saved_frame;
	QAtomicInt other_downloading;

	Logger* m_logger;

signals:
	/* No copy of the property will be made with this signals.
	   Do not free().
	   Only parameters ending with _copy must be freed!
	*/
	void property_defined(indigo_property* property, char *message_copy);
	void property_changed(indigo_property* property, char *message_copy);
	void property_deleted(indigo_property* property_copy, char *message_copy);

	/* property is always NULL */
	void message_sent(indigo_property* property, char *message_copy);

	void create_preview(indigo_property* property, indigo_item *blob_item_copy, bool save_blob);
	void obsolete_preview(indigo_property* property, indigo_item *item);
	void remove_preview(indigo_property* property, indigo_item *item);
	void no_preview(indigo_property* property, indigo_item *item);
};

inline IndigoClient& IndigoClient::instance() {
	static IndigoClient* me = nullptr;
	if (!me) me = new IndigoClient();
	return *me;
}

#endif // INDIGOCLIENT_H
