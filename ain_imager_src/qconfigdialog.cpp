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

#include <QLabel>
#include "qconfigdialog.h"
#include "widget_state.h"
#include <indigo/indigo_bus.h>

QConfigDialog::QConfigDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Configuration manager");

	QFrame *frame = new QFrame;
	QGridLayout *frame_layout = new QGridLayout();
	frame_layout->setAlignment(Qt::AlignTop);
	frame->setLayout(frame_layout);
	frame->setContentsMargins(0, 0, 0, 0);
	frame_layout->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	//------ Remove when stable -------
	QLabel *label1 = new QLabel("Warning: This feature is experimental");
	label1->setToolTip("<p>This feature is experimental and may change. Backward and forward compatibility is not guaranted for the time being.</p>");
	label1->setStyleSheet(QString("color: #E00000; font: 8pt;"));
	frame_layout->addWidget(label1, row, 0, 1, 3);
	row++;
	//---------------------------------

	m_config_agent_select = new QComboBox();
	m_config_agent_select->setMinimumWidth(300);
	frame_layout->addWidget(m_config_agent_select, row, 0, 1, 3);
	connect(m_config_agent_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QConfigDialog::onAgentChangedCB);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	frame_layout->addItem(spacer, row, 0, 1, 3);

	row++;
	QLabel *label = new QLabel("Configuration:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0, 1, 3);

	row++;
	m_configuration_select = new QComboBox();
	m_configuration_select->setEditable(true);
	frame_layout->addWidget(m_configuration_select, row, 0);

	m_add_config_button = new QToolButton(this);
	m_add_config_button->setToolTip("<p>Save the current state to the selected configuration</p>");
	m_add_config_button->setIcon(QIcon(":resource/save.png"));
	frame_layout->addWidget(m_add_config_button, row, 1);
	connect(m_add_config_button, &QToolButton::clicked, this, &QConfigDialog::onSaveConfigCB);

	m_remove_config_button = new QToolButton(this);
	m_remove_config_button->setToolTip("<p>Remove selected configuration</p>");
	m_remove_config_button->setIcon(QIcon(":resource/delete.png"));
	frame_layout->addWidget(m_remove_config_button, row, 2);
	connect(m_remove_config_button, &QToolButton::clicked, this, &QConfigDialog::onRemoveConfigCB);

	row++;
	m_save_devices_cbox = new QCheckBox("Save device profiles");
	m_save_devices_cbox->setToolTip("<p>Save device specific properties to the selected profiles. If unchecked, device properties like gain, resolution etc., will not be saved.</p>");
	frame_layout->addWidget(m_save_devices_cbox, row, 0, 1, 3);

	row++;
	m_unload_drivers_cbox = new QCheckBox("Unload unused drivers");
	m_unload_drivers_cbox->setToolTip("<p>Unload all drivers not needed by the configuration being loaded</p>");
	frame_layout->addWidget(m_unload_drivers_cbox, row, 0, 1, 3);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	frame_layout->addItem(spacer, row, 0, 1, 3);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	m_button_box = new QDialogButtonBox;
	m_load_button = m_button_box->addButton(tr("Load"), QDialogButtonBox::ActionRole);
	m_load_button->setAutoDefault(false);
	m_load_button->setDefault(false);
	horizontalLayout->addWidget(m_button_box);
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);
	m_close_button->setAutoDefault(false);
	m_close_button->setDefault(false);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(frame);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);
	onClearAgents();
	onClearConfigs();

	QObject::connect(m_load_button, SIGNAL(clicked()), this, SLOT(onLoadConfigCB()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onCloseCB()));
	connect(this, &QConfigDialog::addAgent, this, &QConfigDialog::onAddAgent);
	connect(this, &QConfigDialog::removeAgent, this, &QConfigDialog::onRemoveAgent);
	connect(this, &QConfigDialog::addConfig, this, &QConfigDialog::onAddConfig);
	connect(this, &QConfigDialog::removeConfig, this, &QConfigDialog::onRemoveConfig);
	connect(this, &QConfigDialog::setActiveAgent, this, &QConfigDialog::onSetActiveAgent);
	connect(this, &QConfigDialog::setActiveConfig, this, &QConfigDialog::onSetActiveConfig);
	connect(this, &QConfigDialog::clearAgents, this, &QConfigDialog::onClearAgents);
	connect(this, &QConfigDialog::clearConfigs, this, &QConfigDialog::onClearConfigs);
	connect(this, &QConfigDialog::setState, this, &QConfigDialog::onSetState);
}

void QConfigDialog::onAgentChangedCB(int index) {
	emit(agentChanged(m_config_agent_select->currentText()));
}

QString QConfigDialog::getSelectedAgent() {
	return m_config_agent_select->currentText();
}

void QConfigDialog::onCloseCB() {
	close();
}

void QConfigDialog::onSaveConfigCB() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
	configItem.unloadDrivers = m_unload_drivers_cbox->checkState();
	configItem.configName = m_configuration_select->currentText();
	emit(requestSaveConfig(configItem));
}

void QConfigDialog::onLoadConfigCB() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
	configItem.unloadDrivers = m_unload_drivers_cbox->checkState();
	configItem.configName = m_configuration_select->currentText();
	emit(requestLoadConfig(configItem));
}

void QConfigDialog::onRemoveConfigCB() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
	configItem.unloadDrivers = m_unload_drivers_cbox->checkState();
	configItem.configName = m_configuration_select->currentText();

	int ret = QMessageBox::question(
		this,
		"Remove Configuration",
		"Congiguration <b>" + configItem.configName + "</b> from <b>" + configItem.configAgent + "</b> is about to be removed<br><br>" + "Do you want to continue?" ,
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);

	if (ret == QMessageBox::Yes) {
		emit(requestRemoveConfig(configItem));
	}
}

void QConfigDialog::onAddAgent(ConfigItem item) {
	//m_config_agent_select->blockSignals(true);
	int index = m_config_agent_select->findText(item.configAgent);
	if (index < 0) {
		m_config_agent_select->addItem(item.configAgent, item.configAgent);
		indigo_debug("[ADD] %s\n", item.configAgent.toUtf8().data());
		emit(agentChanged(m_config_agent_select->currentText()));
	} else if (item.configAgent == m_config_agent_select->currentText()) {
		m_save_devices_cbox->setCheckState(item.saveDeviceConfigs ? Qt::Checked : Qt::Unchecked);
		m_unload_drivers_cbox->setCheckState(item.unloadDrivers ? Qt::Checked : Qt::Unchecked);
		indigo_debug("[DUPLICATE SELECTED] %s\n", item.configAgent.toUtf8().data());
	} else {
		indigo_debug("[DUPLICATE] %s\n", item.configAgent.toUtf8().data());
	}
	//m_config_agent_select->blockSignals(false);
}

void QConfigDialog::onRemoveAgent(QString agentName) {
	int index = m_config_agent_select->findText(agentName);
	if (index >= 0) {
		m_config_agent_select->removeItem(index);
		indigo_debug("[REMOVE] %s\n", agentName.toUtf8().data());
		emit(agentChanged(m_config_agent_select->currentText()));
	} else {
		indigo_debug("[NOT FOUND] %s\n", agentName.toUtf8().data());
	}
}

void QConfigDialog::onAddConfig(QString configName) {
	int index = m_configuration_select->findText(configName);
	if (index < 0) {
		m_configuration_select->addItem(configName, configName);
		indigo_debug("[ADD] %s\n", configName.toUtf8().data());
	} else {
		indigo_debug("[DUPLICATE Updating data] %s\n", configName.toUtf8().data());
	}
}

void QConfigDialog::onRemoveConfig(QString configName) {
	int index = m_config_agent_select->findText(configName);
	if (index >= 0) {
		m_configuration_select->removeItem(index);
		indigo_debug("[REMOVE] %s\n", configName.toUtf8().data());
	} else {
		indigo_debug("[NOT FOUND] %s\n", configName.toUtf8().data());
	}
}

void QConfigDialog::onSetActiveAgent(QString agentName) {
	int index = m_config_agent_select->findText(agentName);
	if (index >= 0) {
		indigo_debug("[SELECTING] %s\n", agentName.toUtf8().data());
		m_config_agent_select->setCurrentIndex(index);
		emit(agentChanged(agentName));
	}
}

void QConfigDialog::onSetActiveConfig(QString configName) {
	indigo_debug("[SELECTING] %s\n", configName.toUtf8().data());
	m_configuration_select->setCurrentText(configName);
}

void QConfigDialog::onClearAgents() {
	m_config_agent_select->clear();
	m_save_devices_cbox->setCheckState(Qt::Unchecked);
	m_unload_drivers_cbox->setCheckState(Qt::Unchecked);
}

void QConfigDialog::onClearConfigs() {
	m_configuration_select->clear();
	onSetState(INDIGO_OK_STATE);
}

void QConfigDialog::onSetState(int state) {
	switch(state) {
		case INDIGO_OK_STATE:
			set_ok(m_configuration_select);
			break;
		case INDIGO_BUSY_STATE:
			set_busy(m_configuration_select);
			break;
		case INDIGO_ALERT_STATE:
			set_alert(m_configuration_select);
			break;
		default:
			set_ok(m_configuration_select);
	}
}
