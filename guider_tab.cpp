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

void ImagerWindow::create_guider_tab(QFrame *guider_frame) {
	QGridLayout *guider_frame_layout = new QGridLayout();
	guider_frame_layout->setAlignment(Qt::AlignTop);
	guider_frame->setLayout(guider_frame_layout);
	guider_frame->setFrameShape(QFrame::StyledPanel);
	guider_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	guider_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	m_agent_guider_select = new QComboBox();
	guider_frame_layout->addWidget(m_agent_guider_select, row, 0, 1, 4);
	connect(m_agent_guider_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_agent_selected);

	// camera selection
	row++;
	QLabel *label = new QLabel("Guide camera:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	guider_frame_layout->addWidget(label, row, 0);
	m_guider_camera_select = new QComboBox();
	guider_frame_layout->addWidget(m_guider_camera_select, row, 1, 1, 3);
	connect(m_guider_camera_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_camera_selected);

	// Filter wheel selection
	row++;
	label = new QLabel("Guider:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	guider_frame_layout->addWidget(label, row, 0);
	m_guider_select = new QComboBox();
	guider_frame_layout->addWidget(m_guider_select, row, 1, 1, 3);
	connect(m_guider_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_selected);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	guider_frame_layout->addItem(spacer, row, 0);

	// Star Selection
	row++;
	label = new QLabel("Star X:");
	guider_frame_layout->addWidget(label, row, 0);
	m_guide_star_x = new QSpinBox();
	m_guide_star_x->setMaximum(100000);
	m_guide_star_x->setMinimum(0);
	m_guide_star_x->setValue(0);
	guider_frame_layout->addWidget(m_guide_star_x , row, 1);
	connect(m_guide_star_x, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	label = new QLabel("Star Y:");
	guider_frame_layout->addWidget(label, row, 2);
	m_guide_star_y = new QSpinBox();
	m_guide_star_y->setMaximum(100000);
	m_guide_star_y->setMinimum(0);
	m_guide_star_y->setValue(0);
	guider_frame_layout->addWidget(m_guide_star_y, row, 3);
	connect(m_guide_star_y, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	guider_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	m_guider_preview_button = new QPushButton("Preview");
	m_guider_preview_button->setStyleSheet("min-width: 30px");
	m_guider_preview_button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(m_guider_preview_button);
	connect(m_guider_preview_button, &QPushButton::clicked, this, &ImagerWindow::on_guider_preview_start_stop);

	m_guider_calibrate_button = new QPushButton("Calibrate");
	m_guider_calibrate_button->setStyleSheet("min-width: 30px");
	m_guider_calibrate_button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(m_guider_calibrate_button);
	connect(m_guider_calibrate_button , &QPushButton::clicked, this, &ImagerWindow::on_guider_calibrate_start_stop);

	m_guider_guide_button = new QPushButton("Guide");
	m_guider_guide_button->setStyleSheet("min-width: 30px");
	m_guider_guide_button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(m_guider_guide_button);
	connect(m_guider_guide_button, &QPushButton::clicked, this, &ImagerWindow::on_guider_guide_start_stop);

	QPushButton *button = new QPushButton("Stop");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_guider_stop);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	guider_frame_layout->addItem(spacer, row, 0);

	row++;
	label = new QLabel("Guiding statistics:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	guider_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	m_guider_graph = new FocusGraph();
	//m_guider_graph->redraw_data(m_focus_fwhm_data);
	m_guider_graph->set_yaxis_range(-5, 5);
	m_guider_graph->setMinimumHeight(250);
	guider_frame_layout->addWidget(m_guider_graph, row, 0, 1, 4);

	row++;
	label = new QLabel("Drift (X, Y):");
	guider_frame_layout->addWidget(label, row, 0);
	m_guider_drift_label = new QLabel();
	m_guider_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	guider_frame_layout->addWidget(m_guider_drift_label, row, 1);
}

void ImagerWindow::on_guider_agent_selected(int index) {
	QtConcurrent::run([=]() {
		// Clear controls
		indigo_property *property = (indigo_property*)malloc(sizeof(indigo_property));
		memset(property, 0, sizeof(indigo_property));
		get_selected_guider_agent(property->device);
		property_delete(property, nullptr);
		free(property);

		indigo_enumerate_properties(nullptr, &INDIGO_ALL_PROPERTIES);
	});
}

void ImagerWindow::on_guider_camera_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_camera[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_camera_str = m_guider_camera_select->currentText();
		int idx = q_camera_str.indexOf(" @ ");
		if (idx >=0) q_camera_str.truncate(idx);
		if (q_camera_str.compare("No camera") == 0) {
			strcpy(selected_camera, "NONE");
		} else {
			strncpy(selected_camera, q_camera_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_camera);
		static const char * items[] = { selected_camera };
		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_guider_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_guider[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_guider_str = m_guider_select->currentText();
		int idx = q_guider_str.indexOf(" @ ");
		if (idx >=0) q_guider_str.truncate(idx);
		if (q_guider_str.compare("No guider") == 0) {
			strcpy(selected_guider, "NONE");
		} else {
			strncpy(selected_guider, q_guider_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_guider);
		static const char * items[] = { selected_guider };

		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_GUIDER_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_guider_selection_changed(int value) {
	int x = m_guide_star_x->value();
	int y = m_guide_star_y->value();
	/*
	m_guider_viewer->moveSelection(x, y);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_star_selection(selected_agent);
	});
	*/
	/*
	m_HFD_label->setText("n/a");
	m_FWHM_label->setText("n/a");
	m_peak_label->setText("n/a");
	m_drift_label->setText("n/a");
	*/
}

void ImagerWindow::on_guider_image_right_click(int x, int y) {
	m_guide_star_x->setValue(x);
	m_guide_star_y->setValue(y);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_star_selection(selected_agent);
	});
}


void ImagerWindow::on_guider_preview_start_stop(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
			change_agent_abort_process_property(selected_agent);
		} else {
			// m_guider_graph->redraw_data(m_focus_fwhm_data);
			// change_agent_batch_property_for_focus(selected_agent);
			//change_agent_focus_params_property(selected_agent);
			change_agent_start_preview_property(selected_agent);
		}
	});
}

void ImagerWindow::on_guider_calibrate_start_stop(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
			change_agent_abort_process_property(selected_agent);
		} else {
			// m_guider_graph->redraw_data(m_focus_fwhm_data);
			// change_agent_batch_property_for_focus(selected_agent);
			//change_agent_focus_params_property(selected_agent);
			change_agent_start_calibrate_property(selected_agent);
		}
	});
}


void ImagerWindow::on_guider_guide_start_stop(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
			change_agent_abort_process_property(selected_agent);
		} else {
			// m_guider_graph->redraw_data(m_focus_fwhm_data);
			// change_agent_batch_property_for_focus(selected_agent);
			//change_agent_focus_params_property(selected_agent);
			change_agent_start_guide_property(selected_agent);
		}
	});
}

void ImagerWindow::on_guider_stop(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
			change_agent_abort_process_property(selected_agent);
		}
	});
}
