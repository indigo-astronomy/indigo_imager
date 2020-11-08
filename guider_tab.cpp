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
	guider_frame_layout->addWidget(m_agent_guider_select, row, 0, 1, 2);
	connect(m_agent_guider_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_agent_selected);

	// camera selection
	row++;
	QLabel *label = new QLabel("Camera:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	guider_frame_layout->addWidget(label, row, 0);
	m_guider_camera_select = new QComboBox();
	guider_frame_layout->addWidget(m_guider_camera_select, row, 1);
	connect(m_guider_camera_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_camera_selected);

	// Filter wheel selection
	row++;
	label = new QLabel("Guider:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	guider_frame_layout->addWidget(label, row, 0);
	m_guider_select = new QComboBox();
	guider_frame_layout->addWidget(m_guider_select, row, 1);
	connect(m_guider_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_selected);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	guider_frame_layout->addItem(spacer, row, 0);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	guider_frame_layout->addWidget(toolbar, row, 0, 1, 2);

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
	// Tools tabbar
	QTabWidget *guider_tabbar = new QTabWidget;
	guider_frame_layout->addWidget(guider_tabbar, row, 0, 1, 2);

	QFrame *stats_frame = new QFrame();
	guider_tabbar->addTab(stats_frame, "Statistics");

	QGridLayout *stats_frame_layout = new QGridLayout();
	stats_frame_layout->setAlignment(Qt::AlignTop);
	stats_frame->setLayout(stats_frame_layout);
	stats_frame->setFrameShape(QFrame::StyledPanel);
	//stats_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	stats_frame->setContentsMargins(0, 0, 0, 0);

	int stats_row = 0;
	label = new QLabel("Drift Graph RA / Dec (px):");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(label, stats_row, 0, 1, 2);

	stats_row++;
	m_guider_graph = new FocusGraph();
	//m_guider_graph->redraw_data(m_focus_fwhm_data);
	m_guider_graph->set_yaxis_range(-6, 6);
	m_guider_graph->setMinimumHeight(250);
	stats_frame_layout->addWidget(m_guider_graph, stats_row, 0, 1, 2);

	stats_row++;
	label = new QLabel("Drift RA / Dec (px):");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_rd_drift_label = new QLabel();
	m_guider_rd_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_rd_drift_label, stats_row, 1);

	stats_row++;
	label = new QLabel("Drift X / Y (px):");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_xy_drift_label = new QLabel();
	m_guider_xy_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_xy_drift_label, stats_row, 1);

	stats_row++;
	label = new QLabel("Correction RA / Dec (s):");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_pulse_label = new QLabel();
	m_guider_pulse_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_pulse_label, stats_row, 1);

	stats_row++;
	label = new QLabel("RMSE RA / Dec (px):");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_rmse_label = new QLabel();
	m_guider_rmse_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_rmse_label, stats_row, 1);

	QFrame *settings_frame = new QFrame;
	guider_tabbar->addTab(settings_frame, "Settings");

	QGridLayout *settings_frame_layout = new QGridLayout();
	settings_frame_layout->setAlignment(Qt::AlignTop);
	settings_frame->setLayout(settings_frame_layout);
	settings_frame->setFrameShape(QFrame::StyledPanel);
	settings_frame->setContentsMargins(0, 0, 0, 0);

	int settings_row = -1;
	// exposure time
	settings_row++;
	label = new QLabel("Exposure (s):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guider_exposure = new QDoubleSpinBox();
	m_guider_exposure->setMaximum(100000);
	m_guider_exposure->setMinimum(0);
	m_guider_exposure->setValue(0);
	settings_frame_layout->addWidget(m_guider_exposure, settings_row, 3);
	connect(m_guider_exposure, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_exposure_changed);

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Guiding Algorythm:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	// Drift detection
	settings_row++;
	label = new QLabel("Drift Detection:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 2);
	m_detection_mode_select = new QComboBox();
	settings_frame_layout->addWidget(m_detection_mode_select, settings_row, 2, 1, 2);
	connect(m_detection_mode_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_detection_mode_selected);

	settings_row++;
	label = new QLabel("Dec Guiding:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 2);
	m_dec_guiding_select = new QComboBox();
	settings_frame_layout->addWidget(m_dec_guiding_select, settings_row, 2, 1, 2);
	connect(m_dec_guiding_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_dec_guiding_selected);

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Selection:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	// Star Selection
	settings_row++;
	label = new QLabel("Star selection X:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guide_star_x = new QSpinBox();
	m_guide_star_x->setMaximum(100000);
	m_guide_star_x->setMinimum(0);
	m_guide_star_x->setValue(0);
	settings_frame_layout->addWidget(m_guide_star_x , settings_row, 3);
	connect(m_guide_star_x, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	settings_row++;
	label = new QLabel("Star selection Y:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guide_star_y = new QSpinBox();
	m_guide_star_y->setMaximum(100000);
	m_guide_star_y->setMinimum(0);
	m_guide_star_y->setValue(0);
	settings_frame_layout->addWidget(m_guide_star_y, settings_row, 3);
	connect(m_guide_star_y, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	settings_row++;
	label = new QLabel("Selection radius (px):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guide_star_radius = new QSpinBox();
	m_guide_star_radius->setMaximum(100000);
	m_guide_star_radius->setMinimum(0);
	m_guide_star_radius->setValue(0);
	settings_frame_layout->addWidget(m_guide_star_radius, settings_row, 3);
	connect(m_guide_star_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	QFrame *advanced_frame = new QFrame;
	guider_tabbar->addTab(advanced_frame, "Advanced");

	QGridLayout *advanced_frame_layout = new QGridLayout();
	advanced_frame_layout->setAlignment(Qt::AlignTop);
	advanced_frame->setLayout(advanced_frame_layout);
	advanced_frame->setFrameShape(QFrame::StyledPanel);
	//stats_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	advanced_frame->setContentsMargins(0, 0, 0, 0);

	// Guiding pulse
	int advanced_row=0;
	label = new QLabel("Correction limits:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 4);

	advanced_row++;
	label = new QLabel("Min correction error (px):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_min_error = new QDoubleSpinBox();
	m_guide_min_error->setMaximum(100000);
	m_guide_min_error->setMinimum(0);
	m_guide_min_error->setValue(0);
	advanced_frame_layout->addWidget(m_guide_min_error, advanced_row, 3);
	connect(m_guide_min_error, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_pulse_changed);


	advanced_row++;
	label = new QLabel("Min guide pulse (s):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_min_pulse = new QDoubleSpinBox();
	m_guide_min_pulse->setMaximum(100000);
	m_guide_min_pulse->setMinimum(0);
	m_guide_min_pulse->setValue(0);
	advanced_frame_layout->addWidget(m_guide_min_pulse, advanced_row, 3);
	connect(m_guide_min_pulse, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_pulse_changed);

	advanced_row++;
	label = new QLabel("Max guide pulse (s):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_max_pulse = new QDoubleSpinBox();
	m_guide_max_pulse->setMaximum(100000);
	m_guide_max_pulse->setMinimum(0);
	m_guide_max_pulse->setValue(0);
	advanced_frame_layout->addWidget(m_guide_max_pulse, advanced_row, 3);
	connect(m_guide_max_pulse, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_pulse_changed);

	advanced_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	advanced_frame_layout->addItem(spacer, advanced_row, 0);

	advanced_row++;
	label = new QLabel("Proportional-Integral Controller:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 4);

	advanced_row++;
	label = new QLabel("RA Aggressivity (%):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_ra_aggr = new QSpinBox();
	m_guide_ra_aggr->setMaximum(100);
	m_guide_ra_aggr->setMinimum(0);
	m_guide_ra_aggr->setValue(0);
	advanced_frame_layout->addWidget(m_guide_ra_aggr, advanced_row, 3);
	connect(m_guide_ra_aggr, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_aggressivity_changed);

	advanced_row++;
	label = new QLabel("Dec Aggressivity (%):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_dec_aggr = new QSpinBox();
	m_guide_dec_aggr->setMaximum(100);
	m_guide_dec_aggr->setMinimum(0);
	m_guide_dec_aggr->setValue(0);
	advanced_frame_layout->addWidget(m_guide_dec_aggr, advanced_row, 3);
	connect(m_guide_dec_aggr, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_aggressivity_changed);

	advanced_row++;
	label = new QLabel("RA Proportional weight:");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_ra_pw = new QDoubleSpinBox();
	m_guide_ra_pw->setMaximum(1);
	m_guide_ra_pw->setMinimum(0);
	m_guide_ra_pw->setValue(0);
	advanced_frame_layout->addWidget(m_guide_ra_pw, advanced_row, 3);
	connect(m_guide_ra_pw, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_change_guider_agent_pw_changed);

	advanced_row++;
	label = new QLabel("Dec Proportional weight:");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_dec_pw = new QDoubleSpinBox();
	m_guide_dec_pw->setMaximum(1);
	m_guide_dec_pw->setMinimum(0);
	m_guide_dec_pw->setValue(0);
	advanced_frame_layout->addWidget(m_guide_dec_pw, advanced_row, 3);
	connect(m_guide_dec_pw, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_change_guider_agent_pw_changed);

	advanced_row++;
	label = new QLabel("Integral stack (frames):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_is = new QSpinBox();
	m_guide_is->setMaximum(20);
	m_guide_is->setMinimum(0);
	m_guide_is->setValue(0);
	advanced_frame_layout->addWidget(m_guide_is, advanced_row, 3);
	connect(m_guide_is, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_change_guider_agent_is_changed);
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

	//m_guider_viewer->moveSelection(x, y);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_star_selection(selected_agent);
	});
}

void ImagerWindow::on_guider_image_right_click(int x, int y) {
	m_guide_star_x->blockSignals(true);
	m_guide_star_x->setValue(x);
	m_guide_star_x->blockSignals(false);
	m_guide_star_y->blockSignals(true);
	m_guide_star_y->setValue(y);
	m_guide_star_y->blockSignals(false);

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

void ImagerWindow::on_detection_mode_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_detection_mode_property(selected_agent);
	});
}

void ImagerWindow::on_dec_guiding_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_dec_guiding_property(selected_agent);
	});
}

void ImagerWindow::on_guider_agent_exposure_changed(double value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_exposure(selected_agent);
	});
}


void ImagerWindow::on_guider_agent_pulse_changed(double value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_pulse_min_max(selected_agent);
	});
}

void ImagerWindow::on_guider_agent_aggressivity_changed(int value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_aggressivity(selected_agent);
	});
}

void ImagerWindow::on_change_guider_agent_pw_changed(double value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_pi(selected_agent);
	});
}

void ImagerWindow::on_change_guider_agent_is_changed(int value) {
	on_change_guider_agent_pw_changed((double)value);
}
