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
#include <libgen.h>
#include <conf.h>
#include <utils.h>

void write_conf();

void ImagerWindow::create_imager_tab(QFrame *capture_frame) {
	QGridLayout *capture_frame_layout = new QGridLayout();
	capture_frame_layout->setAlignment(Qt::AlignTop);
	capture_frame->setLayout(capture_frame_layout);
	capture_frame->setFrameShape(QFrame::StyledPanel);
	capture_frame->setMinimumWidth(TOOLBAR_MIN_WIDTH);
	capture_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	m_agent_imager_select = new QComboBox();
	capture_frame_layout->addWidget(m_agent_imager_select, row, 0, 1, 6);
	//connect(m_agent_imager_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_agent_selected);
	connect(m_agent_imager_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImagerWindow::on_agent_selected);

	// camera selection
	row++;
	QLabel *label = new QLabel("Camera:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	capture_frame_layout->addWidget(label, row, 0);
	m_camera_select = new QComboBox();
	capture_frame_layout->addWidget(m_camera_select, row, 1, 1, 5);
	connect(m_camera_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_camera_selected);

	// Filter wheel selection
	row++;
	label = new QLabel("Wheel:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	capture_frame_layout->addWidget(label, row, 0);
	m_wheel_select = new QComboBox();
	capture_frame_layout->addWidget(m_wheel_select, row, 1, 1, 5);
	connect(m_wheel_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_wheel_selected);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	capture_frame_layout->addItem(spacer, row, 0);

	// frame type
	row++;
	label = new QLabel("Frame:");
	capture_frame_layout->addWidget(label, row, 0);
	m_frame_size_select = new QComboBox();
	capture_frame_layout->addWidget(m_frame_size_select, row, 1, 1, 3);
	connect(m_frame_size_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_ccd_mode_selected);

	m_frame_type_select = new QComboBox();
	capture_frame_layout->addWidget(m_frame_type_select, row, 4, 1, 2);
	connect(m_frame_type_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_frame_type_selected);

	// Exposure time
	row++;
	label = new QLabel("Exposure (s):");
	capture_frame_layout->addWidget(label, row, 0);
	m_exposure_time = new QDoubleSpinBox();
	m_exposure_time->setDecimals(3);
	m_exposure_time->setMaximum(10000);
	m_exposure_time->setMinimum(0);
	m_exposure_time->setValue(1);
	capture_frame_layout->addWidget(m_exposure_time, row, 1, 1, 2);

	//label = new QLabel(QChar(0x0394)+QString("t:"));
	label = new QLabel("Delay (s):");
	capture_frame_layout->addWidget(label, row, 3);
	m_exposure_delay = new QDoubleSpinBox();
	m_exposure_delay->setDecimals(3);
	m_exposure_delay->setMaximum(10000);
	m_exposure_delay->setMinimum(0);
	m_exposure_delay->setValue(0);
	//m_exposure_delay->setEnabled(false);
	capture_frame_layout->addWidget(m_exposure_delay, row, 4, 1, 2);

	// Frame count
	row++;
	label = new QLabel("No frames:");
	capture_frame_layout->addWidget(label, row, 0);
	m_frame_count = new QSpinBox();
	m_frame_count->setMaximum(100000);
	m_frame_count->setMinimum(-1);
	m_frame_count->setSpecialValueText("∞");
	m_frame_count->setValue(1);
	capture_frame_layout->addWidget(m_frame_count, row, 1, 1, 2);

	label = new QLabel("Filter:");
	capture_frame_layout->addWidget(label, row, 3);
	m_filter_select = new QComboBox();
	capture_frame_layout->addWidget(m_filter_select, row, 4, 1, 2);
	connect(m_filter_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_filter_selected);

	// Frame prefix
	row++;
	label = new QLabel("Object:");
	capture_frame_layout->addWidget(label, row, 0);
	m_object_name = new QLineEdit();
	m_object_name->setPlaceholderText("Image file prefix e.g. M16, M33 ...");
	m_object_name->setToolTip("Object name or any text that will be used as a file name prefix.\nIf empty images will not be saved.");
	capture_frame_layout->addWidget(m_object_name, row, 1, 1, 5);
	connect(m_object_name, &QLineEdit::textChanged, this, &ImagerWindow::on_object_name_changed);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	capture_frame_layout->addItem(spacer, row, 0);

	//button = new QPushButton("Set");
	//button->setStyleSheet("min-width: 60px");
	//button->setIcon(QIcon(":resource/play.png"));
	//cooler_box->addWidget(button);
	// Buttons
	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	capture_frame_layout->addWidget(toolbar, row, 0, 1, 6);

	m_preview_button = new QPushButton("Preview");
	m_preview_button->setStyleSheet("min-width: 30px");
	m_preview_button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(m_preview_button);
	connect(m_preview_button, &QPushButton::clicked, this, &ImagerWindow::on_preview_start_stop);

	m_exposure_button = new QPushButton("Expose");
	m_exposure_button->setStyleSheet("min-width: 30px");
	m_exposure_button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(m_exposure_button);
	connect(m_exposure_button, &QPushButton::clicked, this, &ImagerWindow::on_exposure_start_stop);

	m_pause_button = new QPushButton("Pause");
	toolbox->addWidget(m_pause_button);
	m_pause_button->setStyleSheet("min-width: 30px");
	m_pause_button->setIcon(QIcon(":resource/pause.png"));
	connect(m_pause_button, &QPushButton::clicked, this, &ImagerWindow::on_pause);

	QPushButton *button = new QPushButton("Abort");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/stop.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_abort);

	row++;
	m_exposure_progress = new QProgressBar();
	capture_frame_layout->addWidget(m_exposure_progress, row, 0, 1, 5);
	m_exposure_progress->setFormat("Exposure: Idle");
	m_exposure_progress->setMaximum(1);
	m_exposure_progress->setValue(0);

	const int spinner_size = 32;
	m_download_label = new QLabel(this);
	m_download_label->setFixedSize(spinner_size, spinner_size);
	m_download_label->setAlignment(Qt::AlignCenter);
	m_download_label->setToolTip("Image download progress");
	m_download_label->setStyleSheet(QString("QLabel { background-color: #272727; border-radius: %1px; }").arg(spinner_size / 2));
	capture_frame_layout->addWidget(m_download_label, row, 5, 2, 1);
	m_download_spinner = new QMovie(":resource/spinner.gif");
	m_download_spinner->setScaledSize(m_download_label->size() * 0.7);
	m_download_label->setMovie(m_download_spinner);
	m_download_label->clear();

	row++;
	m_process_progress = new QProgressBar();
	capture_frame_layout->addWidget(m_process_progress, row, 0, 1, 5);
	m_process_progress->setMaximum(1);
	m_process_progress->setValue(0);
	m_process_progress->setFormat("Process: Idle");

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	capture_frame_layout->addItem(spacer, row, 0);

	row++;
	QWidget *cooler_bar = new QWidget();
	cooler_bar->setContentsMargins(0,0,0,0);

	QHBoxLayout *cooler_box = new QHBoxLayout(cooler_bar);
	cooler_box->setContentsMargins(0,0,0,0);

	capture_frame_layout->addWidget(cooler_bar, row, 0, 1, 6);
	cooler_bar->setContentsMargins(0,0,0,6);

	label = new QLabel("Cooler (°C):");
	cooler_box->addWidget(label);

	m_current_temp = new QLineEdit();
	m_current_temp->setToolTip("Current CCD Temperture");
	cooler_box->addWidget(m_current_temp);
	m_current_temp->setStyleSheet("width: 30px");
	m_current_temp->setText("");
	m_current_temp->setReadOnly(true);

	label = new QLabel("P:");
	cooler_box->addWidget(label);

	m_cooler_pwr = new QLineEdit();
	m_cooler_pwr->setToolTip("Current CCD Cooler Power");
	cooler_box->addWidget(m_cooler_pwr);
	m_cooler_pwr->setStyleSheet("width: 30px");
	m_cooler_pwr->setText("");
	m_cooler_pwr->setReadOnly(true);

	m_cooler_onoff = new QCheckBox();
	m_cooler_onoff->setToolTip("Turn CCD Cooler ON/OFF");
	cooler_box->addWidget(m_cooler_onoff);
	m_cooler_onoff->setEnabled(false);
	connect(m_cooler_onoff, &QCheckBox::toggled, this, &ImagerWindow::on_cooler_onoff);

	m_set_temp = new QDoubleSpinBox();
	m_set_temp->setToolTip("Cooler Target Temperature (°C)");
	m_set_temp->setMaximum(60);
	m_set_temp->setMinimum(-120);
	m_set_temp->setValue(0);
	m_set_temp->setEnabled(false);
	m_set_temp->setKeyboardTracking(false);
	connect(m_set_temp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ImagerWindow::on_temperature_set);

	//m_exposure_delay->setEnabled(false);
	cooler_box->addWidget(m_set_temp);


	row++;
	// Tools tabbar
	QTabWidget *capture_tabbar = new QTabWidget;
	capture_frame_layout->addWidget(capture_tabbar, row, 0, 1, 6);

	// image frame
	QFrame *image_frame = new QFrame();
	capture_tabbar->addTab(image_frame, "Image");

	QGridLayout *image_frame_layout = new QGridLayout();
	image_frame_layout->setAlignment(Qt::AlignTop);
	image_frame->setLayout(image_frame_layout);
	image_frame->setFrameShape(QFrame::StyledPanel);
	image_frame->setContentsMargins(0, 0, 0, 0);

	int image_row = 0;

	label = new QLabel("Preview exposure (s):");
	image_frame_layout->addWidget(label, image_row, 0, 1, 3);
	m_preview_exposure_time = new QDoubleSpinBox();
	m_preview_exposure_time->setDecimals(3);
	m_preview_exposure_time->setMaximum(10000);
	m_preview_exposure_time->setMinimum(0);
	m_preview_exposure_time->setValue(1);
	image_frame_layout->addWidget(m_preview_exposure_time, image_row, 3);

	image_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	image_frame_layout->addItem(spacer, image_row, 0, 1, 4);

	image_row++;
	label = new QLabel("Image format:");
	image_frame_layout->addWidget(label, image_row, 0, 1, 2);
	m_frame_format_select = new QComboBox();
	image_frame_layout->addWidget(m_frame_format_select, image_row, 2, 1, 2);
	connect(m_frame_format_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_ccd_image_format_selected);

	image_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	image_frame_layout->addItem(spacer, image_row, 0, 1, 4);

	// ROI
	image_row++;
	label = new QLabel("ROI X:");
	image_frame_layout->addWidget(label, image_row, 0);
	m_roi_x = new QSpinBox();
	m_roi_x->setMaximum(100000);
	m_roi_x->setMinimum(0);
	m_roi_x->setValue(0);
	m_roi_x->setEnabled(false);
	image_frame_layout->addWidget(m_roi_x , image_row, 1);

	label = new QLabel("Width:");
	image_frame_layout->addWidget(label, image_row, 2);
	m_roi_w = new QSpinBox();
	m_roi_w->setMaximum(100000);
	m_roi_w->setMinimum(0);
	m_roi_w->setValue(0);
	m_roi_w->setEnabled(false);
	image_frame_layout->addWidget(m_roi_w, image_row, 3);

	// ROI
	image_row++;
	label = new QLabel("ROI Y:");
	image_frame_layout->addWidget(label, image_row, 0);
	m_roi_y = new QSpinBox();
	m_roi_y->setMaximum(100000);
	m_roi_y->setMinimum(0);
	m_roi_y->setValue(0);
	m_roi_y->setEnabled(false);
	image_frame_layout->addWidget(m_roi_y , image_row, 1);

	label = new QLabel("Height:");
	image_frame_layout->addWidget(label, image_row, 2);
	m_roi_h = new QSpinBox();
	m_roi_h->setMaximum(100000);
	m_roi_h->setMinimum(0);
	m_roi_h->setValue(0);
	m_roi_h->setEnabled(false);
	image_frame_layout->addWidget(m_roi_h, image_row, 3);

	// settings
	QFrame *dither_frame = new QFrame();
	capture_tabbar->addTab(dither_frame, "Dithering");

	QGridLayout *dither_frame_layout = new QGridLayout();
	dither_frame_layout->setAlignment(Qt::AlignTop);
	dither_frame->setLayout(dither_frame_layout);
	dither_frame->setFrameShape(QFrame::StyledPanel);
	dither_frame->setContentsMargins(0, 0, 0, 0);

	int dither_row = 0;

	m_imager_dither_cbox = new QCheckBox("Enable Dithering");
	m_imager_dither_cbox->setEnabled(false);
	set_ok(m_imager_dither_cbox);
	dither_frame_layout->addWidget(m_imager_dither_cbox , dither_row, 0);
	connect(m_imager_dither_cbox , &QPushButton::clicked, this, &ImagerWindow::on_imager_dithering_enable);

	m_dither_strategy_select = new QComboBox();
	dither_frame_layout->addWidget(m_dither_strategy_select, dither_row, 1, 1, 3);
	connect(m_dither_strategy_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_dither_strategy_selected);

	dither_row++;
	label = new QLabel("Ammount (px):");
	dither_frame_layout->addWidget(label, dither_row, 0, 1, 3);
	m_dither_aggr = new QSpinBox();
	m_dither_aggr->setMaximum(100);
	m_dither_aggr->setMinimum(0);
	m_dither_aggr->setValue(0);
	m_dither_aggr->setEnabled(false);
	dither_frame_layout->addWidget(m_dither_aggr , dither_row, 3);
	connect(m_dither_aggr, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_guider_dithering_changed);

	dither_row++;
	label = new QLabel("Settle timeout (s):");
	dither_frame_layout->addWidget(label, dither_row, 0, 1, 3);
	m_dither_to = new QSpinBox();
	m_dither_to->setMaximum(100);
	m_dither_to->setMinimum(0);
	m_dither_to->setValue(0);
	m_dither_to->setEnabled(false);
	dither_frame_layout->addWidget(m_dither_to, dither_row, 3);
	connect(m_dither_to, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_guider_dithering_changed);

	dither_row++;
	label = new QLabel("Skip frames:");
	dither_frame_layout->addWidget(label, dither_row, 0, 1, 3);
	m_dither_skip = new QSpinBox();
	m_dither_skip->setMaximum(100);
	m_dither_skip->setMinimum(0);
	m_dither_skip->setValue(0);
	m_dither_skip->setEnabled(false);
	dither_frame_layout->addWidget(m_dither_skip, dither_row, 3);
	connect(m_dither_skip, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_imager_dithering_changed);

	// settings
	QFrame *camera_frame = new QFrame();
	capture_tabbar->addTab(camera_frame, "Camera");

	QGridLayout *camera_frame_layout = new QGridLayout();
	camera_frame_layout->setAlignment(Qt::AlignTop);
	camera_frame->setLayout(camera_frame_layout);
	camera_frame->setFrameShape(QFrame::StyledPanel);
	camera_frame->setContentsMargins(0, 0, 0, 0);

	int camera_row = 0;
	label = new QLabel("Gain:");
	camera_frame_layout->addWidget(label, camera_row, 0, 1, 3);
	m_imager_gain = new QSpinBox();
	m_imager_gain->setEnabled(false);
	camera_frame_layout->addWidget(m_imager_gain, camera_row, 3);
	connect(m_imager_gain, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_imager_gain_changed);

	camera_row++;
	label = new QLabel("Offset:");
	camera_frame_layout->addWidget(label, camera_row, 0, 1, 3);
	m_imager_offset = new QSpinBox();
	m_imager_offset->setEnabled(false);
	camera_frame_layout->addWidget(m_imager_offset, camera_row, 3);
	connect(m_imager_offset, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_imager_offset_changed);

	camera_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	camera_frame_layout->addItem(spacer, camera_row, 0, 1, 4);

	camera_row++;
	label = new QLabel("X Binning:");
	camera_frame_layout->addWidget(label, camera_row, 0);
	m_imager_bin_x = new QSpinBox();
	m_imager_bin_x->setEnabled(false);
	camera_frame_layout->addWidget(m_imager_bin_x, camera_row, 1);
	label = new QLabel("Y Binning:");
	camera_frame_layout->addWidget(label, camera_row, 2);
	m_imager_bin_y = new QSpinBox();
	m_imager_bin_y->setEnabled(false);
	camera_frame_layout->addWidget(m_imager_bin_y, camera_row, 3);
	connect(m_imager_bin_x, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_imager_binning_changed);
	connect(m_imager_bin_y, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImagerWindow::on_agent_imager_binning_changed);

	// Remote files
	QFrame *remote_files_frame = new QFrame();
	capture_tabbar->addTab(remote_files_frame, "Remote images");

	QGridLayout *remote_files_frame_layout = new QGridLayout();
	remote_files_frame_layout->setAlignment(Qt::AlignTop);
	remote_files_frame->setLayout(remote_files_frame_layout);
	remote_files_frame->setFrameShape(QFrame::StyledPanel);
	remote_files_frame->setContentsMargins(0, 0, 0, 0);

	int remote_files_row = 0;
	m_save_image_on_server_cbox = new QCheckBox("Save image copies on the server");
	m_save_image_on_server_cbox->setToolTip("Save images copies on server");
	m_save_image_on_server_cbox->setEnabled(true);
	m_save_image_on_server_cbox->setChecked(conf.save_images_on_server);
	remote_files_frame_layout->addWidget(m_save_image_on_server_cbox, remote_files_row, 0, 1, 4);
	connect(m_save_image_on_server_cbox, &QPushButton::clicked, this, &ImagerWindow::on_save_image_on_server);

	remote_files_row++;
	m_keep_image_on_server_cbox = new QCheckBox("Keep downloaded images on server");
	m_keep_image_on_server_cbox->setToolTip("Do not remove downloaded image copies from server");
	m_keep_image_on_server_cbox->setEnabled(true);
	m_keep_image_on_server_cbox->setChecked(conf.keep_images_on_server);
	remote_files_frame_layout->addWidget(m_keep_image_on_server_cbox, remote_files_row, 0, 1, 4);
	connect(m_keep_image_on_server_cbox, &QPushButton::clicked, this, &ImagerWindow::on_keep_image_on_server);

	remote_files_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	remote_files_frame_layout->addItem(spacer, remote_files_row, 0, 1, 4);

	remote_files_row++;
	m_sync_files_button = new QPushButton("Download images");
	m_sync_files_button->setToolTip("Download available remote images");
	m_sync_files_button->setStyleSheet("min-width: 30px");
//	m_check_files_button->setIcon(QIcon(":resource/play.png"));
	remote_files_frame_layout->addWidget(m_sync_files_button, remote_files_row, 0, 1, 2);
	connect(m_sync_files_button, &QPushButton::clicked, this, &ImagerWindow::on_sync_remote_files);

	m_remove_synced_files_button = new QPushButton("Server cleanup");
	m_remove_synced_files_button->setToolTip("Remove downloaded image copies from server");
	m_remove_synced_files_button->setStyleSheet("min-width: 30px");
//	m_check_files_button->setIcon(QIcon(":resource/play.png"));
	remote_files_frame_layout->addWidget(m_remove_synced_files_button, remote_files_row, 2, 1, 2);
	connect(m_remove_synced_files_button, &QPushButton::clicked, this, &ImagerWindow::on_remove_synced_remote_files);

	remote_files_row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	remote_files_frame_layout->addItem(spacer, remote_files_row, 0, 1, 4);

	remote_files_row++;
	m_download_progress = new QProgressBar();
	remote_files_frame_layout->addWidget(m_download_progress, remote_files_row, 0, 1, 4);
	m_download_progress->setMaximum(1);
	m_download_progress->setValue(0);
	m_download_progress->setFormat("Download progress");
}

void ImagerWindow::exposure_start_stop(bool clicked, bool is_sequence) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_imager_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_imager_agent);
		static char selected_scripting_agent[INDIGO_NAME_SIZE];
		get_selected_scripting_agent(selected_scripting_agent);
		indigo_debug("start_stop: %s %s", selected_imager_agent, selected_scripting_agent);

		// Stop sequence or exposure if running
		if (is_sequence) {
			indigo_property *agent_sequence_state = properties.get(selected_scripting_agent, "SEQUENCE_STATE");
			if (agent_sequence_state && agent_sequence_state->state == INDIGO_BUSY_STATE) {
				change_agent_abort_process_property(selected_imager_agent);
				indigo_error("Sequence is running, aborting it.");
				return;
			}
		} else {
			indigo_property *agent_start_process = properties.get(selected_imager_agent, AGENT_START_PROCESS_PROPERTY_NAME);
			if (agent_start_process && agent_start_process->state == INDIGO_BUSY_STATE ) {
				change_agent_abort_process_property(selected_scripting_agent);
				indigo_error("Exposure is running, aborting it.");
				return;
			}
		}

		// Start sequence or exposure
		set_related_mount_and_imager_agents();
		set_related_imager_and_guider_agents();

		m_object_name_str = m_object_name->text().trimmed();
		//add_fits_keyword_string(selected_imager_agent, "OBJECT", m_object_name_str);
		change_agent_batch_property(selected_imager_agent);
		change_ccd_frame_property(selected_imager_agent);
		change_ccd_localmode_property(selected_imager_agent, m_object_name_str);
		if(conf.save_images_on_server) {
			change_ccd_upload_property(selected_imager_agent, CCD_UPLOAD_MODE_BOTH_ITEM_NAME);
		} else {
			change_ccd_upload_property(selected_imager_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
		}
		if (is_sequence) {
			int approx_time = m_sequence_editor2->totalExposure();
			if (approx_time >= 0) {
				static char end_time[256];
				get_time_after(end_time, approx_time, "Optimistic time of sequence completion: %d %b %H:%M");
				Logger::instance().log(nullptr, end_time);
			}

			// set aditional agent reations needed for sequence
			static char selected_mount_agent[INDIGO_NAME_SIZE];
			get_selected_mount_agent(selected_mount_agent);
			set_related_solver_agent(selected_imager_agent, "Imager Agent");
			set_related_solver_agent(selected_mount_agent, "Mount Agent");

			change_scripting_agent_sequence(
				selected_scripting_agent,
				m_sequence_editor2->makeScriptFromView()
			);
			change_agent_focus_params_property(selected_imager_agent, false);
			change_agent_start_sequence_property(selected_scripting_agent);
		} else {
			change_agent_start_exposure_property(selected_imager_agent);
		}
	});
}

void ImagerWindow::on_exposure_start_stop(bool clicked) {
	exposure_start_stop(clicked, false);
}

void ImagerWindow::on_preview_start_stop(bool clicked) {
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
			set_related_mount_and_imager_agents();
			set_related_imager_and_guider_agents();
			QString obj_name = m_object_name->text();
			add_fits_keyword_string(selected_agent, "OBJECT", obj_name);
			change_agent_batch_property(selected_agent);
			change_ccd_frame_property(selected_agent);
			change_ccd_upload_property(selected_agent, CCD_UPLOAD_MODE_CLIENT_ITEM_NAME);
			change_ccd_exposure_property(selected_agent, m_preview_exposure_time);
		}
	});
}

void ImagerWindow::on_abort(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_scripting_agent[INDIGO_NAME_SIZE];
		get_selected_scripting_agent(selected_scripting_agent);

		indigo_property *p = properties.get(selected_scripting_agent, "SEQUENCE_STATE");
		if(p && p->state == INDIGO_BUSY_STATE) {
			change_agent_abort_process_property(selected_scripting_agent);
			indigo_debug("Sequence is running, aborting it.");
		} else {
			static char selected_imager_agent[INDIGO_NAME_SIZE];
			get_selected_imager_agent(selected_imager_agent);

			indigo_property *p = properties.get(selected_imager_agent, AGENT_START_PROCESS_PROPERTY_NAME);
			if (p && p->state == INDIGO_BUSY_STATE ) {
				change_agent_abort_process_property(selected_imager_agent);
				indigo_debug("Process is running, aborting it.");
			} else {
				change_ccd_abort_exposure_property(selected_imager_agent);
				indigo_debug("Exposure is running, aborting it.");
			}
		}
	});
}

void ImagerWindow::on_pause(bool clicked) {
	Q_UNUSED(clicked);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);

		//QPushButton *button = (QPushButton *)sender();
		//button->setText("Continue");

		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		indigo_property *p = properties.get(selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME);
		if (p == nullptr || p->count < 1) return;

		change_agent_pause_process_property(selected_agent, true);
	});
}

void ImagerWindow::on_agent_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		// Clear controls
		indigo_property *property = (indigo_property*)malloc(sizeof(indigo_property));
		memset(property, 0, sizeof(indigo_property));
		get_selected_imager_agent(property->device);
		property_delete(property, nullptr);
		properties.remove(property);
		free(property);

		// populate them again with the new values
		// use cache instead of enumeration request
		/*
		property_cache::iterator i = properties.begin();
		while (i != properties.end()) {
			indigo_property *property = i.value();
			QString key = i.key();
			if (property != nullptr) {
				indigo_debug("property: %s(%s) == %p\n", __FUNCTION__, key.toUtf8().constData(), property);
				property_define(property, nullptr);
			} else {
				indigo_debug("property: %s(%s) == EMPTY\n", __FUNCTION__, key.toUtf8().constData());
			}
			i++;
		} */
		indigo_enumerate_properties(nullptr, &INDIGO_ALL_PROPERTIES);
	});
}

void ImagerWindow::on_camera_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_camera[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_camera_str = m_camera_select->currentText();
		if (q_camera_str.compare("No camera") == 0) {
			strcpy(selected_camera, "NONE");
		} else {
			strncpy(selected_camera, q_camera_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_camera);
		static const char * items[] = { selected_camera };
		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_wheel_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_wheel[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
		QString q_wheel_str = m_wheel_select->currentText();
		if (q_wheel_str.compare("No wheel") == 0) {
			strcpy(selected_wheel, "NONE");
		} else {
			strncpy(selected_wheel, q_wheel_str.toUtf8().constData(), INDIGO_NAME_SIZE);
		}
		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_wheel);
		static const char * items[] = { selected_wheel };

		static bool values[] = { true };
		indigo_change_switch_property(nullptr, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME, 1, items, values);
	});
}

void ImagerWindow::on_ccd_mode_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_ccd_mode_property(selected_agent, m_frame_size_select);
	});
}


void ImagerWindow::on_ccd_image_format_selected(int index) {
	Q_UNUSED(index);
	if (m_frame_format_select->currentData().toString() == "RAW") {
		window_log("Warning: Indigo RAW format is for internal use, aquired images will not be auto saved.");
	}
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_ccd_image_format_property(selected_agent);
	});
}


void ImagerWindow::on_frame_type_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_ccd_frame_type_property(selected_agent);
	});
}


void ImagerWindow::on_imager_dithering_enable(int state) {
	Q_UNUSED(state);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);
		bool checked = m_imager_dither_cbox->checkState();

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);

		indigo_change_switch_property_1(nullptr, selected_agent, AGENT_PROCESS_FEATURES_PROPERTY_NAME, AGENT_IMAGER_ENABLE_DITHERING_FEATURE_ITEM_NAME, checked);
	});
}

void ImagerWindow::on_dither_strategy_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[MODE CHANGE SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_guider_ditherung_strategy_property(selected_agent);
	});
}

void ImagerWindow::on_agent_guider_dithering_changed(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_guider_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_guider_dithering_property(selected_agent);
	});
}

void ImagerWindow::on_agent_imager_dithering_changed(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_imager_dithering_property(selected_agent);
	});
}

void ImagerWindow::on_agent_imager_gain_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_gain_property(selected_agent, m_imager_gain);
	});
}

void ImagerWindow::on_agent_imager_offset_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_offset_property(selected_agent, m_imager_offset);
	});
}

void ImagerWindow::on_agent_imager_binning_changed(int value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_agent_binning_property(selected_agent);
	});
}

void ImagerWindow::on_filter_selected(int index) {
	Q_UNUSED(index);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_wheel_slot_property(selected_agent);
	});
}

void ImagerWindow::on_cooler_onoff(bool state) {
	Q_UNUSED(state);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];

		get_selected_imager_agent(selected_agent);

		indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
		change_cooler_onoff_property(selected_agent);
	});
}

void ImagerWindow::on_temperature_set(double value) {
	Q_UNUSED(value);
	QtConcurrent::run([=]() {
		indigo_debug("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_ccd_temperature_property(selected_agent);
	});
}

void ImagerWindow::on_object_name_changed(const QString &object_name) {
	if (m_is_sequence) {
		return;
	}
	QtConcurrent::run([=]() {
		indigo_error("CALLED: %s\n", __FUNCTION__);
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_imager_agent(selected_agent);

		change_ccd_localmode_property(selected_agent, object_name);
		add_fits_keyword_string(selected_agent, "OBJECT", object_name);
	});
}

void ImagerWindow::on_save_image_on_server(int state) {
	Q_UNUSED(state);
	conf.save_images_on_server = m_save_image_on_server_cbox->checkState();
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_keep_image_on_server(int state) {
	Q_UNUSED(state);
	conf.keep_images_on_server = m_keep_image_on_server_cbox->checkState();
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_sync_remote_files(bool clicked) {
	Q_UNUSED(clicked);
	if (!conf.keep_images_on_server) {
		remove_synced_remote_files();
	}
	sync_remote_files();
}

void ImagerWindow::on_remove_synced_remote_files(bool clicked) {
	Q_UNUSED(clicked);
	if (!conf.keep_images_on_server) {
		remove_synced_remote_files();
	} else {
		window_log("Error: Can not remove images if keep images is enabled");
	}
}


void ImagerWindow::sync_remote_files() {
	char message[PATH_LEN];
	char work_dir[PATH_LEN];


	if (!m_files_to_download.empty()) {
		m_download_progress->setFormat("Download canceled %v of %m");
		m_files_to_download.clear();
		return;
	}

	m_download_progress->setFormat("Preparing download...");
	QCoreApplication::processEvents();

	get_current_output_dir(work_dir, conf.data_dir_prefix);
	QString work_dir_str(dirname(work_dir));
	SyncUtils sutil(work_dir_str);
	sutil.rebuild();
	m_files_to_download.clear();
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_imager_agent(selected_agent);

	indigo_property *p = properties.get(selected_agent, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (sutil.needs_sync(p->items[i].name)) {
				m_files_to_download.append(p->items[i].name);
				indigo_debug("Remote file: %s", p->items[i].name);
			} else {
				indigo_debug("Downloaded:  %s", p->items[i].name);
			}

		}
	}

	if (!m_files_to_download.empty()) {
		snprintf(message, sizeof(message), "Downloading %d images from server", m_files_to_download.length());
		m_download_progress->setRange(0, m_files_to_download.length());
		m_download_progress->setValue(0);
		m_download_progress->setFormat("Downloading %v of %m images...");
		window_log(message, INDIGO_OK_STATE);
		QtConcurrent::run([=]() {
			char agent[INDIGO_VALUE_SIZE];
			get_selected_imager_agent(agent);
			QString next_file = m_files_to_download.at(0);
			request_file_download(agent, next_file.toUtf8().constData());
		});
	} else {
		m_download_progress->setRange(0, 1);
		m_download_progress->setValue(0);
		m_download_progress->setFormat("No images to download");
		window_log("No images to download");
	}
}

void ImagerWindow::remove_synced_remote_files() {
	char message[PATH_LEN];
	char work_dir[PATH_LEN];
	get_current_output_dir(work_dir, conf.data_dir_prefix);
	QString work_dir_str(dirname(work_dir));
	SyncUtils sutil(work_dir_str);
	sutil.rebuild();
	m_files_to_remove.clear();
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_imager_agent(selected_agent);

	indigo_property *p = properties.get(selected_agent, AGENT_IMAGER_DOWNLOAD_FILES_PROPERTY_NAME);
	if (p) {
		for (int i = 0; i < p->count; i++) {
			if (!sutil.needs_sync(p->items[i].name) && sutil.syncable(p->items[i].name)) {
				m_files_to_remove.append(p->items[i].name);
				indigo_debug("To remove: %s", p->items[i].name);
			} else {
				indigo_debug("To keep:   %s", p->items[i].name);
			}
		}
	}
	if (!m_files_to_remove.empty()) {
		int file_num = m_files_to_remove.length();
		snprintf(message, sizeof(message), "Removing %d downloaded images from server", file_num);
		window_log(message, INDIGO_OK_STATE);
		QtConcurrent::run([=]() {
			for (int i = 0; i < file_num; i++) {
				static char agent[INDIGO_NAME_SIZE];
				get_selected_imager_agent(agent);
				QString next_file = m_files_to_remove.at(i);
				indigo_debug("Remove:  %s", next_file.toUtf8().constData());
				request_file_remove(agent, next_file.toUtf8().constData());
			}
			m_files_to_remove.clear();
		});
	} else {
		window_log("No downloaded images to remove");
	}
}
