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


#include "qindigoservers.h"
#include <QRegularExpressionValidator>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#define QT_SKIP_EMPTY_PARTS Qt::SkipEmptyParts
#else
#define QT_SKIP_EMPTY_PARTS QString::SkipEmptyParts
#endif

QIndigoServers::QIndigoServers(QWidget *parent): QDialog(parent)
{
	setWindowTitle("Available Services");

	m_server_list = new QListWidget;
	m_view_box = new QWidget();
	m_button_box = new QDialogButtonBox;
	m_service_line = new QLineEdit;
	m_service_line->setMinimumWidth(300);
	m_service_line->setToolTip(
		"service formats:\n"
		"        service@hostname:port\n"
		"        hostname:port\n"
		"        hostname\n"
		"\nservice can be any user defined name,\n"
		"if omitted hostname will be used."
	);
	//m_add_button = m_button_box->addButton(tr("Add service"), QDialogButtonBox::ActionRole);
	m_add_button = new QPushButton(" &Add ");
	m_add_button->setDefault(true);
	m_remove_button = m_button_box->addButton(tr("Remove selected"), QDialogButtonBox::ActionRole);
	m_remove_button->setToolTip(
		"Remove highlighted service.\n"
		"Only manually added services can be removed."
	);
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);

	QVBoxLayout* viewLayout = new QVBoxLayout;
	viewLayout->setContentsMargins(0, 0, 0, 0);
	viewLayout->addWidget(m_server_list);

	m_add_service_box = new QWidget();
	QHBoxLayout* addLayout = new QHBoxLayout;
	addLayout->setContentsMargins(0, 0, 0, 0);
	addLayout->addWidget(m_service_line);
	addLayout->addWidget(m_add_button);
	m_add_service_box->setLayout(addLayout);

	viewLayout->addWidget(m_add_service_box);
	m_view_box->setLayout(viewLayout);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(m_button_box);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(m_view_box);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);

	QObject::connect(m_server_list, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(highlightChecked(QListWidgetItem*)));
	QObject::connect(m_add_button, SIGNAL(clicked()), this, SLOT(onAddManualService()));
	QObject::connect(m_remove_button, SIGNAL(clicked()), this, SLOT(onRemoveManualService()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onClose()));
}


void QIndigoServers::onClose() {
	emit(requestSaveServices());
	close();
}

void QIndigoServers::onConnectionChange(QString service_name, bool is_connected) {
	indigo_debug("Connection State Change [%s] connected = %d\n", service_name.toUtf8().constData(), is_connected);
	QListWidgetItem* item = 0;
	for(int i = 0; i < m_server_list->count(); ++i){
		item = m_server_list->item(i);
		QString service = getServiceName(item);
		if (service == service_name) {
			if (is_connected)
				item->setCheckState(Qt::Checked);
			else
				item->setCheckState(Qt::Unchecked);
			break;
		}
	}
}


void QIndigoServers::onAddService(QString name, QString host, int port, bool is_auto_service, bool is_connected) {
	QString server_string = name + tr(" @ ") + host + tr(":") + QString::number(port);
	QList<QListWidgetItem *> items = m_server_list->findItems(server_string, Qt::MatchExactly);
	if (items.size() > 0) {
		indigo_debug("SERVER IN IS IN THE MENU [%s]", server_string.toUtf8().constData());
		return;
	}

	QListWidgetItem* item = new QListWidgetItem(server_string);

	item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
	if (is_connected)
		item->setCheckState(Qt::Checked);
	else
		item->setCheckState(Qt::Unchecked);

	if (is_auto_service) {
		item->setData(Qt::DecorationRole,QIcon(":resource/bonjour_service.png"));
	} else {
		item->setData(Qt::DecorationRole,QIcon(":resource/manual_service.png"));
	}
	m_server_list->addItem(item);
}


void QIndigoServers::onAddManualService() {
	int port = 7624;
	QString hostname;
	QString service;
	QString service_str = m_service_line->text().trimmed();
	if (service_str.isEmpty()) {
		indigo_debug("Trying to add empty service!");
		return;
	}
	QStringList parts = service_str.split(':', QT_SKIP_EMPTY_PARTS);
	if (parts.size() > 2) {
		indigo_error("%s(): Service format error.\n",__FUNCTION__);
		return;
	} else if (parts.size() == 2) {
		port = atoi(parts.at(1).toUtf8().constData());
	}
	QStringList parts2 = parts.at(0).split('@', QT_SKIP_EMPTY_PARTS);
	if (parts2.size() > 2) {
		indigo_error("%s(): Service format error.\n",__FUNCTION__);
		return;
	} else if (parts2.size() == 2) {
		service = parts2.at(0);
		hostname = parts2.at(1);
	} else {
		hostname = parts2.at(0);
		service = parts2.at(0);
		int index = service.indexOf(QChar('.'));
		if (index > 0) {
			// if IP address is provided use the whole IP as a service name
			QString ip_range = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
			QRegularExpression ip_regex (
				"^" + ip_range
				+ "(\\." + ip_range + ")"
				+ "(\\." + ip_range + ")"
				+ "(\\." + ip_range + ")$"
			);
			QRegularExpressionValidator ip_validator(ip_regex);
			int pos = 0;
			if(QValidator::Acceptable != ip_validator.validate(service, pos)) {
				service.truncate(index);
			}
		}
	}

	QIndigoService indigo_service(service.toUtf8(), hostname.toUtf8(), port);
	emit(requestAddManualService(indigo_service));
	m_service_line->setText("");
	indigo_debug("ADD: Service '%s' host '%s' port = %d\n", service.toUtf8().constData(), hostname.toUtf8().constData(), port);
}


void QIndigoServers::onRemoveService(QString service_name) {
	QListWidgetItem* item = 0;
	for(int i = 0; i < m_server_list->count(); ++i){
		item = m_server_list->item(i);
		QString service = getServiceName(item);
		if (service == service_name) {
			delete item;
			break;
		}
	}
}


void QIndigoServers::highlightChecked(QListWidgetItem *item){
	QString service = getServiceName(item);
	if(item->checkState() == Qt::Checked)
		emit(requestConnect(service));
	else
		emit(requestDisconnect(service));
}


void QIndigoServers::onRemoveManualService() {
	QModelIndex index = m_server_list->currentIndex();
	QString service = index.data(Qt::DisplayRole).toString();
	int pos = service.indexOf('@');
	if (pos > 0) service.truncate(pos);
	service = service.trimmed();
	indigo_debug("TO BE REMOVED: [%s]\n", service.toUtf8().constData());
	emit(requestRemoveManualService(service));
}


QString QIndigoServers::getServiceName(QListWidgetItem* item) {
	QString service = item->text();
	int pos = service.indexOf('@');
	if (pos > 0) service.truncate(pos);
	service = service.trimmed();
	return service;
}
