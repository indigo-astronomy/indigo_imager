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


#ifndef SOLVERCLIENT_H
#define SOLVERCLIENT_H

#include <QObject>
#include <QAtomicInt>
#include <QMetaType>
#include <QString>
#include <QVector>
#include <indigo/indigo_bus.h>
#include <indigo/indigo_client.h>

#define PLATESOLVER_DRIVER_NAME "indigo_agent_platesolver"
#define PLATESOLVER_DEVICE_NAME "Platesolver Agent"
#define SOLVER_INDEX_PROPERTY_NAME "X_AGENT_SOLVER_INDEX"

struct SolverItem {
	QString name;
	QString label;
	double number;
	bool on;
};

struct SolverProperty {
	QString name;
	indigo_property_state state;
	QVector<SolverItem> items;

	const SolverItem *item(const char *item_name) const {
		for (const SolverItem &i : items) {
			if (i.name == item_name) return &i;
		}
		return nullptr;
	}
};

Q_DECLARE_METATYPE(SolverProperty)

// The Platesolver Agent runs in this process and shares the bus with this client. The callbacks
// arrive on threads of the agent, the signals are delivered to the GUI thread by queued connections.
class SolverClient : public QObject {
	Q_OBJECT
public:
	explicit SolverClient(QObject *parent = nullptr);
	virtual ~SolverClient();

	// False if the agent is not installed or it can not be loaded, the reason is in last_error()
	bool load_agent();
	// Aborts whatever the agent is doing and unloads it. Waits for it, the agent can not be detached while it works
	void stop();
	QString last_error() const { return m_last_error; }

	void set_switches(const QString &property, const QVector<SolverItem> &items);
	void set_numbers(const QString &property, const QVector<SolverItem> &items);
	bool solve_image(const unsigned char *data, size_t size);
	void abort();

	// called from the agent threads
	void handle_property(indigo_property *property, const char *message, bool define);
	void handle_delete(indigo_property *property, const char *message);
	void handle_message(const char *device, const char *message);

signals:
	void property_defined(const SolverProperty &property);
	void property_updated(const SolverProperty &property);
	void property_deleted(const QString &name);
	void message_received(const QString &message);

private:
	bool agent_busy() const;

	indigo_client *m_client;
	indigo_driver_entry *m_driver;
	QAtomicInt m_wcs_busy;
	QAtomicInt m_index_busy;
	QString m_last_error;
	bool m_started;
};

#endif // SOLVERCLIENT_H
