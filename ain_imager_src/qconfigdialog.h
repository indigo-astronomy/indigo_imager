// Copyright (c) 2022 Rumen G.Bogdanovski
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


#ifndef __QCONFIGDIALOG_H
#define __QCONFIGDIALOG_H

#include <QObject>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>

#define DEFAULT_CONFIG "Ain_default"

struct ConfigItem {
	QString configAgent;
	QString configName;
	bool saveDeviceConfigs;
};

class QConfigDialog : public QDialog
{
	Q_OBJECT
public:
	QConfigDialog(QWidget *parent = nullptr);
	~QConfigDialog(){ };

signals:
	void requestSaveConfig(ConfigItem configItem);
	void requestLoadConfig(ConfigItem configTarget);
	void addAgent(ConfigItem item);
	void removeAgent(QString agentName);
	void setActive(QString agentName);
	void clear();

public slots:
	void onSetActive(QString agentName);
	void onAddAgent(ConfigItem item);
	void onRemoveAgent(QString agentName);

	void onAgentChanged(int index);
	void onSaveConfig();
	void onLoadConfig();
	void onClose();
	void onClear();

private:
	QComboBox *m_config_agent_select;
	QCheckBox *m_save_devices_cbox;

	QDialogButtonBox *m_button_box;
	QPushButton *m_load_button;
	QPushButton *m_save_button;
	QPushButton *m_close_button;
};

#endif // __QCONFOGDIALOG_H
