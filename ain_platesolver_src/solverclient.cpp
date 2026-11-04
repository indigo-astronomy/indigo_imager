// Copyright (c) 2026 Rumen G.Bogdanovski
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


#include <QElapsedTimer>
#include <QThread>
#include <indigo/indigo_names.h>
#include <solverclient.h>

#define STOP_TIMEOUT_MS 10000

static QString agent_load_error;

static void log_message_handler(indigo_log_levels level, const char *message) {
	// keep the reason of a failed load for the user, indigo_load_driver() returns only the failure
	if (strstr(message, PLATESOLVER_DRIVER_NAME) != nullptr) {
		agent_load_error = message;
	}
	if (level <= indigo_get_log_level()) {
		fprintf(stderr, "%s\n", message);
	}
}

static bool is_solver_device(const char *device) {
	return !strncmp(device, PLATESOLVER_DEVICE_NAME, strlen(PLATESOLVER_DEVICE_NAME));
}

static indigo_result client_attach(indigo_client *client) {
	indigo_enumerate_properties(client, &INDIGO_ALL_PROPERTIES);
	return INDIGO_OK;
}

static indigo_result client_define_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(device);
	((SolverClient *)client->client_context)->handle_property(property, message, true);
	return INDIGO_OK;
}

static indigo_result client_update_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(device);
	((SolverClient *)client->client_context)->handle_property(property, message, false);
	return INDIGO_OK;
}

static indigo_result client_delete_property(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(device);
	((SolverClient *)client->client_context)->handle_delete(property, message);
	return INDIGO_OK;
}

static indigo_result client_send_message(indigo_client *client, indigo_device *device, indigo_property *property, const char *message) {
	Q_UNUSED(property);
	((SolverClient *)client->client_context)->handle_message(device ? device->name : "", message);
	return INDIGO_OK;
}

static indigo_result client_detach(indigo_client *client) {
	Q_UNUSED(client);
	return INDIGO_OK;
}

SolverClient::SolverClient(QObject *parent) : QObject(parent), m_driver(nullptr), m_started(false) {
	qRegisterMetaType<SolverProperty>("SolverProperty");
	static indigo_client client_template = {
		"Ain Platesolver", false, nullptr, INDIGO_OK, INDIGO_VERSION_CURRENT, nullptr,
		client_attach,
		client_define_property,
		client_update_property,
		client_delete_property,
		client_send_message,
		client_detach
	};
	m_client = &client_template;
	m_client->client_context = this;
}

SolverClient::~SolverClient() {
	stop();
}

bool SolverClient::load_agent() {
	indigo_start();
	m_started = true;
	indigo_attach_client(m_client);
	indigo_log_message_handler = log_message_handler;
	indigo_result result = indigo_load_driver(PLATESOLVER_DRIVER_NAME, true, &m_driver);
	indigo_log_message_handler = nullptr;
	if (result != INDIGO_OK) {
		m_driver = nullptr;
		m_last_error = agent_load_error.isEmpty() ? QString("Driver %1 failed to load").arg(PLATESOLVER_DRIVER_NAME) : agent_load_error;
		return false;
	}
	return true;
}

bool SolverClient::agent_busy() const {
	return m_wcs_busy.loadAcquire() || m_index_busy.loadAcquire();
}

void SolverClient::stop() {
	if (!m_started) return;
	m_started = false;
	if (m_driver != nullptr && agent_busy()) {
		abort();
		QElapsedTimer timer;
		timer.start();
		while (agent_busy() && timer.elapsed() < STOP_TIMEOUT_MS) {
			QThread::msleep(20);
		}
	}
	// the state of the agent comes through the client, read it before detaching
	bool busy = agent_busy();
	indigo_detach_client(m_client);
	if (busy) {
		// never detach an agent with a running solve, the exit of the process ends it
		indigo_error("%s does not stop, leaving it to the exit", PLATESOLVER_DEVICE_NAME);
		return;
	}
	if (m_driver != nullptr) {
		indigo_remove_driver(m_driver);
		m_driver = nullptr;
	}
	indigo_stop();
}

void SolverClient::set_switches(const QString &property, const QVector<SolverItem> &items) {
	QVector<QByteArray> names;
	QVector<const char *> name_ptrs;
	QVector<bool> values;
	for (const SolverItem &item : items) {
		names.append(item.name.toUtf8());
		values.append(item.on);
	}
	for (const QByteArray &name : names) {
		name_ptrs.append(name.constData());
	}
	indigo_change_switch_property(m_client, PLATESOLVER_DEVICE_NAME, property.toUtf8().constData(), items.size(), (const char **)name_ptrs.constData(), values.constData());
}

void SolverClient::set_numbers(const QString &property, const QVector<SolverItem> &items) {
	QVector<QByteArray> names;
	QVector<const char *> name_ptrs;
	QVector<double> values;
	for (const SolverItem &item : items) {
		names.append(item.name.toUtf8());
		values.append(item.number);
	}
	for (const QByteArray &name : names) {
		name_ptrs.append(name.constData());
	}
	indigo_change_number_property(m_client, PLATESOLVER_DEVICE_NAME, property.toUtf8().constData(), items.size(), (const char **)name_ptrs.constData(), values.constData());
}

bool SolverClient::solve_image(const unsigned char *data, size_t size) {
	// the agent copies the image before this call returns
	return indigo_change_blob_property_1(
		m_client,
		PLATESOLVER_DEVICE_NAME,
		AGENT_PLATESOLVER_IMAGE_PROPERTY_NAME,
		AGENT_PLATESOLVER_IMAGE_ITEM_NAME,
		(void *)data,
		(long)size,
		"",
		""
	) == INDIGO_OK;
}

void SolverClient::abort() {
	indigo_change_switch_property_1(m_client, PLATESOLVER_DEVICE_NAME, AGENT_ABORT_PROCESS_PROPERTY_NAME, AGENT_ABORT_PROCESS_ITEM_NAME, true);
}

void SolverClient::handle_property(indigo_property *property, const char *message, bool define) {
	if (!is_solver_device(property->device)) return;
	if (message != nullptr && message[0] != '\0') {
		emit message_received(message);
	}
	if (property->type != INDIGO_SWITCH_VECTOR && property->type != INDIGO_NUMBER_VECTOR) return;

	SolverProperty p;
	p.name = property->name;
	p.state = property->state;
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = property->items + i;
		SolverItem si;
		si.name = item->name;
		si.label = item->label;
		si.number = property->type == INDIGO_NUMBER_VECTOR ? item->number.value : 0;
		si.on = property->type == INDIGO_SWITCH_VECTOR ? item->sw.value : false;
		p.items.append(si);
	}
	if (p.name == AGENT_PLATESOLVER_WCS_PROPERTY_NAME) {
		m_wcs_busy.storeRelease(property->state == INDIGO_BUSY_STATE);
	} else if (p.name == SOLVER_INDEX_PROPERTY_NAME) {
		m_index_busy.storeRelease(property->state == INDIGO_BUSY_STATE);
	}
	if (define) {
		emit property_defined(p);
	} else {
		emit property_updated(p);
	}
}

void SolverClient::handle_delete(indigo_property *property, const char *message) {
	if (!is_solver_device(property->device)) return;
	if (message != nullptr && message[0] != '\0') {
		emit message_received(message);
	}
	emit property_deleted(property->name);
}

void SolverClient::handle_message(const char *device, const char *message) {
	if (device[0] != '\0' && !is_solver_device(device)) return;
	if (message != nullptr && message[0] != '\0') {
		emit message_received(message);
	}
}
