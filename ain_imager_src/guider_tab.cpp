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

#include <image_preview_lut.h>

void write_conf();

void ImagerWindow::create_guider_tab(QFrame *guider_frame) {
	QGridLayout *guider_frame_layout = new QGridLayout();
	guider_frame_layout->setAlignment(Qt::AlignTop);
	guider_frame_layout->setColumnStretch(0, 1);
	guider_frame_layout->setColumnStretch(1, 3);

	guider_frame->setLayout(guider_frame_layout);
	guider_frame->setFrameShape(QFrame::StyledPanel);
	guider_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	guider_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	m_agent_guider_select = new QComboBox();
	guider_frame_layout->addWidget(m_agent_guider_select, row, 0, 1, 2);
	connect(m_agent_guider_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImagerWindow::on_guider_agent_selected);

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
	m_guider_calibrate_button->setIcon(QIcon(":resource/calibrate.png"));
	toolbox->addWidget(m_guider_calibrate_button);
	connect(m_guider_calibrate_button , &QPushButton::clicked, this, &ImagerWindow::on_guider_calibrate_start_stop);

	m_guider_guide_button = new QPushButton("Guide");
	m_guider_guide_button->setStyleSheet("min-width: 30px");
	m_guider_guide_button->setIcon(QIcon(":resource/guide.png"));
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
	m_guider_graph_label = new QLabel();
	m_guider_graph_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_graph_label, stats_row, 0, 1, 2);

	stats_row++;
	m_guider_graph = new FocusGraph();
	m_guider_graph->set_yaxis_range(-6, 6);
	m_guider_graph->setMinimumHeight(250);
	stats_frame_layout->addWidget(m_guider_graph, stats_row, 0, 1, 2);

	stats_row++;
	label = new QLabel("Drift RA / Dec:");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_rd_drift_label = new QLabel();
	m_guider_rd_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_rd_drift_label, stats_row, 1);

	stats_row++;
	label = new QLabel("Drift X / Y:");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_xy_drift_label = new QLabel();
	m_guider_xy_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_xy_drift_label, stats_row, 1);

	stats_row++;
	label = new QLabel("Correction RA / Dec:");
	stats_frame_layout->addWidget(label, stats_row, 0);
	m_guider_pulse_label = new QLabel();
	m_guider_pulse_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_guider_pulse_label, stats_row, 1);

	stats_row++;
	label = new QLabel("RMSE RA / Dec:");
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

	settings_row++;
	label = new QLabel("Exposure:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);
	// exposure time
	settings_row++;
	label = new QLabel("Exposure time (s):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guider_exposure = new QDoubleSpinBox();
	m_guider_exposure->setDecimals(3);
	m_guider_exposure->setMaximum(100000);
	m_guider_exposure->setMinimum(0);
	m_guider_exposure->setValue(0);
	settings_frame_layout->addWidget(m_guider_exposure, settings_row, 3);
	connect(m_guider_exposure, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_exposure_changed);

	// exposure delay
	settings_row++;
	label = new QLabel("Exposure delay (s):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guider_delay = new QDoubleSpinBox();
	m_guider_delay->setDecimals(3);
	m_guider_delay->setMaximum(100000);
	m_guider_delay->setMinimum(0);
	m_guider_delay->setValue(0);
	settings_frame_layout->addWidget(m_guider_delay, settings_row, 3);
	connect(m_guider_delay, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_exposure_changed);


	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Guiding:");
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

	//settings_row++;
	//label = new QLabel("Selection:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	//settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	// Star Selection
	settings_row++;
	label = new QLabel("Star selection X / Y:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 2);
	m_guide_star_x = new QDoubleSpinBox();
	m_guide_star_x->setMaximum(100000);
	m_guide_star_x->setMinimum(0);
	m_guide_star_x->setValue(0);
	settings_frame_layout->addWidget(m_guide_star_x , settings_row, 2);
	connect(m_guide_star_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	m_guide_star_y = new QDoubleSpinBox();
	m_guide_star_y->setMaximum(100000);
	m_guide_star_y->setMinimum(0);
	m_guide_star_y->setValue(0);
	settings_frame_layout->addWidget(m_guide_star_y, settings_row, 3);
	connect(m_guide_star_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_changed);

	settings_row++;
	label = new QLabel("Selection radius (px):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guide_star_radius = new QSpinBox();
	m_guide_star_radius->setMaximum(100000);
	m_guide_star_radius->setMinimum(0);
	m_guide_star_radius->setValue(0);
	settings_frame_layout->addWidget(m_guide_star_radius, settings_row, 3);
	connect(m_guide_star_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_radius_changed);

	settings_row++;
	label = new QLabel("Star count:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guide_star_count = new QSpinBox();
	m_guide_star_count->setMaximum(100);
	m_guide_star_count->setMinimum(1);
	m_guide_star_count->setValue(1);
	settings_frame_layout->addWidget(m_guide_star_count, settings_row, 3);
	connect(m_guide_star_count, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_selection_star_count_changed);

	settings_row++;
	button = new QPushButton("Clear star selection");
	button->setStyleSheet("min-width: 30px");
	button->setToolTip("Keyboard shortcut: Ctrl+Backspace");
	settings_frame_layout->addWidget(button, settings_row, 0, 1, 4);

	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_guider_clear_selection);
	QShortcut *shortcut = new QShortcut(QKeySequence("Ctrl+Backspace"), this);
	connect(shortcut, &QShortcut::activated, this, [this](){this->on_guider_clear_selection(true);});

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Edge Clipping (Donuts) (px):");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_guide_edge_clipping = new QSpinBox();
	m_guide_edge_clipping->setMaximum(100000);
	m_guide_edge_clipping->setMinimum(0);
	m_guide_edge_clipping->setValue(0);
	m_guide_edge_clipping->setEnabled(false);
	settings_frame_layout->addWidget(m_guide_edge_clipping, settings_row, 3);
	connect(m_guide_edge_clipping, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_edge_clipping_changed);

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
	label = new QLabel("Min/Max pulse (s):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 2);
	m_guide_min_pulse = new QDoubleSpinBox();
	m_guide_min_pulse->setDecimals(3);
	m_guide_min_pulse->setMaximum(100000);
	m_guide_min_pulse->setMinimum(0);
	m_guide_min_pulse->setValue(0);
	advanced_frame_layout->addWidget(m_guide_min_pulse, advanced_row, 2);
	connect(m_guide_min_pulse, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_pulse_changed);

	m_guide_max_pulse = new QDoubleSpinBox();
	m_guide_max_pulse->setDecimals(3);
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
	label = new QLabel("RA/Dec P Aggressiv. (%):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 2);
	m_guide_ra_aggr = new QSpinBox();
	m_guide_ra_aggr->setMaximum(100);
	m_guide_ra_aggr->setMinimum(0);
	m_guide_ra_aggr->setValue(0);
	advanced_frame_layout->addWidget(m_guide_ra_aggr, advanced_row, 2);
	connect(m_guide_ra_aggr, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_aggressivity_changed);

	m_guide_dec_aggr = new QSpinBox();
	m_guide_dec_aggr->setMaximum(100);
	m_guide_dec_aggr->setMinimum(0);
	m_guide_dec_aggr->setValue(0);
	advanced_frame_layout->addWidget(m_guide_dec_aggr, advanced_row, 3);
	connect(m_guide_dec_aggr, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_aggressivity_changed);

	advanced_row++;
	label = new QLabel("RA/Dec I gain:");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 2);
	m_guide_i_gain_ra = new QDoubleSpinBox();
	m_guide_i_gain_ra->setMaximum(1);
	m_guide_i_gain_ra->setMinimum(0);
	m_guide_i_gain_ra->setValue(0);
	advanced_frame_layout->addWidget(m_guide_i_gain_ra, advanced_row, 2);
	connect(m_guide_i_gain_ra, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_change_guider_agent_i_gain_changed);

	m_guide_i_gain_dec = new QDoubleSpinBox();
	m_guide_i_gain_dec->setMaximum(1);
	m_guide_i_gain_dec->setMinimum(0);
	m_guide_i_gain_dec->setValue(0);
	advanced_frame_layout->addWidget(m_guide_i_gain_dec, advanced_row, 3);
	connect(m_guide_i_gain_dec, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_change_guider_agent_i_gain_changed);

	advanced_row++;
	label = new QLabel("I stack (frames):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_is = new QSpinBox();
	m_guide_is->setMaximum(20);
	m_guide_is->setMinimum(0);
	m_guide_is->setValue(0);
	advanced_frame_layout->addWidget(m_guide_is, advanced_row, 3);
	connect(m_guide_is, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_change_guider_agent_is_changed);

	advanced_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	advanced_frame_layout->addItem(spacer, advanced_row, 0);

	advanced_row++;
	label = new QLabel("Calibration:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 4);

	advanced_row++;
	label = new QLabel("Calibration step (s):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_cal_step = new QDoubleSpinBox();
	m_guide_cal_step->setMaximum(1);
	m_guide_cal_step->setMinimum(0);
	m_guide_cal_step->setValue(0);
	advanced_frame_layout->addWidget(m_guide_cal_step, advanced_row, 3);
	connect(m_guide_cal_step, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_callibration_changed);

	advanced_row++;
	label = new QLabel("Dec Backlash (px):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 2);

	m_guider_apply_backlash_cbox = new QCheckBox("");
	m_guider_apply_backlash_cbox->setToolTip("Compensate declination backlash while guiding");
	m_guider_apply_backlash_cbox->setEnabled(false);
	advanced_frame_layout->addWidget(m_guider_apply_backlash_cbox, advanced_row, 2, Qt::AlignRight);
	connect(m_guider_apply_backlash_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_guider_apply_backlash_changed);

	m_guide_dec_backlash = new QDoubleSpinBox();
	m_guide_dec_backlash->setMaximum(1);
	m_guide_dec_backlash->setMinimum(0);
	m_guide_dec_backlash->setValue(0);
	advanced_frame_layout->addWidget(m_guide_dec_backlash, advanced_row, 3);
	connect(m_guide_dec_backlash, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_callibration_changed);

	advanced_row++;
	label = new QLabel("Axis rotation angle (°):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 3);
	m_guide_rotation = new QDoubleSpinBox();
	m_guide_rotation->setMaximum(1);
	m_guide_rotation->setMinimum(0);
	m_guide_rotation->setValue(0);
	advanced_frame_layout->addWidget(m_guide_rotation, advanced_row, 3);
	connect(m_guide_rotation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_callibration_changed);

	advanced_row++;
	label = new QLabel("RA/Dec speed (px/s):");
	advanced_frame_layout->addWidget(label, advanced_row, 0, 1, 2);
	m_guide_ra_speed = new QDoubleSpinBox();
	m_guide_ra_speed->setMaximum(1);
	m_guide_ra_speed->setMinimum(0);
	m_guide_ra_speed->setValue(0);
	advanced_frame_layout->addWidget(m_guide_ra_speed, advanced_row, 2);
	connect(m_guide_ra_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_callibration_changed);

	m_guide_dec_speed = new QDoubleSpinBox();
	m_guide_dec_speed->setMaximum(1);
	m_guide_dec_speed->setMinimum(0);
	m_guide_dec_speed->setValue(0);
	advanced_frame_layout->addWidget(m_guide_dec_speed, advanced_row, 3);
	connect(m_guide_dec_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_guider_agent_callibration_changed);

	QFrame *misc_frame = new QFrame;
	guider_tabbar->addTab(misc_frame, "Misc");

	QGridLayout *misc_frame_layout = new QGridLayout();
	misc_frame_layout->setAlignment(Qt::AlignTop);
	misc_frame->setLayout(misc_frame_layout);
	misc_frame->setFrameShape(QFrame::StyledPanel);
	misc_frame->setContentsMargins(0, 0, 0, 0);
	misc_frame_layout->setColumnStretch(0, 3);
	misc_frame_layout->setColumnStretch(1, 1);
	misc_frame_layout->setColumnStretch(2, 1);
	misc_frame_layout->setColumnStretch(3, 1);

	int misc_row = 0;
	label = new QLabel("Save bandwidth:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	label = new QLabel("Use JPEG:");
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 2);
	m_guider_save_bw_select = new QComboBox();
	m_guider_save_bw_select->addItem("Off");
	m_guider_save_bw_select->addItem("Fine");
	m_guider_save_bw_select->addItem("Normal");
	m_guider_save_bw_select->addItem("Coarse");
	misc_frame_layout->addWidget(m_guider_save_bw_select, misc_row, 2, 1, 2);
	m_guider_save_bw_select->setCurrentIndex(conf.guider_save_bandwidth);
	connect(m_guider_save_bw_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_bw_save_changed);

	misc_row++;
	label = new QLabel("Use subframe:");
	misc_frame_layout->addWidget(label, misc_row, 0, 1 ,2);
	m_guider_subframe_select = new QComboBox();
	m_guider_subframe_select->addItem("Off");
	m_guider_subframe_select->addItem("10 radii");
	m_guider_subframe_select->addItem("20 radii");
	misc_frame_layout->addWidget(m_guider_subframe_select, misc_row, 2, 1, 2);
	m_guider_subframe_select->setCurrentIndex(conf.guider_subframe);
	connect(m_guider_subframe_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_subframe_changed);

	misc_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	misc_frame_layout->addItem(spacer, misc_row, 0);

	misc_row++;
	label = new QLabel("Camera Settings:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	label = new QLabel("Frame:");
	misc_frame_layout->addWidget(label, misc_row, 0);
	m_guider_frame_size_select = new QComboBox();
	misc_frame_layout->addWidget(m_guider_frame_size_select, misc_row, 1, 1, 3);
	connect(m_guider_frame_size_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_guider_ccd_mode_selected);

	misc_row++;
	label = new QLabel("Gain:");
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 2);
	m_guider_gain = new QSpinBox();
	m_guider_gain->setEnabled(false);
	misc_frame_layout->addWidget(m_guider_gain, misc_row, 2, 1, 2);
	connect(m_guider_gain, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_guider_gain_changed);

	misc_row++;
	label = new QLabel("Offset:");
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 2);
	m_guider_offset = new QSpinBox();
	m_guider_offset->setEnabled(false);
	misc_frame_layout->addWidget(m_guider_offset, misc_row, 2, 1, 2);
	connect(m_guider_offset, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_guider_offset_changed);

	misc_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	misc_frame_layout->addItem(spacer, misc_row, 0);

	misc_row++;
	label = new QLabel("Guider Scope Profile:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	label = new QLabel("Focal length (cm):");
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 2);
	m_guider_focal_lenght = new QDoubleSpinBox();
	m_guider_focal_lenght->setEnabled(false);
	m_guider_focal_lenght->setSpecialValueText(" ");
	misc_frame_layout->addWidget(m_guider_focal_lenght, misc_row, 2, 1, 2);
	connect(m_guider_focal_lenght, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_agent_guider_focal_length_changed);
}

void ImagerWindow::setup_preview(const char *agent) {
	change_agent_ccd_peview(agent, (bool)conf.guider_save_bandwidth);
	switch (conf.guider_save_bandwidth) {
	case 0:
		break;
	case 1:
		change_jpeg_settings_property(agent, 93, stretch_linear_lut[conf.guider_stretch_level].clip_black, stretch_linear_lut[conf.guider_stretch_level].clip_white);
		break;
	case 2:
		change_jpeg_settings_property(agent, 89, stretch_linear_lut[conf.guider_stretch_level].clip_black, stretch_linear_lut[conf.guider_stretch_level].clip_white);
		break;
	case 3:
		change_jpeg_settings_property(agent, 50, stretch_linear_lut[conf.guider_stretch_level].clip_black, stretch_linear_lut[conf.guider_stretch_level].clip_white);
		break;
	default:
		break;
	}
}

void ImagerWindow::select_guider_data(guider_display_data show) {
	switch (show) {
		case SHOW_RA_DEC_DRIFT:
			m_guider_data_1 = &m_drift_data_ra;
			m_guider_data_2 = &m_drift_data_dec;
			m_guider_graph->set_yaxis_range(-6, 6);
			m_guider_graph_label->setText("Drift <font color=\"red\">RA</font> / <font color=\"#03acf0\">Dec</font> (px):");
			break;
		case SHOW_RA_DEC_S_DRIFT:
			m_guider_data_1 = &m_drift_data_ra_s;
			m_guider_data_2 = &m_drift_data_dec_s;
			m_guider_graph->set_yaxis_range(-6, 6);
			m_guider_graph_label->setText("Drift <font color=\"red\">RA</font> / <font color=\"#03acf0\">Dec</font> (arcsec):");
			break;
		case SHOW_RA_DEC_PULSE:
			m_guider_data_1 = &m_pulse_data_ra;
			m_guider_data_2 = &m_pulse_data_dec;
			m_guider_graph->set_yaxis_range(-1.5, 1.5);
			m_guider_graph_label->setText("Guiding Pulses <font color=\"red\">RA</font> / <font color=\"#03acf0\">Dec</font> (s):");
			break;
		case SHOW_X_Y_DRIFT:
			m_guider_data_1 = &m_drift_data_x;
			m_guider_data_2 = &m_drift_data_y;
			m_guider_graph->set_yaxis_range(-6, 6);
			m_guider_graph_label->setText("Drift <font color=\"red\">X</font> / <font color=\"#03acf0\">Y</font> (px):");
			break;
		default:
			m_guider_data_1 = nullptr;
			m_guider_data_2 = nullptr;
	}
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

void ImagerWindow::on_guider_selection_changed(double value) {
	//int x = m_guide_star_x->value();
	//int y = m_guide_star_y->value();
	//m_guider_viewer->moveSelection(x, y);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_star_selection(selected_agent);
	});
}

void ImagerWindow::on_guider_selection_radius_changed(int value) {
	on_guider_selection_changed((double)value);
}

void ImagerWindow::on_guider_selection_star_count_changed(int value) {
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_star_count(selected_agent);
	});
}

void ImagerWindow::on_guider_edge_clipping_changed(int value) {
	//int x = m_guide_star_x->value();
	//int y = m_guide_star_y->value();

	//m_guider_viewer->moveSelection(x, y);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_edge_clipping(selected_agent);
	});
}

void ImagerWindow::on_guider_image_right_click(double x, double y) {
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
			setup_preview(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
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
			setup_preview(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
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
			setup_preview(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
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

void ImagerWindow::on_change_guider_agent_i_gain_changed(double value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_i(selected_agent);
	});
}

void ImagerWindow::on_change_guider_agent_is_changed(int value) {
	on_change_guider_agent_i_gain_changed((double)value);
}

void ImagerWindow::on_guider_agent_callibration_changed(double value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_callibration(selected_agent);
	});
}

void ImagerWindow::on_guider_apply_backlash_changed(int state) {
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_apply_dec_backlash(selected_agent);
	});
}

void ImagerWindow::on_guider_bw_save_changed(int index) {
	conf.guider_save_bandwidth = index;
	write_conf();
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		setup_preview(selected_agent);
	});
}

void ImagerWindow::on_guider_subframe_changed(int index) {
	conf.guider_subframe = index;
	write_conf();
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_agent_subframe(selected_agent);
	});
}

void ImagerWindow::on_guider_clear_selection(bool clicked) {
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);

		clear_guider_agent_star_selection(selected_agent);
	});
}

void ImagerWindow::on_guider_ccd_mode_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[MODE CHANGE SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_ccd_mode_property(selected_agent, m_guider_frame_size_select);
	});
}

void ImagerWindow::on_agent_guider_gain_changed(int value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_gain_property(selected_agent, m_guider_gain);
	});
}

void ImagerWindow::on_agent_guider_offset_changed(int value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_offset_property(selected_agent, m_guider_offset);
	});
}

void ImagerWindow::on_agent_guider_focal_length_changed(int value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_lens_profile_property(selected_agent, m_guider_focal_lenght);
	});
}
