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

#include <qzeroconf.h>
#include "logger.h"


class QIndigoService;


class QServiceModel : public QAbstractListModel
{
	Q_OBJECT

public:
	QServiceModel(const QByteArray &type);
	~QServiceModel();

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
	virtual QVariant data(const QModelIndex &index, int role) const;

signals:
	void serviceAdded(QIndigoService &indigo_service);
	void serviceRemoved(QIndigoService &indigo_service);
	void serviceConnectionChange(QIndigoService &indigo_service);

private Q_SLOTS:
	void onServiceError(QZeroConf::error_t);
	void onServiceAdded(QZeroConfService s);
	void onServiceUpdated(QZeroConfService s);
	void onServiceRemoved(QZeroConfService s);
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
    QZeroConf m_zero_conf;
};

#endif // SERVICEMODEL_H
