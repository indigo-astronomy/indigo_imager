// Copyright (c) 2023 Rumen G.Bogdanovski
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
#include "indigoclient.h"
#include "propertycache.h"
#include <conf.h>
#include <utils.h>

void ImagerWindow::create_sequence_tab(QFrame *sequence_frame) {
	QGridLayout *sequence_frame_layout = new QGridLayout();
	sequence_frame_layout->setAlignment(Qt::AlignTop);
	sequence_frame->setLayout(sequence_frame_layout);
	sequence_frame->setFrameShape(QFrame::StyledPanel);
	sequence_frame->setMinimumWidth(TOOLBAR_MIN_WIDTH);
	sequence_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;

	m_agent_scripting_select = new QComboBox();
	sequence_frame_layout->addWidget(m_agent_scripting_select, row, 0, 1, 4);
	connect(m_agent_imager_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImagerWindow::on_scripting_agent_selected);

	row++;
	QLabel *label = new QLabel("Image preview:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	sequence_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	m_seq_imager_viewer = new ImageViewer(this);
	m_seq_imager_viewer->showStretchButton(false);
	m_seq_imager_viewer->showZoomButtons(false);
	m_seq_imager_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	sequence_frame_layout->addWidget(m_seq_imager_viewer, row, 0, 1, 4);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	sequence_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	m_seq_start_button = new QPushButton("Run");
	m_seq_start_button->setStyleSheet("min-width: 30px");
	m_seq_start_button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(m_seq_start_button);
	connect(m_seq_start_button, &QPushButton::clicked, this, &ImagerWindow::on_sequence_start_stop);

	m_seq_pause_button = new QPushButton("Pause");
	toolbox->addWidget(m_seq_pause_button);
	m_seq_pause_button->setStyleSheet("min-width: 30px");
	m_seq_pause_button->setIcon(QIcon(":resource/pause.png"));
	connect(m_seq_pause_button, &QPushButton::clicked, this, &ImagerWindow::on_sequence_pause);

	m_seq_reset_button = new QPushButton("⟳ Reset");
	toolbox->addWidget(m_seq_reset_button);
	m_seq_reset_button->setStyleSheet("min-width: 30px");
	//m_seq_reset_button->setIcon(QIcon(":resource/pause.png"));
	connect(m_seq_reset_button, &QPushButton::clicked, this, &ImagerWindow::on_reset);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);


	row++;
	spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	label = new QLabel("Sequence progress:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	sequence_frame_layout->addWidget(label, row, 0, 1, 2);
	m_imager_status_label = new QLabel("<img src=\":resource/led-grey.png\"> Idle");
	sequence_frame_layout->addWidget(m_imager_status_label, row, 2, 1, 2);

	row++;
	m_seq_exposure_progress = new QProgressBar();
	sequence_frame_layout->addWidget(m_seq_exposure_progress, row, 0, 1, 4);
	m_seq_exposure_progress->setFormat("Exposure: Idle");
	m_seq_exposure_progress->setMaximum(1);
	m_seq_exposure_progress->setValue(0);

	row++;
	m_seq_batch_progress = new QProgressBar();
	sequence_frame_layout->addWidget(m_seq_batch_progress, row, 0, 1, 4);
	m_seq_batch_progress->setMaximum(1);
	m_seq_batch_progress->setValue(0);
	m_seq_batch_progress->setFormat("Batch: Idle");

	row++;
	m_seq_sequence_progress = new QProgressBar();
	sequence_frame_layout->addWidget(m_seq_sequence_progress, row, 0, 1, 4);
	m_seq_sequence_progress->setMaximum(1);
	m_seq_sequence_progress->setValue(0);
	m_seq_sequence_progress->setFormat("Sequence: Idle");

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	m_mount_meridian_flip_label = new QLabel("Meridian flip: <b>OFF</b>");
	set_ok(m_mount_meridian_flip_label);
	sequence_frame_layout->addWidget(m_mount_meridian_flip_label, row, 0, 1, 4);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	sequence_frame_layout->addItem(spacer, row, 0);

	row++;
	m_seq_esimated_duration = new QLabel(QString("Total exposure: ") + indigo_dtos(0, "%02d:%02d:%02.0f"));
	m_seq_esimated_duration->setToolTip("This is the total exposure time of the composed sequence.");
	sequence_frame_layout->addWidget(m_seq_esimated_duration, row, 0, 1, 3);

	QToolButton *tbutton = new QToolButton();
	//tbutton->setIcon(QIcon(":resource/download.png"));
	//tbutton->setText("⟳");
	tbutton->setText("Σ");
	tbutton->setToolTip("Calculate sequence total exposure");
	sequence_frame_layout->addWidget(tbutton, row, 3);
	connect(tbutton, &QToolButton::clicked, this, &ImagerWindow::on_recalculate_exposure);

}

void ImagerWindow::on_scripting_agent_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		// Clear controls
		indigo_property *property = (indigo_property*)malloc(sizeof(indigo_property));
		memset(property, 0, sizeof(indigo_property));
		get_selected_scripting_agent(property->device);
		property_delete(property, nullptr);
		free(property);

		indigo_enumerate_properties(nullptr, &INDIGO_ALL_PROPERTIES);
	});
}

void ImagerWindow::on_recalculate_exposure() {
	double totalExposure = m_sequence_editor2->totalExposure();
	m_seq_esimated_duration->setText(QString("Total exposure: ") + QString(indigo_dtos(totalExposure / 3600, "%02d:%02d:%02.0f")));
}

void ImagerWindow::on_sequence_name_changed(const QString &object_name) {
	if (!m_is_sequence) {
		return;
	}
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_ccd_localmode_property(selected_agent, object_name);
		add_fits_keyword_string(selected_agent, "OBJECT", object_name);
	});
}

void ImagerWindow::on_request_sequence() {
	indigo_debug("Sequence requested");

	QString sequence;
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_scripting_agent(selected_agent);

	char sequence_script[INDIGO_NAME_SIZE] = {0};
	indigo_property *p = nullptr;

	for (int i = 0; i < 32; i++) {
		snprintf(sequence_script, sizeof(sequence_script), AGENT_SCRIPTING_SCRIPT_PROPERTY_NAME, i);
		p = properties.get(selected_agent, sequence_script);
		if (p && !strcmp(p->label, AIN_SEQUENCE_NAME)) {
			break;
		}
	}

	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], AGENT_SCRIPTING_SCRIPT_ITEM_NAME)) {
				sequence = indigo_get_text_item_value(&p->items[i]);
				break;
			}
		}
		m_sequence_editor2->loadScriptToView(sequence);
	}
	on_recalculate_exposure();
}

void ImagerWindow::on_sequence_start_stop(bool clicked) {
	m_sequence_editor2->enable(true);
	exposure_start_stop(clicked, true);
}

void ImagerWindow::on_sequence_pause(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);

		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_scripting_agent(selected_agent);

		indigo_property *p = properties.get(selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME);
		if (p == nullptr || p->count < 1) return;

		bool paused = false;
		for (int i = 0; i < p->count; i++) {
			if (client_match_item(&p->items[i], AGENT_PAUSE_PROCESS_WAIT_ITEM_NAME)) {
				paused = p->items[i].sw.value;
				break;
			}
		}
		change_agent_pause_process_property(selected_agent, true, !paused);
	});
}

void ImagerWindow::on_reset(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);

		static char selected_scripting_agent[INDIGO_NAME_SIZE];
		get_selected_scripting_agent(selected_scripting_agent);


		indigo_property *p = properties.get(selected_scripting_agent, "SEQUENCE_STATE");
		if (p == nullptr || p->state == INDIGO_BUSY_STATE) return;

		m_sequence_editor2->enable(true);

		indigo_change_switch_property_1(nullptr, selected_scripting_agent, "SEQUENCE_RESET", "RESET", true);
	});
}
