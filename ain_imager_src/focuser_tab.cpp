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
#include "widget_state.h"

void write_conf();

void ImagerWindow::create_focuser_tab(QFrame *focuser_frame) {
	QSpacerItem *spacer;

	QGridLayout *focuser_frame_layout = new QGridLayout();
	focuser_frame_layout->setAlignment(Qt::AlignTop);
	focuser_frame_layout->setColumnStretch(0, 1);
	focuser_frame_layout->setColumnStretch(1, 2);
	focuser_frame_layout->setColumnStretch(2, 1);
	focuser_frame_layout->setColumnStretch(3, 1);
	focuser_frame_layout->setColumnStretch(4, 1);
	focuser_frame_layout->setColumnStretch(5, 1);

	focuser_frame->setLayout(focuser_frame_layout);
	focuser_frame->setFrameShape(QFrame::StyledPanel);
	focuser_frame->setMinimumWidth(TOOLBAR_MIN_WIDTH);
	focuser_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	// Focuser selection
	row++;
	QLabel *label = new QLabel("Focuser:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	focuser_frame_layout->addWidget(label, row, 0);
	m_focuser_select = new QComboBox();
	focuser_frame_layout->addWidget(m_focuser_select, row, 1, 1, 5);
	connect(m_focuser_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_selected);

	row++;
	label = new QLabel("Absolute position:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 2);

	m_focus_position_label = new QLabel("0");
	set_ok(m_focus_position_label);
	m_focus_position_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	m_focus_position_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	focuser_frame_layout->addWidget(m_focus_position_label, row, 2, 1, 1);

	m_focus_position = new QSpinBox();
	m_focus_position->setMaximum(1000000);
	m_focus_position->setMinimum(-1000000);
	m_focus_position->setValue(0);
	//m_focus_position->setEnabled(false);

	focuser_frame_layout->addWidget(m_focus_position, row, 3, 1, 2);

	m_focus_position_button = new QToolButton(this);
	m_focus_position_button->setToolTip(tr("Go to absolute position / Abort move"));
	m_focus_position_button->setIcon(QIcon(":resource/play.png"));
	focuser_frame_layout->addWidget(m_focus_position_button, row, 5);
	QObject::connect(m_focus_position_button, &QToolButton::clicked, this, &ImagerWindow::on_focuser_position_changed);

	row++;
	label = new QLabel("Relative move:");
	focuser_frame_layout->addWidget(label, row, 0, 1, 3);
	m_focus_steps = new QSpinBox();
	m_focus_steps->setMaximum(100000);
	m_focus_steps->setMinimum(0);
	m_focus_steps->setValue(0);
	m_focus_steps->setEnabled(false);
	focuser_frame_layout->addWidget(m_focus_steps, row, 3);

	m_focusing_out_button = new QToolButton(this);
	m_focusing_out_button->setStyleSheet("min-width: 15px");
	m_focusing_out_button->setIcon(QIcon(":resource/focus_out.png"));
	m_focusing_out_button->setToolTip("Relative move out");
	focuser_frame_layout->addWidget(m_focusing_out_button, row, 4);
	connect(m_focusing_out_button, &QPushButton::clicked, this, &ImagerWindow::on_focus_out);

	m_focusing_in_button = new QToolButton(this);
	m_focusing_in_button->setStyleSheet("min-width: 15px");
	m_focusing_in_button->setIcon(QIcon(":resource/focus_in.png"));
	m_focusing_in_button->setToolTip("Relative move in");
	focuser_frame_layout->addWidget(m_focusing_in_button, row, 5);
	connect(m_focusing_in_button, &QPushButton::clicked, this, &ImagerWindow::on_focus_in);

	row++;
	spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	focuser_frame_layout->addWidget(toolbar, row, 0, 1, 6);

	m_focusing_preview_button = new QPushButton("Preview");
	m_focusing_preview_button->setStyleSheet("min-width: 30px");
	m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	m_focusing_preview_button->setToolTip("Take preview exposure");
	toolbox->addWidget(m_focusing_preview_button);
	set_ok(m_focusing_preview_button);
	connect(m_focusing_preview_button, &QPushButton::clicked, this, &ImagerWindow::on_focus_preview_start_stop);
	m_focusing_button = new QPushButton("Focus");
	m_focusing_button->setToolTip("Start focusing process");
	m_focusing_button->setStyleSheet("min-width: 30px");
	m_focusing_button->setIcon(QIcon(":resource/focus.png"));
	toolbox->addWidget(m_focusing_button);
	set_ok(m_focusing_button);
	connect(m_focusing_button, &QPushButton::clicked, this, &ImagerWindow::on_focus_start_stop);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	button->setToolTip("Abort focusing process");
	toolbox->addWidget(button);
	set_ok(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);

	row++;
	m_focusing_progress = new QProgressBar();
	focuser_frame_layout->addWidget(m_focusing_progress, row, 0, 1, 6);
	m_focusing_progress->setMaximum(1);
	m_focusing_progress->setValue(0);
	m_focusing_progress->setFormat("Focusing: Idle");

	row++;
	spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	focuser_frame_layout->addItem(spacer, row, 0);

	row++;
	m_temperature_compensation_frame = new QFrame();
	m_temperature_compensation_frame->setHidden(true);
	QGridLayout *temperature_compensation_frame_layout = new QGridLayout();
	temperature_compensation_frame_layout->setAlignment(Qt::AlignTop);
	temperature_compensation_frame_layout->setColumnStretch(0, 1);
	temperature_compensation_frame_layout->setColumnStretch(1, 0);
	temperature_compensation_frame_layout->setColumnStretch(2, 0);
	temperature_compensation_frame_layout->setContentsMargins(0, 0, 0, 0);

	m_temperature_compensation_frame->setLayout(temperature_compensation_frame_layout);
	m_temperature_compensation_frame->setFrameShape(QFrame::StyledPanel);
	m_temperature_compensation_frame->setContentsMargins(0, 0, 0, 0);
	focuser_frame_layout->addWidget(m_temperature_compensation_frame, row, 0, 1, 6);

	int temp_comp_row = 0;
	label = new QLabel("Reference T (°C):");
	label->setToolTip("Reference temperature used by the focuser");
	temperature_compensation_frame_layout->addWidget(label, temp_comp_row, 0, 1, 1, Qt::AlignRight);

	m_focuser_temperature = new QLineEdit();
	m_focuser_temperature->setStyleSheet("width: 30px");
	m_focuser_temperature->setText("");
	m_focuser_temperature->setReadOnly(true);
	m_focuser_temperature->setToolTip("Reference temperature used by the focuser");
	temperature_compensation_frame_layout->addWidget(m_focuser_temperature, temp_comp_row, 1, 1, 1, Qt::AlignRight);

	m_temperature_compensation_cbox = new QCheckBox("Auto compensation");
	m_temperature_compensation_cbox->setEnabled(false);
	m_temperature_compensation_cbox->setToolTip("Turn ON/OFF automatic focus compensation");
	temperature_compensation_frame_layout->addWidget(m_temperature_compensation_cbox, temp_comp_row, 2, 1, 1, Qt::AlignRight);
	connect(m_temperature_compensation_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_focuser_temp_compensation_changed);

	row++;
	// Tools tabbar
	QTabWidget *focuser_tabbar = new QTabWidget;
	focuser_frame_layout->addWidget(focuser_tabbar, row, 0, 1, 6);

	QFrame *stats_frame = new QFrame();
	focuser_tabbar->addTab(stats_frame, "Statistics");

	QGridLayout *stats_frame_layout = new QGridLayout();
	stats_frame_layout->setAlignment(Qt::AlignTop);
	stats_frame->setLayout(stats_frame_layout);
	stats_frame->setFrameShape(QFrame::StyledPanel);
	//stats_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	stats_frame->setContentsMargins(0, 0, 0, 0);

	int stats_row = 0;
	m_focus_graph_label = new QLabel();
	m_focus_graph_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	stats_frame_layout->addWidget(m_focus_graph_label, stats_row, 0);

	stats_row++;
	m_focus_graph = new FocusGraph();
	m_focus_graph->redraw_data(m_focus_hfd_data);
	m_focus_graph->setMinimumHeight(230);
	stats_frame_layout->addWidget(m_focus_graph, stats_row, 0);

	stats_row++;
	m_contrast_stats_frame = new QFrame();
	QGridLayout *contrast_stats_frame_layout = new QGridLayout();
	contrast_stats_frame_layout->setAlignment(Qt::AlignTop);
	m_contrast_stats_frame->setLayout(contrast_stats_frame_layout);
	m_contrast_stats_frame->setFrameShape(QFrame::StyledPanel);
	m_contrast_stats_frame->setContentsMargins(0, 0, 0, 0);
	contrast_stats_frame_layout->setColumnStretch(0, 1);
	contrast_stats_frame_layout->setColumnStretch(1, 1);
	contrast_stats_frame_layout->setColumnStretch(2, 2);
	stats_frame_layout->addWidget(m_contrast_stats_frame, stats_row, 0);

	int contrast_stats_row = 0;
	label = new QLabel("Contrast x100 (c/b):");
	label->setToolTip("RMS contrast x100 (current/best)");
	contrast_stats_frame_layout->addWidget(label, contrast_stats_row, 0);
	m_contrast_label = new QLabel();
	m_contrast_label->setToolTip("RMS contrast x100 (current/best)");
	m_contrast_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	contrast_stats_frame_layout->addWidget(m_contrast_label, contrast_stats_row, 1);
	m_contrast_stats_frame->hide();

	m_bahtinov_stats_frame = new QFrame();
	QGridLayout *bahtinov_stats_frame_layout = new QGridLayout();
	bahtinov_stats_frame_layout->setAlignment(Qt::AlignTop);
	m_bahtinov_stats_frame->setLayout(bahtinov_stats_frame_layout);
	m_bahtinov_stats_frame->setFrameShape(QFrame::StyledPanel);
	m_bahtinov_stats_frame->setContentsMargins(0, 0, 0, 0);
	bahtinov_stats_frame_layout->setColumnStretch(0, 1);
	bahtinov_stats_frame_layout->setColumnStretch(1, 1);
	bahtinov_stats_frame_layout->setColumnStretch(2, 2);
	stats_frame_layout->addWidget(m_bahtinov_stats_frame, stats_row, 0);

	int bahtinov_stats_row = 0;
	label = new QLabel("Bahtinov error (c/b):");
	label->setToolTip("Bahtinov error (current/best)");
	bahtinov_stats_frame_layout->addWidget(label, bahtinov_stats_row, 0);
	m_bahtinov_label = new QLabel();
	m_bahtinov_label->setToolTip("Bahtinov error (current/best)");
	m_bahtinov_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	bahtinov_stats_frame_layout->addWidget(m_bahtinov_label, bahtinov_stats_row, 1);
	m_bahtinov_stats_frame->hide();

	m_hfd_stats_frame = new QFrame();
	QGridLayout *hfd_stats_frame_layout = new QGridLayout();
	hfd_stats_frame_layout->setAlignment(Qt::AlignTop);
	m_hfd_stats_frame->setLayout(hfd_stats_frame_layout);
	m_hfd_stats_frame->setFrameShape(QFrame::StyledPanel);
	m_hfd_stats_frame->setContentsMargins(0, 0, 0, 0);
	stats_frame_layout->addWidget(m_hfd_stats_frame, stats_row, 0);

	int hfd_stats_row = 0;
	label = new QLabel("HFD (c/b):");
	label->setToolTip("Half Flux Diameter (current/best)");
	hfd_stats_frame_layout->addWidget(label, hfd_stats_row, 0);
	m_HFD_label = new QLabel();
	m_HFD_label->setToolTip("Half Flux Diameter (current/best)");
	m_HFD_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	hfd_stats_frame_layout->addWidget(m_HFD_label, hfd_stats_row, 1);

	label = new QLabel("FWHM:");
	hfd_stats_frame_layout->addWidget(label, hfd_stats_row, 2);
	m_FWHM_label = new QLabel();
	m_FWHM_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	hfd_stats_frame_layout->addWidget(m_FWHM_label, hfd_stats_row, 3);

	hfd_stats_row++;
	label = new QLabel("Drift (X, Y):");
	hfd_stats_frame_layout->addWidget(label, hfd_stats_row, 0);
	m_drift_label = new QLabel();
	m_drift_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	hfd_stats_frame_layout->addWidget(m_drift_label, hfd_stats_row, 1);

	label = new QLabel("Peak:");
	hfd_stats_frame_layout->addWidget(label, hfd_stats_row, 2);
	m_peak_label = new QLabel();
	m_peak_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	hfd_stats_frame_layout->addWidget(m_peak_label, hfd_stats_row, 3);

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
	m_focuser_exposure_time->setDecimals(3);
	m_focuser_exposure_time->setMaximum(10000);
	m_focuser_exposure_time->setMinimum(0);
	m_focuser_exposure_time->setValue(1);
	settings_frame_layout->addWidget(m_focuser_exposure_time, settings_row, 2, 1, 2);

	settings_row++;
	label = new QLabel("Estimator:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1 ,1);
	m_focus_estimator_select = new QComboBox();
	settings_frame_layout->addWidget(m_focus_estimator_select, settings_row, 1, 1, 2);
	connect(m_focus_estimator_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focus_estimator_selected);

	m_focus_mode_select = new QComboBox();
	m_focus_mode_select->addItem("Manual");
	m_focus_mode_select->addItem("Auto");
	settings_frame_layout->addWidget(m_focus_mode_select, settings_row, 3, 1, 1);
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
	m_star_x->setEnabled(false);
	settings_frame_layout->addWidget(m_star_x , settings_row, 2);
	connect(m_star_x, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_selection_changed);

	m_star_y = new QDoubleSpinBox();
	m_star_y->setMaximum(100000);
	m_star_y->setMinimum(0);
	m_star_y->setValue(0);
	m_star_y->setEnabled(false);
	settings_frame_layout->addWidget(m_star_y, settings_row, 3);
	connect(m_star_y, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_selection_changed);

	settings_row++;

	label = new QLabel("Star count:");
	settings_frame_layout->addWidget(label, settings_row, 0);
	m_focus_star_count = new QSpinBox();
	m_focus_star_count->setMaximum(100000);
	m_focus_star_count->setMinimum(0);
	m_focus_star_count->setValue(0);
	m_focus_star_count->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_star_count, settings_row, 1);
	connect(m_focus_star_count, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_focuser_selection_star_count_changed);

	label = new QLabel("Radius (px):");
	settings_frame_layout->addWidget(label, settings_row, 2);
	m_focus_star_radius = new QSpinBox();
	m_focus_star_radius->setMaximum(100000);
	m_focus_star_radius->setMinimum(0);
	m_focus_star_radius->setValue(0);
	m_focus_star_radius->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_star_radius, settings_row, 3);
	connect(m_focus_star_radius, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_focuser_selection_radius_changed);

	settings_row++;
	spacer = new QSpacerItem(1, 5, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	button = new QPushButton("Clear star selection");
	button->setStyleSheet("min-width: 30px");
	button->setToolTip("Keyboard shortcut: Shift+Backspace");
	settings_frame_layout->addWidget(button, settings_row, 0, 1, 4);

	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_focuser_clear_selection);
	QShortcut *shortcut = new QShortcut(QKeySequence("Shift+Backspace"), this);
	connect(shortcut, &QShortcut::activated, this, [this](){this->on_focuser_clear_selection(true);});

	settings_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	settings_frame_layout->addItem(spacer, settings_row, 0);

	settings_row++;
	label = new QLabel("Autofocus settings:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 4);

	settings_row++;
	m_initial_step_label = new QLabel("Initial step:");
	settings_frame_layout->addWidget(m_initial_step_label, settings_row, 0);
	m_initial_step = new QSpinBox();
	m_initial_step->setMaximum(1000000);
	m_initial_step->setMinimum(0);
	m_initial_step->setValue(0);
	m_initial_step->setEnabled(false);
	settings_frame_layout->addWidget(m_initial_step , settings_row, 1);

	m_ucurve_step = new QSpinBox();
	m_ucurve_step->setMaximum(100000);
	m_ucurve_step->setMinimum(0);
	m_ucurve_step->setValue(0);
	m_ucurve_step->setEnabled(false);
	m_ucurve_step->hide();
	settings_frame_layout->addWidget(m_ucurve_step, settings_row, 1);

	m_final_step_label = new QLabel("Final step:");
	settings_frame_layout->addWidget(m_final_step_label, settings_row, 2);
	m_final_step = new QSpinBox();
	m_final_step->setMaximum(100000);
	m_final_step->setMinimum(0);
	m_final_step->setValue(0);
	m_final_step->setEnabled(false);
	settings_frame_layout->addWidget(m_final_step, settings_row, 3);

	m_ucurve_samples = new QSpinBox();
	m_ucurve_samples->setMaximum(100000);
	m_ucurve_samples->setMinimum(0);
	m_ucurve_samples->setValue(0);
	m_ucurve_samples->setEnabled(false);
	m_ucurve_samples->hide();
	settings_frame_layout->addWidget(m_ucurve_samples, settings_row, 3);

	settings_row++;
	label = new QLabel("Backlash:");
	settings_frame_layout->addWidget(label, settings_row, 0);
	m_focus_backlash = new QSpinBox();
	m_focus_backlash->setMaximum(1000000);
	m_focus_backlash->setMinimum(0);
	m_focus_backlash->setValue(0);
	m_focus_backlash->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_backlash, settings_row, 1);
	m_focus_backlash->setKeyboardTracking(false);
	connect(m_focus_backlash, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_focuser_backlash_changed);

	label = new QLabel("Stacking:");
	settings_frame_layout->addWidget(label, settings_row, 2);
	m_focus_stack = new QSpinBox();
	m_focus_stack->setMaximum(100000);
	m_focus_stack->setMinimum(0);
	m_focus_stack->setValue(0);
	m_focus_stack->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_stack, settings_row, 3);

	settings_row++;
	label = new QLabel("Backlash overshoot factor:");
	settings_frame_layout->addWidget(label, settings_row, 0, 1, 3);
	m_focus_bl_overshoot = new QDoubleSpinBox();
	m_focus_bl_overshoot->setMaximum(1000000);
	m_focus_bl_overshoot->setMinimum(0);
	m_focus_bl_overshoot->setValue(1);
	m_focus_bl_overshoot->setEnabled(false);
	settings_frame_layout->addWidget(m_focus_bl_overshoot, settings_row, 3);
	connect(m_focus_bl_overshoot, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_focuser_bl_overshoot_changed);

	// Misc tab
	QFrame *misc_frame = new QFrame;
	focuser_tabbar->addTab(misc_frame, "Misc");

	QGridLayout *misc_frame_layout = new QGridLayout();
	misc_frame_layout->setAlignment(Qt::AlignTop);
	misc_frame->setLayout(misc_frame_layout);
	misc_frame->setFrameShape(QFrame::StyledPanel);
	misc_frame->setContentsMargins(0, 0, 0, 0);

	int misc_row = 0;
	label = new QLabel("Save bandwidth:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	label = new QLabel("Use subframe:");
	misc_frame_layout->addWidget(label, misc_row, 0, 1 ,2);
	m_focuser_subframe_select = new QComboBox();
	m_focuser_subframe_select->addItem("Off");
	m_focuser_subframe_select->addItem("10 radii");
	m_focuser_subframe_select->addItem("20 radii");
	misc_frame_layout->addWidget(m_focuser_subframe_select, misc_row, 2, 1, 2);
	m_focuser_subframe_select->setCurrentIndex(conf.focuser_subframe);
	connect(m_focuser_subframe_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_subframe_changed);

	misc_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	misc_frame_layout->addItem(spacer, misc_row, 0);

	misc_row++;
	label = new QLabel("Focus compensantion:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	label = new QLabel("Compensation (steps/°C):");
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 2);
	m_focuser_temperature_compensation_steps = new QSpinBox();
	m_focuser_temperature_compensation_steps->setMaximum(10000);
	m_focuser_temperature_compensation_steps->setMinimum(-10000);
	m_focuser_temperature_compensation_steps->setValue(0);
	m_focuser_temperature_compensation_steps->setEnabled(false);
	m_focuser_temperature_compensation_steps->setKeyboardTracking(false);
	misc_frame_layout->addWidget(m_focuser_temperature_compensation_steps, misc_row, 2, 1, 2);

	connect(m_focuser_temperature_compensation_steps, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_focuser_temp_compensation_steps_changed);

	misc_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	misc_frame_layout->addItem(spacer, misc_row, 0);

	misc_row++;
	label = new QLabel("On focus failed (Peak/HFD):");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	m_focuser_failreturn_cbox = new QCheckBox("Return to the initial position");
	m_focuser_failreturn_cbox->setEnabled(false);
	misc_frame_layout->addWidget(m_focuser_failreturn_cbox, misc_row, 0, 1, 4);
	connect(m_focuser_failreturn_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_focuser_failreturn_changed);

	misc_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	misc_frame_layout->addItem(spacer, misc_row, 0);

	misc_row++;
	label = new QLabel("Miscellaneous settings:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 4);

	misc_row++;
	label = new QLabel("Invert IN and OUT motion:");
	misc_frame_layout->addWidget(label, misc_row, 0, 1, 2);

	m_focuser_reverse_select = new QComboBox();
	misc_frame_layout->addWidget(m_focuser_reverse_select, misc_row, 2, 1, 2);
	connect(m_focuser_reverse_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_focuser_reverse_changed);
}

void ImagerWindow::on_focus_estimator_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_focus_estimator_property(selected_agent);
	});
}

void ImagerWindow::select_focuser_data(focuser_display_data show) {
	switch (show) {
		case SHOW_FWHM:  // FWHM is not shown any more
		case SHOW_HFD:
			m_focus_display_data = &m_focus_hfd_data;
			set_text(m_focus_graph_label, "Focus HFD (px):");
			break;
		case SHOW_BAHTINOV:
			m_focus_display_data = &m_focus_bahtinov_data;
			set_text(m_focus_graph_label, "Bahtinov error (px):");
			break;
		case SHOW_CONTRAST:
			m_focus_display_data = &m_focus_contrast_data;
			set_text(m_focus_graph_label, "RMS Contrast (x100):");
			break;
		default:
			m_focus_display_data = nullptr;
	}
}

void ImagerWindow::on_focuser_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_focuser[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_focuser_str = m_focuser_select->currentText();
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

void ImagerWindow::on_focuser_backlash_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_focus_params_property(selected_agent, true);
	});
}

void ImagerWindow::on_focuser_bl_overshoot_changed(double value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_focuser_bl_overshoot(selected_agent);
	});
}

void ImagerWindow::on_focuser_temp_compensation_changed(int state) {
	Q_UNUSED(state);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		bool checked = m_temperature_compensation_cbox->checkState();
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		if (checked) {
			indigo_change_switch_property_1(nullptr, selected_agent, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_AUTOMATIC_ITEM_NAME, true);
		} else {
			indigo_change_switch_property_1(nullptr, selected_agent, FOCUSER_MODE_PROPERTY_NAME, FOCUSER_MODE_MANUAL_ITEM_NAME, true);
		}
	});
}

void ImagerWindow::on_focuser_temp_compensation_steps_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_focuser_temperature_compensation_steps(selected_agent);
	});
}

void ImagerWindow::on_selection_changed(double value) {
	Q_UNUSED(value);
	int x = m_star_x->value();
	int y = m_star_y->value();
	m_imager_viewer->moveSelection(x, y);
	m_HFD_label->setText("n/a");
	m_FWHM_label->setText("n/a");
	m_peak_label->setText("n/a");
	m_drift_label->setText("n/a");
}

void ImagerWindow::on_focuser_selection_radius_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_star_selection(selected_agent);
	});
}

void ImagerWindow::on_focuser_selection_star_count_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_star_count(selected_agent);
	});
}

void ImagerWindow::on_focuser_clear_selection(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		clear_imager_agent_star_selection(selected_agent);
	});
}

void ImagerWindow::on_focus_mode_selected(int index) {
	conf.focus_mode = index;
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_image_right_click(double x, double y, Qt::KeyboardModifiers modifiers) {
	Q_UNUSED(modifiers);
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

void ImagerWindow::on_focuser_position_changed() {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		indigo_property *focuser_position = properties.get(selected_agent, FOCUSER_POSITION_PROPERTY_NAME);
		if (!focuser_position) {
			return;
		}
		if (focuser_position->state != INDIGO_BUSY_STATE) {
			change_focuser_position_property(selected_agent);
		} else {
			indigo_change_switch_property_1(nullptr, selected_agent, FOCUSER_ABORT_MOTION_PROPERTY_NAME, FOCUSER_ABORT_MOTION_ITEM_NAME, true);
		}
	});
}

void ImagerWindow::on_focus_preview_start_stop(bool clicked) {
	Q_UNUSED(clicked);
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
			change_ccd_frame_property(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
			change_ccd_exposure_property(selected_agent, m_focuser_exposure_time);
		}
	});
}

void ImagerWindow::on_focus_start_stop(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		indigo_property *agent_start_process = properties.get(selected_agent, AGENT_START_PROCESS_PROPERTY_NAME);
		if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
			change_agent_abort_process_property(selected_agent);
		} else {
			m_focus_hfd_data.clear();
			m_focus_bahtinov_data.clear();
			change_agent_star_selection(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
			change_agent_batch_property_for_focus(selected_agent);
			change_agent_focus_params_property(selected_agent, false);
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
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_focuser_focus_in_property(selected_agent);
		change_focuser_steps_property(selected_agent);
	});
}

void ImagerWindow::on_focus_out(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_focuser_focus_out_property(selected_agent);
		change_focuser_steps_property(selected_agent);
	});
}

void ImagerWindow::on_focuser_subframe_changed(int index) {
	Q_UNUSED(index);
	conf.focuser_subframe = index;
	write_conf();
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_focuser_subframe(selected_agent);
	});
}

void ImagerWindow::on_focuser_failreturn_changed(int state) {
	Q_UNUSED(state);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		bool checked = m_focuser_failreturn_cbox->checkState();

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		if (checked) {
			indigo_change_switch_property_1(nullptr, selected_agent, AGENT_IMAGER_FOCUS_FAILURE_PROPERTY_NAME, AGENT_IMAGER_FOCUS_FAILURE_RESTORE_ITEM_NAME, true);
		} else {
			indigo_change_switch_property_1(nullptr, selected_agent, AGENT_IMAGER_FOCUS_FAILURE_PROPERTY_NAME, AGENT_IMAGER_FOCUS_FAILURE_STOP_ITEM_NAME, true);
		}
	});
}

void ImagerWindow::on_focuser_reverse_changed(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_focuser_reverse_property(selected_agent);
	});
}
