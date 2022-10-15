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


#ifndef QINDIGOSERVERS_H
#define QINDIGOSERVERS_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QLineEdit>
#include "qindigoservice.h"

class QIndigoServers : public QDialog
{
	Q_OBJECT
public:
	QIndigoServers(QWidget *parent = 0);
	QString getServiceName(QListWidgetItem* item);

signals:
	void requestConnect(const QString &service);
	void requestDisconnect(const QString &service);
	void requestAddManualService(QIndigoService &indigo_service);
	void requestRemoveManualService(const QString &service);
	void requestSaveServices();

public slots:
	void onClose();
	void onAddService(QIndigoService &indigo_service);
	void onRemoveService(QIndigoService &indigo_service);
	void highlightChecked(QListWidgetItem* item);
	void onConnectionChange(QIndigoService &indigo_service);
	void onAddManualService();
	void onRemoveManualService();

private:
	QListWidget* m_server_list;
	QDialogButtonBox* m_button_box;
	QWidget* m_view_box;
	QWidget* m_add_service_box;
	QLineEdit* m_service_line;
	QPushButton* m_add_button;
	QPushButton* m_remove_button;
	QPushButton* m_close_button;
};

#endif // QINDIGO_SERVERS_H
