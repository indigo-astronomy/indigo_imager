// Copyright (c) 2021 Rumen G.Bogdanovski
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

#include <imagerwindow.h>
#include <propertycache.h>
#include <indigoclient.h>
#include <conf.h>
#include <indigo_cat_data.h>
#include <QLCDNumber>
#include <qaddcustomobject.h>

void write_conf();

void ImagerWindow::create_telescope_tab(QFrame *telescope_frame) {
	QGridLayout *telescope_frame_layout = new QGridLayout();
	telescope_frame_layout->setAlignment(Qt::AlignTop);
	telescope_frame_layout->setColumnStretch(0, 2);
	telescope_frame_layout->setColumnStretch(1, 2);
	telescope_frame_layout->setColumnStretch(2, 7);
	telescope_frame_layout->setColumnStretch(3, 7);
	//telescope_frame_layout->setColumnStretch(4, 3);

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
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);

	m_mount_ra_label = new QLCDNumber(13);
	m_mount_ra_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_ra_label->setMinimumHeight(LCD_SIZE);
	m_mount_ra_label->display("00: 00:00.00");
	set_ok(m_mount_ra_label);
	m_mount_ra_label->show();
	telescope_frame_layout->addWidget(m_mount_ra_label, row, 2, 1, 2);
	m_mount_ra = 0;

	row++;
	m_mount_dec_label = new QLCDNumber(13);
	m_mount_dec_label->setSegmentStyle(QLCDNumber::Flat);
	m_mount_dec_label->setMinimumHeight(LCD_SIZE);
	m_mount_dec_label->display("00' 00 00.00");
	set_ok(m_mount_dec_label);
	m_mount_dec_label->show();
	telescope_frame_layout->addWidget(m_mount_dec_label, row, 2, 1, 2);
	m_mount_dec = 0;

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	QFont font;
	row++;
	label = new QLabel("Az / Alt:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);

	m_mount_az_label = new QLabel("0° 00' 00.0\"");
	m_mount_az_label->setAlignment(Qt::AlignCenter);
	//m_mount_az_label->setContentsMargins(1, 1, 1, 1);
	font = m_mount_az_label->font();
	font.setPointSize(font.pointSize() + 2);
	//font.setBold(true);
	m_mount_az_label->setFont(font);
	set_ok(m_mount_az_label);
	telescope_frame_layout->addWidget(m_mount_az_label, row, 2, 1, 1);

	m_mount_alt_label = new QLabel("0° 00' 00.0\"");
	m_mount_alt_label->setAlignment(Qt::AlignCenter);
	//m_mount_alt_label->setContentsMargins(1, 1, 1, 1);
	font = m_mount_alt_label->font();
	font.setPointSize(font.pointSize() + 2);
	//font.setBold(true);
	m_mount_alt_label->setFont(font);
	set_ok(m_mount_alt_label);
	telescope_frame_layout->addWidget(m_mount_alt_label, row, 3, 1, 1);

	row++;
	label = new QLabel("LST / Transit in:");
	label->setToolTip("Local Sidereal Time / Time to Transit");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0);

	m_mount_lst_label = new QLabel("00:00:00");
	m_mount_lst_label->setAlignment(Qt::AlignCenter);
	font = m_mount_lst_label->font();
	font.setPointSize(font.pointSize() + 2);
	//font.setBold(true);
	m_mount_lst_label->setFont(font);
	set_ok(m_mount_lst_label);
	telescope_frame_layout->addWidget(m_mount_lst_label, row, 2, 1, 1);

	m_mount_ttr_label = new QLabel("0:00:00");
	m_mount_ttr_label->setAlignment(Qt::AlignCenter);
	font = m_mount_ttr_label->font();
	font.setPointSize(font.pointSize() + 2);
	//font.setBold(true);
	m_mount_ttr_label->setFont(font);
	set_ok(m_mount_ttr_label);
	telescope_frame_layout->addWidget(m_mount_ttr_label, row, 3, 1, 1);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	telescope_frame_layout->addItem(spacer, row, 0, 1, 4);

	row++;
	label = new QLabel("RA / Dec input:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	telescope_frame_layout->addWidget(label, row, 0, 1, 2);

	m_mount_ra_input = new QLineEdit();
	m_mount_ra_input->setPlaceholderText("hh:mm:ss");
	m_mount_ra_input->setToolTip("Enter Right ascension in format hh:mm:ss or\nright-click on a solved image to load the coordinates");
	telescope_frame_layout->addWidget(m_mount_ra_input, row, 2);

	m_mount_dec_input = new QLineEdit();
	m_mount_dec_input->setPlaceholderText("dd:mm:ss");
	m_mount_dec_input->setToolTip("Enter Declination in format dd:mm:ss or\nright-click on a solved image to load the coordinates");
	telescope_frame_layout->addWidget(m_mount_dec_input, row, 3);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	telescope_frame_layout->addWidget(toolbar, row, 0, 1, 4);

	//m_mount_use_solver_cbox = new QCheckBox("Use Solver");
	//m_mount_use_solver_cbox->setEnabled(false);
	//toolbox->addWidget(m_mount_use_solver_cbox);
	//connect(m_mount_guide_rate_cbox, &QPushButton::clicked, this, &ImagerWindow::on_mount_set_guide_rate);

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

	row++;
	// Tools tabbar
	QTabWidget *telescope_tabbar = new QTabWidget;
	telescope_frame_layout->addWidget(telescope_tabbar, row, 0, 1, 4);

	// image frame
	QFrame *slew_frame = new QFrame();
	telescope_tabbar->addTab(slew_frame, "Main");

	QGridLayout *slew_frame_layout = new QGridLayout();
	slew_frame_layout->setAlignment(Qt::AlignTop);
	slew_frame_layout->setColumnStretch(0, 1);
	slew_frame_layout->setColumnStretch(1, 1);
	slew_frame_layout->setColumnStretch(2, 1);
	slew_frame_layout->setColumnStretch(3, 1);
	slew_frame_layout->setColumnStretch(4, 4);
	slew_frame->setLayout(slew_frame_layout);
	slew_frame->setFrameShape(QFrame::StyledPanel);
	slew_frame->setContentsMargins(0, 0, 0, 0);

	int slew_row = 0;
	int slew_col = 0;
	QPushButton *move_button = new QPushButton("N");
	move_button->setStyleSheet("min-width: 20px");
	slew_frame_layout->addWidget(move_button, slew_row + 0, slew_col + 1);
	connect(move_button, &QPushButton::pressed, this, &ImagerWindow::on_mount_move_north);
	connect(move_button, &QPushButton::released, this, &ImagerWindow::on_mount_stop_north);

	move_button = new QPushButton("W");
	move_button->setStyleSheet("min-width: 20px");
	slew_frame_layout->addWidget(move_button, slew_row + 1, slew_col + 0);
	connect(move_button, &QPushButton::pressed, this, &ImagerWindow::on_mount_move_west);
	connect(move_button, &QPushButton::released, this, &ImagerWindow::on_mount_stop_west);

	move_button = new QPushButton("");
	move_button->setStyleSheet("min-width: 20px");
	move_button->setIcon(QIcon(":resource/stop.png"));
	slew_frame_layout->addWidget(move_button, slew_row + 1, slew_col + 1);
	connect(move_button, &QPushButton::clicked, this, &ImagerWindow::on_mount_abort);

	move_button = new QPushButton("E");
	move_button->setStyleSheet("min-width: 20px");
	slew_frame_layout->addWidget(move_button, slew_row + 1, slew_col + 2);
	connect(move_button, &QPushButton::pressed, this, &ImagerWindow::on_mount_move_east);
	connect(move_button, &QPushButton::released, this, &ImagerWindow::on_mount_stop_east);

	move_button = new QPushButton("S");
	move_button->setStyleSheet("min-width: 20px");
	slew_frame_layout->addWidget(move_button, slew_row + 2, slew_col + 1);
	connect(move_button, &QPushButton::pressed, this, &ImagerWindow::on_mount_move_south);
	connect(move_button, &QPushButton::released, this, &ImagerWindow::on_mount_stop_south);

	slew_col = 0;
	slew_row = 3;
	m_mount_guide_rate_cbox = new QCheckBox("Guide rate");
	m_mount_guide_rate_cbox->setEnabled(false);
	slew_frame_layout->addWidget(m_mount_guide_rate_cbox, slew_row, slew_col, 1, 3);
	connect(m_mount_guide_rate_cbox, &QPushButton::clicked, this, &ImagerWindow::on_mount_set_guide_rate);

	slew_row++;
	m_mount_center_rate_cbox = new QCheckBox("Centering rate");
	m_mount_center_rate_cbox->setEnabled(false);
	slew_frame_layout->addWidget(m_mount_center_rate_cbox, slew_row, slew_col, 1, 3);
	connect(m_mount_center_rate_cbox, &QPushButton::clicked, this, &ImagerWindow::on_mount_set_center_rate);

	slew_row++;
	m_mount_find_rate_cbox = new QCheckBox("Finding rate");
	m_mount_find_rate_cbox->setEnabled(false);
	slew_frame_layout->addWidget(m_mount_find_rate_cbox, slew_row, slew_col, 1, 3);
	connect(m_mount_find_rate_cbox, &QPushButton::clicked, this, &ImagerWindow::on_mount_set_find_rate);

	slew_row++;
	m_mount_max_rate_cbox = new QCheckBox("Max rate");
	m_mount_max_rate_cbox->setEnabled(false);
	slew_frame_layout->addWidget(m_mount_max_rate_cbox, slew_row, slew_col, 1, 3);
	connect(m_mount_max_rate_cbox, &QPushButton::clicked, this, &ImagerWindow::on_mount_set_max_rate);

	slew_col = 4;
	slew_row = 0;
	m_mount_side_of_pier_label = new QLabel("Side of pier: Unknown");
	set_ok(m_mount_side_of_pier_label);
	slew_frame_layout->addWidget(m_mount_side_of_pier_label, slew_row, slew_col);

	slew_row++;
	m_mount_track_cbox = new QCheckBox("Tracking");
	m_mount_track_cbox->setEnabled(false);
	set_ok(m_mount_track_cbox);
	slew_frame_layout->addWidget(m_mount_track_cbox, slew_row, slew_col);
	connect(m_mount_track_cbox, &QPushButton::clicked, this, &ImagerWindow::on_mount_track);

	slew_row++;
	m_mount_home_cbox = new QCheckBox("Go home");
	m_mount_home_cbox->setEnabled(false);
	set_ok(m_mount_home_cbox);
	slew_frame_layout->addWidget(m_mount_home_cbox, slew_row, slew_col);
	connect(m_mount_home_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_mount_home);

	slew_row++;
	m_mount_park_cbox = new QCheckBox("Park");
	m_mount_park_cbox->setEnabled(false);
	set_ok(m_mount_park_cbox);
	slew_frame_layout->addWidget(m_mount_park_cbox, slew_row, slew_col);
	connect(m_mount_park_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_mount_park);

	slew_row+=2;
	label = new QLabel("Joystick control:");
	slew_frame_layout->addWidget(label, slew_row, slew_col);
	slew_row++;
	m_mount_joystick_select = new QComboBox();
	m_mount_joystick_select->setToolTip("Select joystick to control the telescope");
	slew_frame_layout->addWidget(m_mount_joystick_select, slew_row, slew_col);
	connect(m_mount_joystick_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_mount_joystick_selected);

	QFrame *obj_frame = new QFrame();
	telescope_tabbar->addTab(obj_frame, "Object");

	QGridLayout *obj_frame_layout = new QGridLayout();
	obj_frame_layout->setAlignment(Qt::AlignTop);
	//obj_frame_layout->setColumnStretch(0, 1);
	//obj_frame_layout->setColumnStretch(1, 3);
	obj_frame->setLayout(obj_frame_layout);
	obj_frame->setFrameShape(QFrame::StyledPanel);
	obj_frame->setContentsMargins(0, 0, 0, 0);

	int obj_row = 0;
	label = new QLabel("Search: ");
	obj_frame_layout->addWidget(label, obj_row, 0);
	m_object_search_line = new QLineEdit();
	m_object_search_line->setPlaceholderText("E.g. M42, Ain, Vega ...");
	obj_frame_layout->addWidget(m_object_search_line, obj_row, 1, 1, 2);
	connect(m_object_search_line, &QLineEdit::textEdited, this, &ImagerWindow::on_object_search_changed);
	connect(m_object_search_line, &QLineEdit::returnPressed, this, &ImagerWindow::on_object_search_entered);

	m_add_object_button = new QToolButton(this);
	m_add_object_button->setToolTip(tr("Add object to custom database"));
	m_add_object_button->setIcon(QIcon(":resource/zoom-in.png"));
	obj_frame_layout->addWidget(m_add_object_button, obj_row, 3);
	connect(m_add_object_button, &QToolButton::clicked, this, &ImagerWindow::on_custom_object_add);

	m_remove_object_button = new QToolButton(this);
	m_remove_object_button->setToolTip(tr("Remove selected custom database object"));
	m_remove_object_button->setIcon(QIcon(":resource/zoom-out.png"));
	obj_frame_layout->addWidget(m_remove_object_button, obj_row, 4);
	connect(m_remove_object_button, &QToolButton::clicked, this, &ImagerWindow::on_custom_object_remove);

	obj_row++;
	m_object_list = new QListWidget();
	m_object_list->setStyleSheet("QListWidget {border: 1px solid #404040;}");
	obj_frame_layout->addWidget(m_object_list, obj_row, 0, 1, 5);
	connect(m_object_list, &QListWidget::itemSelectionChanged, this, &ImagerWindow::on_object_selected);
	connect(m_object_list, &QListWidget::itemClicked, this, &ImagerWindow::on_object_clicked);

	QFrame *solve_frame = new QFrame();
	telescope_tabbar->addTab(solve_frame, "Solver");

	QGridLayout *solve_frame_layout = new QGridLayout();
	solve_frame_layout->setAlignment(Qt::AlignTop);
	solve_frame->setLayout(solve_frame_layout);
	solve_frame->setFrameShape(QFrame::StyledPanel);
	solve_frame->setContentsMargins(0, 0, 0, 0);

	int solve_row = 0;
	m_solver_status_label2 = new QLabel("");
	m_solver_status_label2->setTextFormat(Qt::RichText);
	m_solver_status_label2->setText("<img src=\":resource/led-grey.png\"> Idle");
	solve_frame_layout->addWidget(m_solver_status_label2, solve_row, 0, 1, 4);

	solve_row++;
	label = new QLabel("Image source:");
	solve_frame_layout->addWidget(label, solve_row, 0, 1, 2);
	m_solver_source_select2 = new QComboBox();
	solve_frame_layout->addWidget(m_solver_source_select2, solve_row, 2, 1, 2);
	connect(m_solver_source_select2, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_image_source2_selected);

	solve_row++;
	// Exposure time
	label = new QLabel("Exposure time (s):");
	solve_frame_layout->addWidget(label, solve_row, 0, 1, 2);
	m_solver_exposure2 = new QDoubleSpinBox();
	m_solver_exposure2->setDecimals(3);
	m_solver_exposure2->setMaximum(10000);
	m_solver_exposure2->setMinimum(0);
	m_solver_exposure2->setValue(1);
	m_solver_exposure2->setEnabled(false);
	solve_frame_layout->addWidget(m_solver_exposure2, solve_row, 2, 1, 2);

	solve_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	solve_frame_layout->addItem(spacer, solve_row, 0, 1, 4);

	solve_row++;
	m_mount_precise_goto_button = new QPushButton("Precise Goto");
	m_mount_precise_goto_button->setStyleSheet("min-width: 30px");
	m_mount_precise_goto_button->setIcon(QIcon(":resource/play.png"));
	m_mount_precise_goto_button->setToolTip("Solver assisted goto: performs goto, solve and recenter");
	solve_frame_layout->addWidget(m_mount_precise_goto_button, solve_row, 0, 1, 4);
	connect(m_mount_precise_goto_button, &QPushButton::clicked, this, &ImagerWindow::on_mount_precise_goto);

	solve_row++;
	toolbar = new QWidget;
	toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(0,0,0,0);
	toolbox->setContentsMargins(0,0,0,0);
	solve_frame_layout->addWidget(toolbar, solve_row, 0, 1, 4);

	m_mount_solve_and_center_button = new QPushButton("Solve && Center");
	m_mount_solve_and_center_button->setStyleSheet("min-width: 30px");
	m_mount_solve_and_center_button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(m_mount_solve_and_center_button);
	connect(m_mount_solve_and_center_button, &QPushButton::clicked, this, &ImagerWindow::on_mount_solve_and_center);

	m_mount_solve_and_sync_button = new QPushButton("Solve && Sync");
	m_mount_solve_and_sync_button->setStyleSheet("min-width: 30px");
	m_mount_solve_and_sync_button->setIcon(QIcon(":resource/calibrate.png"));
	toolbox->addWidget(m_mount_solve_and_sync_button);
	connect(m_mount_solve_and_sync_button , &QPushButton::clicked, this, &ImagerWindow::on_mount_solve_and_sync);

	QFrame *site_frame = new QFrame();
	telescope_tabbar->addTab(site_frame, "Site");
	QGridLayout *site_frame_layout = new QGridLayout();
	site_frame_layout->setAlignment(Qt::AlignTop);
	site_frame->setLayout(site_frame_layout);
	site_frame->setFrameShape(QFrame::StyledPanel);
	site_frame->setContentsMargins(0, 0, 0, 0);

	int site_row = 0;
	label = new QLabel("Source:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	site_frame_layout->addWidget(label, site_row, 0);
	m_mount_coord_source_select = new QComboBox();
	site_frame_layout->addWidget(m_mount_coord_source_select, site_row, 1, 1, 3);
	connect(m_mount_coord_source_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_mount_coord_source_selected);

	site_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	site_frame_layout->addItem(spacer, site_row, 0, 1, 4);

	site_row++;
	label = new QLabel("Latitude (-S / +N):");
	site_frame_layout->addWidget(label, site_row, 0, 1, 2);
	m_mount_latitude = new QLabel("0° 00' 00.0\"");
	set_ok(m_mount_latitude);
	site_frame_layout->addWidget(m_mount_latitude, site_row, 2, 1, 2);

	site_row++;
	label = new QLabel("Longitude (-W / +E):");
	site_frame_layout->addWidget(label, site_row, 0, 1, 2);
	m_mount_longitude = new QLabel("0° 00' 00.0\"");
	set_ok(m_mount_longitude);
	site_frame_layout->addWidget(m_mount_longitude, site_row, 2, 1, 2);

	site_row++;
	label = new QLabel("UTC time:");
	site_frame_layout->addWidget(label, site_row, 0, 1, 2);
	m_mount_utc = new QLabel("00");
	set_idle(m_mount_utc);
	site_frame_layout->addWidget(m_mount_utc, site_row, 2, 1, 2);

	site_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	site_frame_layout->addItem(spacer, site_row, 0, 1, 4);

	site_row++;
	label = new QLabel("Set Location / Time:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	site_frame_layout->addWidget(label, site_row, 0, 1, 4);

	site_row++;
	toolbar = new QWidget;
	toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	site_frame_layout->addWidget(toolbar, site_row, 0, 1, 4);

	label = new QLabel("Lat / Lon:");
	//label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	toolbox->addWidget(label);

	m_mount_lat_input = new QLineEdit();
	m_mount_lat_input->setEnabled(false);
	toolbox->addWidget(m_mount_lat_input);

	m_mount_lon_input = new QLineEdit();
	m_mount_lon_input->setEnabled(false);
	toolbox->addWidget(m_mount_lon_input);

	QPushButton *button = new QPushButton("Set");
	button->setStyleSheet("min-width: 25px");
	//button->setIcon(QIcon(":resource/calibrate.png"));
	toolbox->addWidget(button);
	connect(button , &QPushButton::clicked, this, &ImagerWindow::on_mount_set_coordinates_to_agent);

	site_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	site_frame_layout->addItem(spacer, site_row, 0, 1, 4);

	site_row++;
	m_mount_sync_time_cbox = new QCheckBox("Keep mount time synchronized");
	m_mount_sync_time_cbox->setEnabled(false);
	site_frame_layout->addWidget(m_mount_sync_time_cbox, site_row, 0, 1, 4);
	connect(m_mount_sync_time_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_mount_sync_time);

	QFrame *gps_frame = new QFrame();
	telescope_tabbar->addTab(gps_frame, "GPS");

	QGridLayout *gps_frame_layout = new QGridLayout();
	gps_frame_layout->setAlignment(Qt::AlignTop);
	gps_frame->setLayout(gps_frame_layout);
	gps_frame->setFrameShape(QFrame::StyledPanel);
	gps_frame->setContentsMargins(0, 0, 0, 0);

	int gps_row = 0;
	label = new QLabel("GPS:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	gps_frame_layout->addWidget(label, gps_row, 0);
	m_mount_gps_select = new QComboBox();
	gps_frame_layout->addWidget(m_mount_gps_select, gps_row, 1, 1, 3);
	connect(m_mount_gps_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_mount_gps_selected);

	gps_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	gps_frame_layout->addItem(spacer, gps_row, 0, 1, 4);

	gps_row++;
	label = new QLabel("GPS status:");
	gps_frame_layout->addWidget(label, gps_row, 0, 1, 2);
	m_gps_status = new QLabel("Unknown");
	set_idle(m_gps_status);
	gps_frame_layout->addWidget(m_gps_status, gps_row, 2, 1, 2);

	gps_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	gps_frame_layout->addItem(spacer, gps_row, 0, 1, 4);

	gps_row++;
	label = new QLabel("Latitude (-S / +N):");
	gps_frame_layout->addWidget(label, gps_row, 0, 1, 2);
	m_gps_latitude = new QLabel("0° 00' 00.0\"");
	set_ok(m_gps_latitude);
	gps_frame_layout->addWidget(m_gps_latitude, gps_row, 2, 1, 2);

	gps_row++;
	label = new QLabel("Longitude (-W / +E):");
	gps_frame_layout->addWidget(label, gps_row, 0, 1, 2);
	m_gps_longitude = new QLabel("0° 00' 00.0\"");
	set_ok(m_gps_longitude);
	gps_frame_layout->addWidget(m_gps_longitude, gps_row, 2, 1, 2);

	gps_row++;
	label = new QLabel("Elevation (m):");
	gps_frame_layout->addWidget(label, gps_row, 0, 1, 2);
	m_gps_elevation = new QLabel("0");
	set_ok(m_gps_elevation);
	gps_frame_layout->addWidget(m_gps_elevation, gps_row, 2, 1, 2);

	gps_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	gps_frame_layout->addItem(spacer, gps_row, 0, 1, 4);

	gps_row++;
	label = new QLabel("UTC time:");
	gps_frame_layout->addWidget(label, gps_row, 0, 1, 2);
	m_gps_utc = new QLabel("00");
	set_idle(m_gps_utc);
	gps_frame_layout->addWidget(m_gps_utc, gps_row, 2, 1, 2);

	// Polar Align tab
	QFrame *palign_frame = new QFrame();
	telescope_tabbar->addTab(palign_frame, "Polar align");

	QGridLayout *palign_frame_layout = new QGridLayout();
	palign_frame_layout->setAlignment(Qt::AlignTop);
	palign_frame->setLayout(palign_frame_layout);
	palign_frame->setFrameShape(QFrame::StyledPanel);
	palign_frame->setContentsMargins(0, 0, 0, 0);

	int palign_row = 0;
	m_pa_status_label = new QLabel("");
	m_pa_status_label->setTextFormat(Qt::RichText);
	m_pa_status_label->setText("<img src=\":resource/led-grey.png\"> Idle");
	palign_frame_layout->addWidget(m_pa_status_label, palign_row, 0, 1, 4);

	palign_row++;
	label = new QLabel("Image source:");
	palign_frame_layout->addWidget(label, palign_row, 0, 1, 2);
	m_solver_source_select3 = new QComboBox();
	palign_frame_layout->addWidget(m_solver_source_select3, palign_row, 2, 1, 2);
	connect(m_solver_source_select3, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_image_source3_selected);

	palign_row++;
	// Exposure time
	label = new QLabel("Exposure time (s):");
	palign_frame_layout->addWidget(label, palign_row, 0, 1, 2);
	m_solver_exposure3 = new QDoubleSpinBox();
	m_solver_exposure3->setDecimals(3);
	m_solver_exposure3->setMaximum(10000);
	m_solver_exposure3->setMinimum(0);
	m_solver_exposure3->setValue(1);
	m_solver_exposure3->setEnabled(false);
	palign_frame_layout->addWidget(m_solver_exposure3, palign_row, 2, 1, 1);
	connect(m_solver_exposure3, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_mount_agent_set_pa_settings);

	m_pa_refraction_cbox = new QCheckBox("Comp. AR");
	m_pa_refraction_cbox->setToolTip("Compensate for atmospheric refraction.\nSome mounts do it automatically, in this case it should be off.");
	m_pa_refraction_cbox->setEnabled(false);
	palign_frame_layout->addWidget(m_pa_refraction_cbox, palign_row, 3, 1, 1);
	connect(m_pa_refraction_cbox, &QCheckBox::clicked, this, &ImagerWindow::on_mount_agent_set_pa_refraction);

	palign_row++;
	label = new QLabel("Hour Angle move (°):");
	palign_frame_layout->addWidget(label, palign_row, 0, 1, 2);
	m_pa_move_ha = new QDoubleSpinBox();
	m_pa_move_ha->setEnabled(false);
	connect(m_pa_move_ha, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_mount_agent_set_pa_settings);
	palign_frame_layout->addWidget(m_pa_move_ha, palign_row, 2, 1, 1);

	palign_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	palign_frame_layout->addItem(spacer, palign_row, 0, 1, 4);

	palign_row++;
	toolbar = new QWidget;
	toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(0,0,0,0);
	toolbox->setContentsMargins(0,0,0,0);
	palign_frame_layout->addWidget(toolbar, palign_row, 0, 1, 4);

	m_mount_start_pa_button = new QPushButton("Start alignment");
	m_mount_start_pa_button->setEnabled(false);
	m_mount_start_pa_button->setStyleSheet("min-width: 30px");
	m_mount_start_pa_button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(m_mount_start_pa_button);
	connect(m_mount_start_pa_button, &QPushButton::clicked, this, &ImagerWindow::on_mount_polar_align);

	m_mount_recalculate_pe_button = new QPushButton("Recalculate error");
	m_mount_recalculate_pe_button->setEnabled(false);
	m_mount_recalculate_pe_button->setStyleSheet("min-width: 30px");
	m_mount_recalculate_pe_button->setIcon(QIcon(":resource/calibrate.png"));
	toolbox->addWidget(m_mount_recalculate_pe_button);
	connect(m_mount_recalculate_pe_button , &QPushButton::clicked, this, &ImagerWindow::on_mount_recalculate_polar_error);

	m_mount_pa_stop_button = new QPushButton("Stop");
	m_mount_pa_stop_button->setEnabled(false);
	m_mount_pa_stop_button->setStyleSheet("min-width: 15px");
	m_mount_pa_stop_button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(m_mount_pa_stop_button);
	connect(m_mount_pa_stop_button , &QPushButton::clicked, this, &ImagerWindow::on_mount_polar_align_stop);

	palign_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	palign_frame_layout->addItem(spacer, palign_row, 0, 1, 4);

	palign_row++;
	label = new QLabel("Polar error:");
	palign_frame_layout->addWidget(label, palign_row, 0, 1, 1);
	m_pa_error_label = new QLabel();
	m_pa_error_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	palign_frame_layout->addWidget(m_pa_error_label, palign_row, 1, 1, 3);

	palign_row++;
	label = new QLabel("Azimuth error:");
	palign_frame_layout->addWidget(label, palign_row, 0, 1, 1);
	m_pa_error_az_label = new QLabel();
	m_pa_error_az_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	palign_frame_layout->addWidget(m_pa_error_az_label, palign_row, 1, 1, 3);

	palign_row++;
	label = new QLabel("Altitude error:");
	palign_frame_layout->addWidget(label, palign_row, 0, 1, 1);
	m_pa_error_alt_label = new QLabel();
	m_pa_error_alt_label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	palign_frame_layout->addWidget(m_pa_error_alt_label, palign_row, 1, 1, 3);
}

void ImagerWindow::on_mount_polar_align_stop() {
	QtConcurrent::run([&]() {
		m_property_mutex.lock();
		static char selected_solver_agent[INDIGO_NAME_SIZE];
		if (get_selected_solver_agent(selected_solver_agent)) {
			change_solver_agent_abort(selected_solver_agent);
		}
		m_property_mutex.unlock();
	});
}

void ImagerWindow::on_mount_joystick_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_joystick[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_joystick_str = m_mount_joystick_select->currentText();
		if (q_joystick_str.compare("No joystick") == 0) {
			strcpy(selected_joystick, "NONE");
		} else {
			strncpy(selected_joystick, q_joystick_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_joystick);
		static const char * items[] = { selected_joystick };

		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_JOYSTICK_LIST_PROPERTY_NAME, 1, items, values);
	});
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
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		change_mount_agent_equatorial(selected_agent, false);
	});
}

void ImagerWindow::on_mount_sync(int index) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		change_mount_agent_equatorial(selected_agent, true);
	});
}

void ImagerWindow::on_mount_agent_set_pa_settings(double value) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_solver_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		change_solver_agent_pa_settings(selected_agent);
	});
}

void ImagerWindow::on_mount_agent_set_pa_refraction(bool clicked) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_solver_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		change_solver_agent_pa_settings(selected_agent);
	});
}

void ImagerWindow::on_mount_abort(int index) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		change_mount_agent_abort(selected_agent);
	});
}

void ImagerWindow::on_mount_track(int state) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);
		bool checked = m_mount_track_cbox->checkState();

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		if (checked) {
			indigo_change_switch_property_1(nullptr, selected_agent, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_ON_ITEM_NAME, true);
		} else {
			indigo_change_switch_property_1(nullptr, selected_agent, MOUNT_TRACKING_PROPERTY_NAME, MOUNT_TRACKING_OFF_ITEM_NAME, true);
		}
	});
}

void ImagerWindow::on_mount_park(int state) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);
		bool checked = m_mount_park_cbox->checkState();

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		if (checked) {
			indigo_change_switch_property_1(nullptr, selected_agent, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_PARKED_ITEM_NAME, true);
		} else {
			indigo_change_switch_property_1(nullptr, selected_agent, MOUNT_PARK_PROPERTY_NAME, MOUNT_PARK_UNPARKED_ITEM_NAME, true);
		}
	});
}

void ImagerWindow::on_mount_home(int state) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);
		bool checked = m_mount_home_cbox->checkState();

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		indigo_change_switch_property_1(nullptr, selected_agent, MOUNT_HOME_PROPERTY_NAME, MOUNT_HOME_ITEM_NAME, checked);
	});
}

void ImagerWindow::mount_agent_set_switch_async(char *property, char *item, bool move) {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		indigo_change_switch_property_1(nullptr, selected_agent, property, item, move);
	});
}

void ImagerWindow::on_mount_move_north() {
	mount_agent_set_switch_async(MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, true);
}

void ImagerWindow::on_mount_stop_north() {
	mount_agent_set_switch_async(MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_NORTH_ITEM_NAME, false);
}

void ImagerWindow::on_mount_move_south() {
	mount_agent_set_switch_async(MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, true);
}

void ImagerWindow::on_mount_stop_south() {
	mount_agent_set_switch_async(MOUNT_MOTION_DEC_PROPERTY_NAME, MOUNT_MOTION_SOUTH_ITEM_NAME, false);
}

void ImagerWindow::on_mount_move_east() {
	mount_agent_set_switch_async(MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, true);
}

void ImagerWindow::on_mount_stop_east() {
	mount_agent_set_switch_async(MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_EAST_ITEM_NAME, false);
}

void ImagerWindow::on_mount_move_west() {
	mount_agent_set_switch_async(MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, true);
}

void ImagerWindow::on_mount_stop_west() {
	mount_agent_set_switch_async(MOUNT_MOTION_RA_PROPERTY_NAME, MOUNT_MOTION_WEST_ITEM_NAME, false);
}

void ImagerWindow::on_mount_set_guide_rate(int state) {
	mount_agent_set_switch_async(MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_GUIDE_ITEM_NAME, true);
}

void ImagerWindow::on_mount_set_center_rate(int state) {
	mount_agent_set_switch_async(MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_CENTERING_ITEM_NAME, true);
}

void ImagerWindow::on_mount_set_find_rate(int state) {
	mount_agent_set_switch_async(MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_FIND_ITEM_NAME, true);
}

void ImagerWindow::on_mount_set_max_rate(int state) {
	mount_agent_set_switch_async(MOUNT_SLEW_RATE_PROPERTY_NAME, MOUNT_SLEW_RATE_MAX_ITEM_NAME, true);
}

void ImagerWindow::on_mount_sync_time(int state) {
	mount_agent_set_switch_async(AGENT_SET_HOST_TIME_PROPERTY_NAME, AGENT_SET_HOST_TIME_MOUNT_ITEM_NAME, state);
}

void ImagerWindow::on_mount_coord_source_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_source[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];

		strncpy(selected_source, m_mount_coord_source_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
		get_selected_mount_agent(selected_agent);
		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_source);

		indigo_change_switch_property_1(nullptr, selected_agent, AGENT_SITE_DATA_SOURCE_PROPERTY_NAME, selected_source, true);
	});
}

void ImagerWindow::on_mount_set_coordinates_to_agent() {
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		change_mount_agent_location(selected_agent, "");
	});
}

void ImagerWindow::on_mount_gps_selected(int index) {
	QtConcurrent::run([=]() {
		static char selected_gps[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_gps_str = m_mount_gps_select->currentText();
		if (q_gps_str.compare("No GPS") == 0) {
			strcpy(selected_gps, "NONE");
		} else {
			strncpy(selected_gps, q_gps_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_mount_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_gps);
		static const char * items[] = { selected_gps };

		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_GPS_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_image_right_click_ra_dec(double ra, double dec, double telescope_ra, double telescope_dec, Qt::KeyboardModifiers modifiers) {
	char message[255];

	set_text(m_mount_ra_input, indigo_dtos(telescope_ra / 15.0, "%d:%02d:%04.1f"));
	set_text(m_mount_dec_input, indigo_dtos(telescope_dec, "%d:%02d:%04.1f"));
	if (modifiers & Qt::ControlModifier || modifiers & Qt::AltModifier) {
		snprintf(
			message, 255, "Centering telescope α = %s, δ = %s (solved α = %s, δ = %s)",
			indigo_dtos(telescope_ra / 15, "%dh %02d' %04.1f\""),
			indigo_dtos(telescope_dec, "%+d° %02d' %04.1f\""),
			indigo_dtos(ra / 15, "%dh %02d' %04.1f\""),
			indigo_dtos(dec, "%+d° %02d' %04.1f\"")
		);
		window_log(message);
		on_mount_goto(0);
	} else {
		snprintf(
			message, 255, "Push Goto to center telescope α = %s, δ = %s (solved α = %s, δ = %s)",
			indigo_dtos(telescope_ra / 15, "%dh %02d' %04.1f\""),
			indigo_dtos(telescope_dec, "%+d° %02d' %04.1f\""),
			indigo_dtos(ra / 15, "%dh %02d' %04.1f\""),
			indigo_dtos(dec, "%+d° %02d' %04.1f\"")
		);
		window_log(message);
	}

}

void ImagerWindow::on_mount_precise_goto() {
	trigger_precise_goto();
}

void ImagerWindow::on_mount_solve_and_center() {
	trigger_solve_and_sync(true);
}

void ImagerWindow::on_mount_solve_and_sync() {
	trigger_solve_and_sync(false);
}

void ImagerWindow::on_mount_polar_align() {
	char selected_agent[INDIGO_NAME_SIZE];
	get_selected_solver_agent(selected_agent);
	indigo_property *property = properties.get(selected_agent, AGENT_PLATESOLVER_PA_STATE_PROPERTY_NAME);
	if (property) {
		for (int i = 0; i < property->count; i++) {
			if (client_match_item(&property->items[i], AGENT_PLATESOLVER_PA_STATE_ITEM_NAME) && (int)property->items[i].number.value != INDIGO_POLAR_ALIGN_IDLE) {
				QMessageBox msgBox(this);
				msgBox.setWindowTitle("Polar alignment");
				msgBox.setText(QString("Restart running polar alignment process?"));
				msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				msgBox.setDefaultButton(QMessageBox::No);
				if (QMessageBox::No == msgBox.exec()) {
					return;
				}
			}
		}
	}
	trigger_polar_alignment(false);
}

void ImagerWindow::on_mount_recalculate_polar_error() {
	trigger_polar_alignment(true);
}

void ImagerWindow::on_image_source2_selected(int index) {
	QString solver_source = m_solver_source_select2->currentText();
	strncpy(conf.solver_image_source2, solver_source.toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_debug("%s -> %s\n", __FUNCTION__, conf.solver_image_source2);
	write_conf();
}

void ImagerWindow::on_image_source3_selected(int index) {
	QString solver_source = m_solver_source_select3->currentText();
	strncpy(conf.solver_image_source3, solver_source.toUtf8().constData(), INDIGO_NAME_SIZE);
	indigo_debug("%s -> %s\n", __FUNCTION__, conf.solver_image_source3);
	write_conf();
}

void ImagerWindow::on_object_search_changed(const QString &obj_name) {
	QString data;
	QString name;
	char obj_name_c[INDIGO_VALUE_SIZE];
	char tooltip_c[INDIGO_VALUE_SIZE];
	m_object_list->clear();
	strncpy(obj_name_c, obj_name.toUtf8().data(), INDIGO_VALUE_SIZE);

	if (obj_name_c[0] == '\0') return;

	auto objects = m_custom_object_model->m_objects;
	for (auto i = objects.constBegin(); i != objects.constEnd(); ++i) {
		auto object = *i;
		if (object->matchObject(obj_name_c)) {
			QListWidgetItem *item = new QListWidgetItem(object->m_name);
			snprintf(
				tooltip_c,
				INDIGO_VALUE_SIZE,
				"<b>%s</b> (Custom DB object)<p>α: %s<br>δ: %s<br>Apparent magnitude: %.1f<sup>m</sup><br><nobr>Description: %s</nobr></p>\n",
				object->m_name.toUtf8().constData(),
				indigo_dtos(object->m_ra, "%d:%02d:%04.1f"),
				indigo_dtos(object->m_dec, "+%d:%02d:%04.1f"),
				object->m_mag,
				object->m_description.toUtf8().constData()
			);
			item->setToolTip(tooltip_c);
			item->setData(Qt::UserRole, object->m_name);
			m_object_list->addItem(item);
			//indigo_debug("%s -> %s = %s (custom)\n", __FUNCTION__, obj_name_c, name.toUtf8().constData());
		}
	}

	indigo_dso_entry *dso = &indigo_dso_data[0];
	while (dso->id) {
		if (
			QString(dso->id).contains(obj_name_c, Qt::CaseInsensitive) ||
			QString(dso->name).contains(obj_name_c, Qt::CaseInsensitive)
		) {
			data = QString(dso->id);
			if (dso->name[0] == '\0') {
				name = QString(dso->id);
			} else {
				name = QString(dso->id) + ", " + dso->name;
			}
			QListWidgetItem *item = new QListWidgetItem(name);
			snprintf(
				tooltip_c,
				INDIGO_VALUE_SIZE,
				"<b>%s</b> (%s)<p>α: %s<br>δ: %s<br>Apparent size: %.1f' x %.1f'<br>Apparent magnitude: %.1f<sup>m</sup><br><nobr>Names: %s</nobr></p>\n",
				dso->id,
				indigo_dso_type_description[dso->type],
				indigo_dtos(dso->ra, "%d:%02d:%04.1f"),
				indigo_dtos(dso->dec, "+%d:%02d:%04.1f"),
				dso->r1, dso->r2,
				dso->mag,
				dso->name
			);
			item->setToolTip(tooltip_c);
			item->setData(Qt::UserRole, data);
			m_object_list->addItem(item);
			//indigo_debug("%s -> %s = %s\n", __FUNCTION__, obj_name_c, name.toUtf8().constData());
		}
		dso++;
	}

	indigo_star_entry *star = &indigo_star_data[0];
	QRegExp hip_re("HIP\\d+");
	hip_re.setCaseSensitivity(Qt::CaseInsensitive);
	while (star->hip) {
		QString star_name = "HIP" + QString::number(star->hip);
		if (
			(star->name && QString(star->name).contains(obj_name_c, Qt::CaseInsensitive)) ||
			(hip_re.exactMatch(obj_name) && star_name.contains(obj_name_c, Qt::CaseInsensitive))
		) {
			data = star_name;
			if (star->name == nullptr || star->name[0] == '\0') {
				name = star_name;
			} else {
				name = star_name + ", " + star->name;
			}
			QListWidgetItem *item = new QListWidgetItem(name);
			snprintf(
				tooltip_c,
				INDIGO_VALUE_SIZE,
				"<b>HIP%d</b> (Star)<p>α: %s<br>δ: %s<br>Apparent magnitude: %.1f<sup>m</sup><br><nobr>Names: %s</nobr></p>\n",
				star->hip,
				indigo_dtos(star->ra, "%d:%02d:%04.1f"),
				indigo_dtos(star->dec, "+%d:%02d:%04.1f"),
				star->mag,
				star->name ? star->name : ""
			);
			item->setToolTip(tooltip_c);
			item->setData(Qt::UserRole, data);
			m_object_list->addItem(item);
			//indigo_debug("%s -> %s = %s\n", __FUNCTION__, obj_name_c, name.toUtf8().constData());
		}
		star++;
	}
	indigo_debug("%s -> %s\n", __FUNCTION__, obj_name.toUtf8().constData());
}

void ImagerWindow::on_object_search_entered() {
	if (m_object_list->count() == 0) return;
	m_object_list->setCurrentRow(0);
	m_object_list->setFocus();
	indigo_debug("%s -> 0\n", __FUNCTION__);
}

void ImagerWindow::on_object_clicked(QListWidgetItem *item) {
	Q_UNUSED(item);
	on_object_selected();
}

void ImagerWindow::on_object_selected() {
	QList<QListWidgetItem *> selected = m_object_list->selectedItems();
	if (selected.isEmpty()) return;
	QListWidgetItem *object = selected.at(0);
	QString object_id = object->data(Qt::UserRole).toString();

	char obj_id_c[1000];
	strcpy(obj_id_c, object_id.toUtf8().data());

	int index = m_custom_object_model->findObject(obj_id_c);
	if (index >= 0) {
		set_text(m_mount_ra_input, indigo_dtos(m_custom_object_model->m_objects.at(index)->m_ra, "%d:%02d:%04.1f"));
		set_text(m_mount_dec_input, indigo_dtos(m_custom_object_model->m_objects.at(index)->m_dec, "%d:%02d:%04.1f"));
	}

	indigo_dso_entry *dso = &indigo_dso_data[0];
	while (dso->id) {
		if (!strcmp(dso->id, obj_id_c)) {
			set_text(m_mount_ra_input, indigo_dtos(dso->ra, "%d:%02d:%04.1f"));
			set_text(m_mount_dec_input, indigo_dtos(dso->dec, "%d:%02d:%04.1f"));
			break;
		}
		dso++;
	}
	indigo_star_entry *star = &indigo_star_data[0];
	while (star->hip) {
		QString star_name = "HIP" + QString::number(star->hip);
		if (star_name == object_id) {
			set_text(m_mount_ra_input, indigo_dtos(star->ra, "%d:%02d:%04.1f"));
			set_text(m_mount_dec_input, indigo_dtos(star->dec, "%d:%02d:%04.1f"));
			break;
		}
		star++;
	}

	indigo_debug("%s -> %s = %s\n", __FUNCTION__, object->text().toUtf8().constData(), object_id.toUtf8().constData());
}

void ImagerWindow::on_custom_object_add() {
	indigo_debug("%s -> 0\n", __FUNCTION__);
	m_add_object_dialog->show();
}

void ImagerWindow::on_custom_object_added(CustomObject object) {
	indigo_debug("%s -> %s\n", __FUNCTION__, object.m_name.toUtf8().constData());
	if (m_custom_object_model->addObject(object.m_name, object.m_ra, object.m_dec, object.m_mag, object.m_description)) {
		m_custom_object_model->saveObjects();
		QString message("Object '" + object.m_name + "' added to custom database");
		window_log(message.toUtf8().data());
		m_add_object_dialog->clear();
	} else {
		QString message("Object '" + object.m_name + "' already exists in custom database");
		window_log(message.toUtf8().data(), INDIGO_ALERT_STATE);
	}
}

void ImagerWindow::on_custom_object_remove() {
	QList<QListWidgetItem *> selected = m_object_list->selectedItems();
	if (selected.isEmpty()) {
		indigo_debug("%s -> No selection\n", __FUNCTION__);
		return;
	}
	QListWidgetItem *object = selected.at(0);
	QString obj_id = object->data(Qt::UserRole).toString();

	int index = m_custom_object_model->findObject(obj_id);
	if (index < 0) {
		window_log("Only objects in custom database can be removed", INDIGO_ALERT_STATE);
		return;
	}

	int ret = QMessageBox::question(
		this,
		"Remove object",
		"Object <b>" + obj_id + "</b> is about to be removed<br><br>" + "Do you want to continue?" ,
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);

	if (ret == QMessageBox::Yes) {
		m_custom_object_model->removeObject(obj_id);
		m_custom_object_model->saveObjects();
		delete(object);
		QString message("Object '" + obj_id + "' removed");
		window_log(message.toUtf8().data());
		indigo_debug("%s -> removed %s\n", __FUNCTION__, obj_id.toUtf8().constData());
	} else {
		indigo_debug("%s -> keep %s\n", __FUNCTION__, obj_id.toUtf8().constData());
	}
}
