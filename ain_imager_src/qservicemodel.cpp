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

#include <QRegularExpression>
#include <QStandardPaths>
#include "qindigoservice.h"
#include "qservicemodel.h"
#include <indigo/indigo_client.h>
#include <indigo/indigo_service_discovery.h>
#include "conf.h"

#define SERVICE_FILENAME "indigo_imager.services"

void resolve_callback(const char *service_name, uint32_t interface_index, const char *host, int port) {
	if (host != NULL) {
		QServiceModel *model = NULL;
		model = &QServiceModel::instance();
		indigo_debug("resolved %p", model);
		if (model) {
			model->addServicePreferLocalhost(QByteArray(service_name), interface_index, QByteArray(host), port);
		}
	}
}


void discover_callback(indigo_service_discovery_event event, const char *service_name, uint32_t interface_index) {
	if (event == INDIGO_SERVICE_ADDED) {
		indigo_resolve_service(service_name, interface_index, resolve_callback);
	} else if (event == INDIGO_SERVICE_REMOVED) {
		QServiceModel *model = &QServiceModel::instance();
		if (model) {
			model->removeServiceKeepLocalhost(QByteArray(service_name), interface_index);
		}
	}
}


QServiceModel::QServiceModel() {
	m_logger = &Logger::instance();
	m_auto_connect = true;
	QTimer *m_timer = new QTimer(this);
	connect(m_timer, &QTimer::timeout, this, QOverload<>::of(&QServiceModel::onTimer));
	m_timer->start(3000);
	indigo_start_service_browser(discover_callback);
	indigo_debug("Service discovery started");
}


QServiceModel::~QServiceModel() {
	saveManualServices();
	while (!m_services.isEmpty()) delete m_services.takeFirst();
	indigo_stop_service_browser();
	indigo_debug("Service discovery stopped");
}


void QServiceModel::addServicePreferLocalhost(QByteArray service_name, uint32_t interface_index, QByteArray host, int port) {
	if (interface_index == 1 ) { // if interface is loopback, use localhost, it is imune to interface drops
		int i = findService(service_name);
		if (i != -1 && m_services.at(i)->host() != QByteArray("localhost")) {
			onServiceRemoved(service_name);
		}
		onServiceAdded(service_name, QByteArray("localhost"), port);
	} else {
		onServiceAdded(service_name, host, port);
	}
}


void QServiceModel::removeServiceKeepLocalhost(QByteArray service_name, uint32_t interface_index) {
	int i = findService(service_name);
	if (i != -1) {
		if (m_services.at(i)->host() == QByteArray("localhost")) {
			if (interface_index == 1) {
				onServiceRemoved(service_name);
			}
		} else {
			onServiceRemoved(service_name);
		}
	}
}


void QServiceModel::saveManualServices() {
	char filename[PATH_LEN];
	snprintf(filename, PATH_LEN, "%s/%s", config_path, SERVICE_FILENAME);
	FILE *file = fopen(filename, "w");
	if (file != NULL) {
		for (auto i = m_services.constBegin(); i != m_services.constEnd(); ++i) {
			char service_type;
			if((*i)->is_auto_service) {
				service_type = 'A';
			} else {
				service_type = 'M';
			}
			indigo_debug("Saved service: %s@%s:%d, %c, %d\n", (*i)->name().constData(), (*i)->host().constData(), (*i)->port(), service_type, (*i)->auto_connect);
			fprintf(file, "%s@%s:%d, %c, %d\n", (*i)->name().constData(), (*i)->host().constData(), (*i)->port(), service_type, (*i)->auto_connect);
		}
		fclose(file);
	}
}


void QServiceModel::loadManualServices() {
	char filename[PATH_LEN];
	char name[256] = {0};
	char host[256] = {0};
	char type = 0;
	int auto_connect = 1;
	int port = 7624;
	snprintf(filename, PATH_LEN, "%s/%s", config_path, SERVICE_FILENAME);
	FILE * file = fopen(filename, "r");
	if (file != NULL) {
		indigo_debug("Services file open: %s\n", filename);
		while (fscanf(file,"%[^@]@%[^:]:%d, %c, %d\n", name, host, &port, &type, &auto_connect) == 5) {
			indigo_debug("Loading service: %s@%s:%d -> '%c' %d\n", name, host, port, type, auto_connect);
			addService(QByteArray(name), QByteArray(host), port, auto_connect, (type == 'A') ? 0 : 1);
		}
		fclose(file);
	}
}


void QServiceModel::onTimer() {
	for (auto i = m_services.constBegin(); i != m_services.constEnd(); ++i) {
		if (i == nullptr) continue;
		if ((*i)->m_server_entry == nullptr) continue;
#ifdef INDIGO_VERSION_3
		indigo_uni_handle *handle = (*i)->m_server_entry->handle;
		if (handle != (*i)->prev_handle) {
			indigo_debug("SERVICE Handles '%s' '%s' [%p] %p\n",(*i)->m_server_entry->name, (*i)->m_server_entry->host, handle, (*i)->prev_handle);
			(*i)->prev_handle = handle;
			emit(serviceConnectionChange((*i)->name(), (*i)->connected()));
		}
#else
		int socket = (*i)->m_server_entry->socket;
		if (socket != (*i)->prev_socket) {
			indigo_debug("SERVICE Sockets '%s' '%s' [%d] %d\n",(*i)->m_server_entry->name, (*i)->m_server_entry->host, socket, (*i)->prev_socket);
			(*i)->prev_socket = socket;
			emit(serviceConnectionChange((*i)->name(), (*i)->connected()));
		}
#endif
	}
}


int QServiceModel::rowCount(const QModelIndex &) const {
	return m_services.count();
}


bool QServiceModel::addService(QByteArray name, QByteArray host, int port, bool auto_connect, bool is_manual_service) {
	host = host.trimmed();
	if (host.endsWith('.')) {
		host.chop(1);
	}
	name = name.trimmed();
	int i = findService(name);
	if (i != -1) {
		indigo_debug("SERVICE DUPLICATE [%s]\n", name.constData());
		return false;
	}

	indigo_debug("SERVICE ADDED MANUALLY [%s]\n", name.constData());

	beginInsertRows(QModelIndex(), m_services.count(), m_services.count());
	QIndigoService* indigo_service = new QIndigoService(name, host, port, auto_connect, is_manual_service);
	m_services.append(indigo_service);
	endInsertRows();

	if (indigo_service->auto_connect && !indigo_service->is_auto_service) indigo_service->connect();
	if (!indigo_service->is_auto_service) {
		emit(
			serviceAdded(
				indigo_service->name(),
				indigo_service->host(),
				indigo_service->port(),
				indigo_service->is_auto_service,
				indigo_service->connected()
			)
		);
	}
	return true;
}


bool QServiceModel::addService(QByteArray name, QByteArray host, int port) {
	return addService(name, host, port, true, true);
}


bool QServiceModel::removeService(QByteArray name) {
	int i = findService(name);
	if (i != -1) {
		QIndigoService* indigo_service = m_services.at(i);
		if (indigo_service->is_auto_service) return false;
		beginRemoveRows(QModelIndex(), i, i);
		m_services.removeAt(i);
		endRemoveRows();
		indigo_service->disconnect();
		emit(serviceRemoved(indigo_service->name()));
		delete indigo_service;
		indigo_debug("SERVICE REMOVED [%s]\n", name.constData());
		return true;
	}
	indigo_debug("SERVICE DOES NOT EXIST [%s]\n", name.constData());
	return false;
}


bool QServiceModel::connectService(QByteArray name) {
	int i = findService(name);
	if (i == -1) {
		indigo_debug("SERVICE NOT FOUND [%s]\n", name.constData());
		return false;
	}
	QIndigoService* indigo_service = m_services.at(i);
	indigo_service->auto_connect = true;
	indigo_debug("CONNECTING TO SERVICE [%s] on %s:%d\n", name.constData(), indigo_service->host().constData(), indigo_service->port());
	return indigo_service->connect();
}


bool QServiceModel::disconnectService(QByteArray name) {
	int i = findService(name);
	if (i == -1) {
		indigo_debug("SERVICE NOT FOUND [%s]\n", name.constData());
		return false;
	}
	QIndigoService* indigo_service = m_services.at(i);
	indigo_service->auto_connect = false;
	indigo_debug("DISCONNECTING FROM SERVICE [%s] on %s:%d auto_connect=%d\n", name.constData(), indigo_service->host().constData(), indigo_service->port(), indigo_service->auto_connect);

	return indigo_service->disconnect();
}


QVariant QServiceModel::data(const QModelIndex &index, int role) const {
	// Ensure the index points to a valid row
	if (!index.isValid() || index.row() < 0 || index.row() >= m_services.count()) {
		return QVariant();
	}

	QIndigoService* service = m_services.at(index.row());

	switch (role) {
	case Qt::DisplayRole:
		return QString("%1:%2")
			.arg(QString(service->host()))
			.arg(service->port());
	case Qt::UserRole:
		return QVariant();
	}

	return QVariant();
}


void QServiceModel::onServiceAdded(QByteArray name, QByteArray host, int port) {
	QIndigoService* indigo_service = new QIndigoService(name, host, port, m_auto_connect, false);
	QIndigoService* stored_service = nullptr;
	int i = findService(name);
	if (i == -1) {
		indigo_debug("SERVICE ADDED [%s] on %s:%d\n", name.constData(), host.constData(), port);
		beginInsertRows(QModelIndex(), m_services.count(), m_services.count());
		m_services.append(indigo_service);
		endInsertRows();
		stored_service = indigo_service;
	} else {
		stored_service = m_services.at(i);
		if (
			stored_service->name() == indigo_service->name() &&
			(stored_service->host() == indigo_service->host() || indigo_service->host() == "localhost") &&
			stored_service->port() == indigo_service->port()
		) {
			if (indigo_service->host() == "localhost") {
				stored_service->setHost(indigo_service->host());
			}
			indigo_debug("SERVICE HAS RECORD [%s] connect = %d\n", name.constData(), stored_service->auto_connect);
			delete indigo_service;
		} else {
			indigo_debug("DUPLICATE SERVICE [%s]\n", name.constData());
			delete indigo_service;
			return;
		}
	}

	if (stored_service->auto_connect) stored_service->connect();
	emit(
		serviceAdded(
			stored_service->name(),
			stored_service->host(),
			stored_service->port(),
			stored_service->is_auto_service,
			stored_service->connected()
		)
	);
}


void QServiceModel::onServiceRemoved(QByteArray name) {
	indigo_debug("REMOVE SERVICE [%s]\n", name.constData());

	int i = findService(name);
	if (i != -1) {
		QIndigoService* indigo_service = m_services.at(i);
		indigo_debug("SERVICE REMOVED [%s]\n", name.constData());
		if (indigo_service->connected()) indigo_service->disconnect();
		emit(serviceRemoved(indigo_service->name()));
	}
}


void QServiceModel::onRequestConnect(const QString &service) {
	connectService(service.toUtf8());
}


void QServiceModel::onRequestDisconnect(const QString &service) {
	disconnectService(service.toUtf8());
}


void QServiceModel::onRequestAddManualService(QIndigoService &indigo_service) {
	if (addService(indigo_service.name(), indigo_service.host(), indigo_service.port())) saveManualServices();
}


void QServiceModel::onRequestRemoveManualService(const QString &service) {
	if (removeService(service.toUtf8())) saveManualServices();
}


void QServiceModel::onRequestSaveServices() {
	saveManualServices();
}


int QServiceModel::findService(const QByteArray &name) {
	for (auto i = m_services.constBegin(); i != m_services.constEnd(); ++i) {
		if ((*i)->name() == name) {
			return i - m_services.constBegin();
		}
	}
	return -1;
}
