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

void write_conf();

void ImagerWindow::create_focuser_tab(QFrame *focuser_frame) {
	QSpacerItem *spacer;

	QGridLayout *focuser_frame_layout = new QGridLayout();
	focuser_frame_layout->setAlignment(Qt::AlignTop);
	focuser_frame->setLayout(focuser_frame_layout);
	focuser_frame->setFrameShape(QFrame::StyledPanel);
	focuser_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	focuser_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	// Focuser selection
	row++;
	QLabel *label = new QLabel("Focuser:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(label, row, 0);
	m_focuser_select = new QComboBox();
	focuser_frame_layout->addWidget(m_focuser_select, row, 1, 1, 3);
	connect(m_focuser_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_selected);

	row++;
	label = new QLabel("Focuser control:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(label, row, 0, 1, 4);

	row++;
	label = new QLabel("Absoute Position:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 2);
	m_focus_position = new QSpinBox();
	m_focus_position->setMaximum(1000000);
	m_focus_position->setMinimum(-1000000);
	m_focus_position->setValue(0);
	m_focus_position->setEnabled(false);
	m_focus_position->setKeyboardTracking(false);
	focuser_frame_layout->addWidget(m_focus_position, row, 2, 1, 2);
	connect(m_focus_position, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_focuser_position_changed);

	row++;
	label = new QLabel("Move:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 2);
	m_focus_steps = new QSpinBox();
	m_focus_steps->setMaximum(100000);
	m_focus_steps->setMinimum(0);
	m_focus_steps->setValue(0);
	m_focus_steps->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_steps, row, 2, 1, 2);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	focuser_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	m_focusing_preview_button = new QPushButton("Preview");
	m_focusing_preview_button->setStyleSheet("min-width: 30px");
	m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(m_focusing_preview_button);
	connect(m_focusing_preview_button, &QPushButton::clicked, this, &ImagerWindow::on_focus_preview_start_stop);

	m_focusing_button = new QPushButton("Focus");
	m_focusing_button->setStyleSheet("min-width: 30px");
	m_focusing_button->setIcon(QIcon(":resource/focus.png"));
	toolbox->addWidget(m_focusing_button);
	connect(m_focusing_button, &QPushButton::clicked, this, &ImagerWindow::on_focus_start_stop);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);

	QPushButton *but = new QPushButton("In");
	but->setStyleSheet("min-width: 15px");
	//but->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(but);
	connect(but, &QPushButton::clicked, this, &ImagerWindow::on_focus_in);

	but = new QPushButton("Out");
	but->setStyleSheet("min-width: 15px");
	//but->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(but);
	connect(but, &QPushButton::clicked, this, &ImagerWindow::on_focus_out);

	row++;
	m_focusing_progress = new QProgressBar();
	focuser_frame_layout->addWidget(m_focusing_progress, row, 0, 1, 4);
	m_focusing_progress->setMaximum(1);
	m_focusing_progress->setValue(0);
	m_focusing_progress->setFormat("Focusing: Idle");

	row++;
	spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);

	row++;
	// Tools tabbar
	QTabWidget *focuser_tabbar = new QTabWidget;
	focuser_frame_layout->addWidget(focuser_tabbar, row, 0, 1, 4);

	QFrame *stats_frame = new QFrame();
	focuser_tabbar->addTab(stats_frame, "Statistics");

	QGridLayout *stats_frame_layout = new QGridLayout();
	stats_frame_layout->setAlignment(Qt::AlignTop);
	stats_frame->setLayout(stats_frame_layout);
	stats_frame->setFrameShape(QFrame::StyledPanel);
	//stats_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	stats_frame->setContentsMargins(0, 0, 0, 0);

	int stats_row = 0;
	label = new QLabel("Focus statistics:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(label, stats_row, 0, 1, 4);

	stats_row++;
	m_focus_graph = new FocusGraph();
	m_focus_graph->redraw_data(m_focus_fwhm_data);
	m_focus_graph->setMinimumHeight(150);
	stats_frame_layout->addWidget(m_focus_graph, stats_row, 0, 1, 4);

	stats_row++;
	label = new QLabel("FWHM:");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_FWHM_label = new QLabel();
	m_FWHM_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_FWHM_label, stats_row, 1);

	label = new QLabel("HFD:");
	stats_frame_layout->addWidget(label, stats_row, 2);
	m_HFD_label = new QLabel();
	m_HFD_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_HFD_label, stats_row, 3);

	stats_row++;
	label = new QLabel("Drift (X, Y):");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_drift_label = new QLabel();
	m_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_drift_label, stats_row, 1);

	label = new QLabel("Peak:");
	stats_frame_layout->addWidget(label, stats_row, 2);
	m_peak_label = new QLabel();
	m_peak_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_peak_label, stats_row, 3);

	// Settings tab
	QFrame *settings_frame = new QFrame;
	focuser_tabbar->addTab(settings_frame, "Settings");

	QGridLayout *settings_frame_layout = new QGridLayout();
	settings_frame_layout->setAlignment(Qt::AlignTop);
	settings_frame->setLayout(settings_frame_layout);
	settings_frame->setFrameShape(QFrame::StyledPanel);
	settings_frame->setContentsMargins(0, 0, 0, 0);

	int settings_row = -1;

	// Exposure time
	settings_row++;
	label = new QLabel("Exposure time (s):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 2);
	m_focuser_exposure_time = new QDoubleSpinBox();
	m_focuser_exposure_time->setMaximum(10000);
	m_focuser_exposure_time->setMinimum(0);
	m_focuser_exposure_time->setValue(1);
	settings_frame_layout->addWidget(m_focuser_exposure_time, settings_row, 2, 1, 2);

	settings_row++;
	label = new QLabel("Focus mode:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 2);
	m_focus_mode_select = new QComboBox();
	m_focus_mode_select->addItem("Manual");
	m_focus_mode_select->addItem("Auto");
	settings_frame_layout->addWidget(m_focus_mode_select, settings_row, 2, 1, 2);
	m_focus_mode_select->setCurrentIndex(conf.focus_mode);
	connect(m_focus_mode_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focus_mode_selected);

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Selection:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	// Star Selection
	settings_row++;
	label = new QLabel("Star selection X / Y:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 2);
	m_star_x = new QDoubleSpinBox();
	m_star_x->setMaximum(100000);
	m_star_x->setMinimum(0);
	m_star_x->setValue(0);
	//m_star_x->setEnabled(false);
	settings_frame_layout->addWidget(m_star_x , settings_row, 2);
	connect(m_star_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_selection_changed);

	m_star_y = new QDoubleSpinBox();
	m_star_y->setMaximum(100000);
	m_star_y->setMinimum(0);
	m_star_y->setValue(0);
	//m_star_y->setEnabled(false);
	settings_frame_layout->addWidget(m_star_y, settings_row, 3);
	connect(m_star_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_selection_changed);

	settings_row++;
	label = new QLabel("Selection radius (px):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_focus_star_radius = new QSpinBox();
	m_focus_star_radius->setMaximum(100000);
	m_focus_star_radius->setMinimum(0);
	m_focus_star_radius->setValue(0);
	settings_frame_layout->addWidget(m_focus_star_radius, settings_row, 3);
	connect(m_focus_star_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_focuser_selection_radius_changed);

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Autofocus settings:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	settings_row++;
	label = new QLabel("Initial Step:");
	settings_frame_layout->addWidget(label, settings_row, 0);
	m_initial_step = new QSpinBox();
	m_initial_step->setMaximum(1000000);
	m_initial_step->setMinimum(0);
	m_initial_step->setValue(0);
	//m_initial_step->setEnabled(false);
	settings_frame_layout->addWidget(m_initial_step , settings_row, 1);

	label = new QLabel("Final step:");
	settings_frame_layout->addWidget(label, settings_row, 2);
	m_final_step = new QSpinBox();
	m_final_step->setMaximum(100000);
	m_final_step->setMinimum(0);
	m_final_step->setValue(0);
	//m_final_step->setEnabled(false);
	settings_frame_layout->addWidget(m_final_step, settings_row, 3);

	settings_row++;
	label = new QLabel("Backlash:");
	settings_frame_layout->addWidget(label, settings_row, 0);
	m_focus_backlash = new QSpinBox();
	m_focus_backlash->setMaximum(1000000);
	m_focus_backlash->setMinimum(0);
	m_focus_backlash->setValue(0);
	//m_focus_backlash->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_backlash, settings_row, 1);

	label = new QLabel("Stacking:");
	settings_frame_layout->addWidget(label, settings_row, 2);
	m_focus_stack = new QSpinBox();
	m_focus_stack->setMaximum(100000);
	m_focus_stack->setMinimum(0);
	m_focus_stack->setValue(0);
	//m_focus_stack->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_stack, settings_row, 3);

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Misc settings:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	settings_row++;
	label = new QLabel("Use subframe:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1 ,2);
	m_focuser_subframe_select = new QComboBox();
	m_focuser_subframe_select->addItem("Off");
	m_focuser_subframe_select->addItem("10 radii");
	m_focuser_subframe_select->addItem("20 radii");
	settings_frame_layout->addWidget(m_focuser_subframe_select, settings_row, 2, 1, 2);
	m_focuser_subframe_select->setCurrentIndex(conf.focuser_subframe);
	connect(m_focuser_subframe_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_subframe_changed);
}

void ImagerWindow::on_focuser_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_focuser[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_focuser_str = m_focuser_select->currentText();
		int idx = q_focuser_str.indexOf(" @ ");
		if (idx >=0) q_focuser_str.truncate(idx);
		if (q_focuser_str.compare("No focuser") == 0) {
			strcpy(selected_focuser, "NONE");
		} else {
			strncpy(selected_focuser, q_focuser_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_focuser);
		static const char * items[] = { selected_focuser };
		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_FOCUSER_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_selection_changed(double value) {
	int x = m_star_x->value();
	int y = m_star_y->value();
	m_imager_viewer->moveSelection(x, y);
	m_HFD_label->setText("n/a");
	m_FWHM_label->setText("n/a");
	m_peak_label->setText("n/a");
	m_drift_label->setText("n/a");
}

void ImagerWindow::on_focuser_selection_radius_changed(int value) {
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_star_selection(selected_agent);
	});
}

void ImagerWindow::on_focus_mode_selected(int index) {
	conf.focus_mode = index;
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_image_right_click(double x, double y) {
	m_star_x->blockSignals(true);
	m_star_x->setValue(x);
	m_star_x->blockSignals(false);
	m_star_y->blockSignals(true);
	m_star_y->setValue(y);
	m_star_y->blockSignals(false);

	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_star_selection(selected_agent);
	});
}


void ImagerWindow::on_focuser_position_changed(int value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_focuser_position_property(selected_agent);
	});
}


void ImagerWindow::on_focus_preview_start_stop(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		indigo_property *ccd_exposure = properties.get(selected_agent, CCD_EXPOSURE_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state != INDIGO_BUSY_STATE &&
			ccd_exposure && ccd_exposure->state == INDIGO_BUSY_STATE) {
			change_ccd_abort_exposure_property(selected_agent);
		} else {
			m_save_blob = false;
			change_ccd_frame_property(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
			change_ccd_exposure_property(selected_agent, m_focuser_exposure_time);
		}
	});
}


void ImagerWindow::on_focus_start_stop(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
			change_agent_abort_process_property(selected_agent);
		} else {
			m_save_blob = false;
			m_focus_fwhm_data.clear();
			//m_focus_graph->redraw_data(m_focus_fwhm_data);
			change_agent_star_selection(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
			change_agent_batch_property_for_focus(selected_agent);
			change_agent_focus_params_property(selected_agent);
			change_ccd_frame_property(selected_agent);
			if(m_focus_mode_select->currentIndex() == 0) {
				change_agent_start_preview_property(selected_agent);
			} else {
				change_agent_start_focusing_property(selected_agent);
			}
		}
	});
}

void ImagerWindow::on_focus_in(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_focuser_focus_in_property(selected_agent);
		change_focuser_steps_property(selected_agent);
	});
}

void ImagerWindow::on_focus_out(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_focuser_focus_out_property(selected_agent);
		change_focuser_steps_property(selected_agent);
	});
}


void ImagerWindow::on_focuser_subframe_changed(int index) {
	conf.focuser_subframe = index;
	write_conf();
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_focuser_subframe(selected_agent);
	});
}
