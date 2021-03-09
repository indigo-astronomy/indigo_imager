// Copyright (c) 2020 Rumen G.Bogdanovski
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

#include "imagerwindow.h"
#include "propertycache.h"
#include "conf.h"
#include <QLCDNumber>

#include <image_preview_lut.h>

void write_conf();

void ImagerWindow::create_telescope_tab(QFrame *telescope_frame) {
	QGridLayout *telescope_frame_layout = new QGridLayout();
	telescope_frame_layout->setAlignment(Qt::AlignTop);
	telescope_frame_layout->setColumnStretch(0, 2);
	telescope_frame_layout->setColumnStretch(1, 2);
	telescope_frame_layout->setColumnStretch(2, 3);
	telescope_frame_layout->setColumnStretch(3, 3);

	telescope_frame->setLayout(telescope_frame_layout);
	telescope_frame->setFrameShape(QFrame::StyledPanel);
	telescope_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	telescope_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	m_agent_mount_select = new QComboBox();
	telescope_frame_layout->addWidget(m_agent_mount_select, row, 0, 1, 4);
	connect(m_agent_mount_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImagerWindow::on_mount_agent_selected);

	row++;
	QLabel *label = new QLabel("Mount:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);
	m_mount_select = new QComboBox();
	telescope_frame_layout->addWidget(m_mount_select, row, 1, 1, 3);
	connect(m_mount_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_mount_selected);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	#define LCD_SIZE 24
	row++;
	label = new QLabel("RA / Dec:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);

	m_mount_ra_label = new QLCDNumber(13);
	m_mount_ra_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_ra_label->setMinimumHeight(LCD_SIZE);
	m_mount_ra_label->display("00: 00:00.00");
	set_ok(m_mount_ra_label);
	m_mount_ra_label->show();
	telescope_frame_layout->addWidget(m_mount_ra_label, row, 2, 1, 2);

	row++;
	m_mount_dec_label = new QLCDNumber(13);
	m_mount_dec_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_dec_label->setMinimumHeight(LCD_SIZE);
	m_mount_dec_label->display("00' 00 00.00");
	set_ok(m_mount_dec_label);
	m_mount_dec_label->show();
	telescope_frame_layout->addWidget(m_mount_dec_label, row, 2, 1, 2);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("Az / Alt:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);

	m_mount_az_label = new QLCDNumber(13);
	m_mount_az_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_az_label->setMinimumHeight(LCD_SIZE);
	m_mount_az_label->display("00' 00 00.00");
	set_ok(m_mount_az_label);
	m_mount_az_label->show();
	telescope_frame_layout->addWidget(m_mount_az_label, row, 2, 1, 2);

	row++;
	m_mount_alt_label = new QLCDNumber(13);
	m_mount_alt_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_alt_label->setMinimumHeight(LCD_SIZE);
	m_mount_alt_label->display("00' 00 00.00");
	set_ok(m_mount_alt_label);
	m_mount_alt_label->show();
	telescope_frame_layout->addWidget(m_mount_alt_label, row, 2, 1, 2);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("LST:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);

	m_mount_lst_label = new QLCDNumber(13);
	m_mount_lst_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_lst_label->setMinimumHeight(LCD_SIZE);
	m_mount_lst_label->display("00: 00:00.00");
	set_ok(m_mount_lst_label);
	m_mount_lst_label->show();
	telescope_frame_layout->addWidget(m_mount_lst_label, row, 2, 1, 2);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("Ra / Dec input:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0, 1, 2);

	m_mount_ra_input = new QLineEdit();
	telescope_frame_layout->addWidget(m_mount_ra_input, row, 2);

	m_mount_dec_input = new QLineEdit();
	telescope_frame_layout->addWidget(m_mount_dec_input, row, 3);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	telescope_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	m_mount_goto_button = new QPushButton("Goto");
	m_mount_goto_button->setStyleSheet("min-width: 30px");
	m_mount_goto_button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(m_mount_goto_button);
	connect(m_mount_goto_button, &QPushButton::clicked, this, &ImagerWindow::on_mount_goto);

	m_mount_sync_button = new QPushButton("Sync");
	m_mount_sync_button->setStyleSheet("min-width: 30px");
	m_mount_sync_button->setIcon(QIcon(":resource/calibrate.png"));
	toolbox->addWidget(m_mount_sync_button);
	connect(m_mount_sync_button , &QPushButton::clicked, this, &ImagerWindow::on_mount_sync);

	m_mount_abort_button = new QPushButton("Abort");
	m_mount_abort_button->setStyleSheet("min-width: 30px");
	m_mount_abort_button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(m_mount_abort_button);
	connect(m_mount_abort_button, &QPushButton::clicked, this, &ImagerWindow::on_mount_abort);
}

void ImagerWindow::on_mount_agent_selected(int index) {
	QtConcurrent::run([=]() {
		// Clear controls
		indigo_property *property = (indigo_property*)malloc(sizeof(indigo_property));
		memset(property, 0, sizeof(indigo_property));
		get_selected_mount_agent(property->device);
		property_delete(property, nullptr);
		free(property);

		indigo_enumerate_properties(nullptr, &INDIGO_ALL_PROPERTIES);
	});
}

void ImagerWindow::on_mount_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_mount[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_mount_str = m_mount_select->currentText();
		if (q_mount_str.compare("No mount") == 0) {
			strcpy(selected_mount, "NONE");
		} else {
			strncpy(selected_mount, q_mount_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_mount);
		static const char * items[] = { selected_mount };

		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_MOUNT_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_mount_goto(int index) {
	QtConcurrent::run([=]() {
		static char selected_mount[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_mount_str = m_mount_select->currentText();
		if (q_mount_str.compare("No mount") == 0) {
			strcpy(selected_mount, "NONE");
		} else {
			strncpy(selected_mount, q_mount_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_mount);

		change_mount_agent_equatorial(selected_agent, false);
	});
}

void ImagerWindow::on_mount_sync(int index) {
	QtConcurrent::run([=]() {
		static char selected_mount[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_mount_str = m_mount_select->currentText();
		if (q_mount_str.compare("No mount") == 0) {
			strcpy(selected_mount, "NONE");
		} else {
			strncpy(selected_mount, q_mount_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_mount);

		change_mount_agent_equatorial(selected_agent, true);
	});
}

void ImagerWindow::on_mount_abort(int index) {
	QtConcurrent::run([=]() {
		static char selected_mount[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_mount_str = m_mount_select->currentText();
		if (q_mount_str.compare("No mount") == 0) {
			strcpy(selected_mount, "NONE");
		} else {
			strncpy(selected_mount, q_mount_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_mount);

		change_mount_agent_abort(selected_agent);
	});
}
