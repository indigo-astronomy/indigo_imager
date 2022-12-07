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

	int row = 0;
	QLabel *label = new QLabel("Current configuration:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);

	row++;
	m_config_agent_select = new QComboBox();
	frame_layout->addWidget(m_config_agent_select, row, 0);

	row++;
	m_save_devices_cbox = new QCheckBox("Save device profiles");
	m_save_devices_cbox->setToolTip("Save device configurations to the selected profiles");
	frame_layout->addWidget(m_save_devices_cbox, row, 0);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	frame_layout->addItem(spacer, row, 0);

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
	onClear();

	QObject::connect(m_save_button, SIGNAL(clicked()), this, SLOT(onSaveConfig()));
	QObject::connect(m_load_button, SIGNAL(clicked()), this, SLOT(onLoadConfig()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onClose()));
	connect(this, &QConfigDialog::populate, this, &QConfigDialog::onPopulate);
	connect(this, &QConfigDialog::setActive, this, &QConfigDialog::onSetActive);
	connect(this, &QConfigDialog::clear, this, &QConfigDialog::onClear);
}

void QConfigDialog::onClose() {
	close();
}

void QConfigDialog::onSaveConfig() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
	configItem.configName = "Default_ain_config";
	emit(requestSaveConfig(configItem));
}

void QConfigDialog::onLoadConfig() {
	ConfigItem configItem;
	configItem.configAgent = m_config_agent_select->currentText();
	configItem.saveDeviceConfigs = m_save_devices_cbox->checkState();
	configItem.configName = "Default_ain_config";
	emit(requestLoadConfig(configItem));
}

void QConfigDialog::onPopulate(QList<ConfigItem> configTargets) {
	m_config_agent_select->clear();
	for (int i=0; i<configTargets.count(); i++) {
		ConfigItem item = configTargets[i];
		if (m_config_agent_select->findText(item.configAgent) < 0) {
			m_config_agent_select->addItem(item.configAgent, item.saveDeviceConfigs);
			indigo_debug("[ADD] %s\n", item.configAgent.toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE] %s\n", item.configAgent.toUtf8().data());
		}
	}
	m_save_devices_cbox->setCheckState(configTargets[0].saveDeviceConfigs ? Qt::Checked : Qt::Unchecked);
}

void QConfigDialog::onSetActive(QString agentName) {
	int index = m_config_agent_select->findText(agentName);
	if (index >= 0) {
		indigo_debug("[SELECTING] %s\n", agentName.toUtf8().data());
		m_config_agent_select->setCurrentIndex(index);
		bool checked = m_config_agent_select->currentData().toBool();
		m_save_devices_cbox->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	}
}

void QConfigDialog::onClear() {
	m_config_agent_select->clear();
	m_save_devices_cbox->setCheckState(Qt::Unchecked);
}
