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


#ifndef SERVICEMODEL_H
#define SERVICEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QTimer>

#include "logger.h"


class QIndigoService;


class QServiceModel : public QAbstractListModel
{
	Q_OBJECT

public:
	QServiceModel();
	~QServiceModel();

	static QServiceModel& instance();

	void saveManualServices();
	void loadManualServices();
	virtual int rowCount(const QModelIndex &parent) const;
	bool addService(QByteArray name, QByteArray host, int port);
	bool addService(QByteArray name, QByteArray host, int port, bool auto_connect, bool is_manual_service);
	bool connectService(QByteArray name);
	bool disconnectService(QByteArray name);
	bool removeService(QByteArray name);
	void enable_auto_connect(bool enable) {
		m_auto_connect = enable;
	};
	void addServicePreferLocalhost(QByteArray service_name, uint32_t interface_index, QByteArray host, int port);
	void removeServiceKeepLocalhost(QByteArray service_name, uint32_t interface_index);
	virtual QVariant data(const QModelIndex &index, int role) const;
	void onServiceAdded(QByteArray name, QByteArray host, int port);
	void onServiceRemoved(QByteArray name);

signals:
	void serviceAdded(QString name, QString host, int port, bool is_auto_service, bool is_connected);
	void serviceRemoved(QString name);
	void serviceConnectionChange(QString service_name, bool is_connected);

private Q_SLOTS:
	void onTimer();
public Q_SLOTS:
	void onRequestConnect(const QString &service);
	void onRequestAddManualService(QIndigoService &indigo_service);
	void onRequestRemoveManualService(const QString &service);
	void onRequestDisconnect(const QString &service);
	void onRequestSaveServices();

private:
	int findService(const QByteArray &name);

	Logger* m_logger;
	bool m_auto_connect;
	QList<QIndigoService*> m_services;
};

inline QServiceModel& QServiceModel::instance() {
	static QServiceModel* me = nullptr;
	if (!me)
		me = new QServiceModel();
	return *me;
}

#endif // SERVICEMODEL_H
