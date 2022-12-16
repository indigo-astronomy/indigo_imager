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
#include <indigo/indigo_bus.h>

QConfigDialog::QConfigDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Manage configuration");

	QFrame *frame = new QFrame;
	QGridLayout *frame_layout = new QGridLayout();
	frame_layout->setAlignment(Qt::AlignTop);
	frame->setLayout(frame_layout);
	frame->setContentsMargins(0, 0, 0, 0);
	frame_layout->setContentsMargins(0, 0, 0, 0);

	//int row = 0;
	//QLabel *label = new QLabel("Ageent:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	//frame_layout->addWidget(label, row, 0, 1, 3);

	int row = 0;
	m_config_agent_select = new QComboBox();
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
	m_add_config_button->setToolTip(tr("Save configuration"));
	m_add_config_button->setIcon(QIcon(":resource/save.png"));
	frame_layout->addWidget(m_add_config_button, row, 1);
	connect(m_add_config_button, &QToolButton::clicked, this, &QConfigDialog::onSaveConfigCB);

	m_remove_config_button = new QToolButton(this);
	m_remove_config_button->setToolTip(tr("Remove selected configuration"));
	m_remove_config_button->setIcon(QIcon(":resource/delete.png"));
	frame_layout->addWidget(m_remove_config_button, row, 2);
	connect(m_remove_config_button, &QToolButton::clicked, this, &QConfigDialog::onRemoveConfigCB);

	row++;
	m_save_devices_cbox = new QCheckBox("Save device profiles");
	m_save_devices_cbox->setToolTip("Save device configurations to the selected profiles");
	frame_layout->addWidget(m_save_devices_cbox, row, 0, 1, 3);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	frame_layout->addItem(spacer, row, 0, 1, 3);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	m_button_box = new QDialogButtonBox;
	m_load_button = m_button_box->addButton(tr("Load"), QDialogButtonBox::ActionRole);
	m_load_button->setAutoDefault(false);
	m_load_button->setDefault(false);
	m_save_button = m_button_box->addButton(tr("Save"), QDialogButtonBox::ActionRole);
	m_save_button->setAutoDefault(true);
	m_save_button->setDefault(true);
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

	QObject::connect(m_save_button, SIGNAL(clicked()), this, SLOT(onSaveConfigCB()));
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

}

void QConfigDialog::onAgentChangedCB(int index) {
	bool checked = m_config_agent_select->itemData(index).toBool();
	m_save_devices_cbox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
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
	configItem.configName = m_configuration_select->currentText();
	emit(requestSaveConfig(configItem));
}

void QConfigDialog::onLoadConfigCB() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
	configItem.configName = m_configuration_select->currentText();
	emit(requestLoadConfig(configItem));
}

void QConfigDialog::onRemoveConfigCB() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
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
		m_config_agent_select->addItem(item.configAgent, item.saveDeviceConfigs);
		indigo_debug("[ADD] %s\n", item.configAgent.toUtf8().data());
		emit(agentChanged(m_config_agent_select->currentText()));
	} else {
		m_config_agent_select->setItemData(index, item.saveDeviceConfigs);
		m_save_devices_cbox->setCheckState(item.saveDeviceConfigs ? Qt::Checked : Qt::Unchecked);
		indigo_debug("[DUPLICATE Updating data] %s\n", item.configAgent.toUtf8().data());
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
		bool checked = m_config_agent_select->currentData().toBool();
		m_save_devices_cbox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
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
}

void QConfigDialog::onClearConfigs() {
	m_configuration_select->clear();
}
