#include "imagerwindow.h"
#include "propertycache.h"
#include "conf.h"

void ImagerWindow::crate_imager_tab(QFrame *capture_frame) {
	QGridLayout *capture_frame_layout = new QGridLayout();
	capture_frame_layout->setAlignment(Qt::AlignTop);
	capture_frame->setLayout(capture_frame_layout);
	capture_frame->setFrameShape(QFrame::StyledPanel);
	capture_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	capture_frame->setContentsMargins(0, 0, 0, 0);

	int row = 0;
	// camera selection
	QLabel *label = new QLabel("Camera:");
	capture_frame_layout->addWidget(label, row, 0);
	m_camera_select = new QComboBox();
	capture_frame_layout->addWidget(m_camera_select, row, 1, 1, 3);
	connect(m_camera_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_camera_selected);

	// Filter wheel selection
	row++;
	label = new QLabel("Wheel:");
	capture_frame_layout->addWidget(label, row, 0);
	m_wheel_select = new QComboBox();
	capture_frame_layout->addWidget(m_wheel_select, row, 1, 1, 3);
	connect(m_wheel_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_wheel_selected);

	//row++;
	//QFrame* line = new QFrame();
	//line->setFrameShape(QFrame::HLine);
	//line->setFrameShadow(QFrame::Plain);
	//capture_frame_layout->addWidget(line, row, 0, 1, 4);

	// frame type
	row++;
	label = new QLabel("Frame:");
	capture_frame_layout->addWidget(label, row, 0);
	m_frame_size_select = new QComboBox();
	capture_frame_layout->addWidget(m_frame_size_select, row, 1, 1, 2);
	connect(m_frame_size_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_ccd_mode_selected);
	m_frame_type_select = new QComboBox();
	capture_frame_layout->addWidget(m_frame_type_select, row, 3);
	connect(m_frame_type_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_frame_type_selected);

	// ROI
	row++;
	label = new QLabel("ROI X:");
	capture_frame_layout->addWidget(label, row, 0);
	m_roi_x = new QSpinBox();
	m_roi_x->setMaximum(100000);
	m_roi_x->setMinimum(0);
	m_roi_x->setValue(0);
	m_roi_x->setEnabled(false);
	capture_frame_layout->addWidget(m_roi_x , row, 1);

	label = new QLabel("W:");
	capture_frame_layout->addWidget(label, row, 2);
	m_roi_w = new QSpinBox();
	m_roi_w->setMaximum(100000);
	m_roi_w->setMinimum(0);
	m_roi_w->setValue(0);
	m_roi_w->setEnabled(false);
	capture_frame_layout->addWidget(m_roi_w, row, 3);

	// ROI
	row++;
	label = new QLabel("ROI Y:");
	capture_frame_layout->addWidget(label, row, 0);
	m_roi_y = new QSpinBox();
	m_roi_y->setMaximum(100000);
	m_roi_y->setMinimum(0);
	m_roi_y->setValue(0);
	m_roi_y->setEnabled(false);
	capture_frame_layout->addWidget(m_roi_y , row, 1);

	label = new QLabel("H:");
	capture_frame_layout->addWidget(label, row, 2);
	m_roi_h = new QSpinBox();
	m_roi_h->setMaximum(100000);
	m_roi_h->setMinimum(0);
	m_roi_h->setValue(0);
	m_roi_h->setEnabled(false);
	capture_frame_layout->addWidget(m_roi_h, row, 3);

	// Exposure time
	row++;
	label = new QLabel("Exposure (s):");
	capture_frame_layout->addWidget(label, row, 0);
	m_exposure_time = new QDoubleSpinBox();
	m_exposure_time->setMaximum(10000);
	m_exposure_time->setMinimum(0);
	m_exposure_time->setValue(1);
	capture_frame_layout->addWidget(m_exposure_time, row, 1);

	//label = new QLabel(QChar(0x0394)+QString("t:"));
	label = new QLabel("Delay (s):");
	capture_frame_layout->addWidget(label, row, 2);
	m_exposure_delay = new QDoubleSpinBox();
	m_exposure_delay->setMaximum(10000);
	m_exposure_delay->setMinimum(0);
	m_exposure_delay->setValue(0);
	//m_exposure_delay->setEnabled(false);
	capture_frame_layout->addWidget(m_exposure_delay, row, 3);

	// Frame count
	row++;
	label = new QLabel("No frames:");
	capture_frame_layout->addWidget(label, row, 0);
	m_frame_count = new QSpinBox();
	m_frame_count->setMaximum(100000);
	m_frame_count->setMinimum(-1);
	m_frame_count->setValue(1);
	capture_frame_layout->addWidget(m_frame_count, row, 1);

	label = new QLabel("Filter:");
	capture_frame_layout->addWidget(label, row, 2);
	m_filter_select = new QComboBox();
	capture_frame_layout->addWidget(m_filter_select, row, 3);
	connect(m_filter_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_filter_selected);

	// Frame prefix
	row++;
	label = new QLabel("Object:");
	capture_frame_layout->addWidget(label, row, 0);
	m_object_name = new QLineEdit();
	capture_frame_layout->addWidget(m_object_name, row, 1, 1, 3);

	// Cooler
	row++;
	QFrame *line = new QFrame();
	line->setFrameShape(QFrame::HLine);
	line->setFrameShadow(QFrame::Plain);
	capture_frame_layout->addWidget(line, row, 0, 1, 4);

	row++;
	QWidget *cooler_bar = new QWidget();
	cooler_bar->setContentsMargins(0,0,0,0);

	QHBoxLayout *cooler_box = new QHBoxLayout(cooler_bar);
	cooler_box->setContentsMargins(0,0,0,0);

	capture_frame_layout->addWidget(cooler_bar, row, 0, 1, 4);
	cooler_bar->setContentsMargins(0,0,0,6);

	label = new QLabel("Cooler (C):");
	cooler_box->addWidget(label);

	m_current_temp = new QLineEdit();
	cooler_box->addWidget(m_current_temp);
	m_current_temp->setStyleSheet("width: 30px");
	m_current_temp->setText("");
	m_current_temp->setEnabled(false);

	label = new QLabel("P:");
	cooler_box->addWidget(label);

	m_cooler_pwr = new QLineEdit();
	cooler_box->addWidget(m_cooler_pwr);
	m_cooler_pwr->setStyleSheet("width: 30px");
	m_cooler_pwr->setText("");
	m_cooler_pwr->setEnabled(false);

	m_cooler_onoff = new QCheckBox();
	cooler_box->addWidget(m_cooler_onoff);
	m_cooler_onoff->setEnabled(false);
	connect(m_cooler_onoff, &QCheckBox::toggled, this, &ImagerWindow::on_cooler_onoff);

	m_set_temp = new QDoubleSpinBox();
	m_set_temp->setMaximum(60);
	m_set_temp->setMinimum(-120);
	m_set_temp->setValue(0);
	m_set_temp->setEnabled(false);
	connect(m_set_temp, &QDoubleSpinBox::editingFinished, this, &ImagerWindow::on_teperature_set);
	//m_exposure_delay->setEnabled(false);
	cooler_box->addWidget(m_set_temp);

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
	capture_frame_layout->addWidget(toolbar, row, 0, 1, 4);


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

	button = new QPushButton("Start");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/record.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_start);

	button = new QPushButton("Preview");
	button->setStyleSheet("min-width: 30px");
	button->setIcon(QIcon(":resource/play.png"));
	toolbox->addWidget(button);
	connect(button, &QPushButton::clicked, this, &ImagerWindow::on_preview);

	row++;
	m_exposure_progress = new QProgressBar();
	capture_frame_layout->addWidget(m_exposure_progress, row, 0, 1, 4);
	m_exposure_progress->setFormat("Exposure: Idle");
	m_exposure_progress->setMaximum(1);
	m_exposure_progress->setValue(0);

	row++;
	m_process_progress = new QProgressBar();
	capture_frame_layout->addWidget(m_process_progress, row, 0, 1, 4);
	m_process_progress->setMaximum(1);
	m_process_progress->setValue(0);
	m_process_progress->setFormat("Process: Idle");
}


void ImagerWindow::on_preview(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	change_ccd_frame_property(selected_agent);
	change_ccd_exposure_property(selected_agent);
	m_preview = true;
}


void ImagerWindow::on_start(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	change_agent_batch_property(selected_agent);
	change_ccd_frame_property(selected_agent);
	change_agent_start_exposure_property(selected_agent);
	m_preview = false;
}

void ImagerWindow::on_abort(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	if (m_preview) {
		change_ccd_abort_exposure_property(selected_agent);
	} else {
		change_agent_abort_process_property(selected_agent);
	}
}

void ImagerWindow::on_pause(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);

	//QPushButton *button = (QPushButton *)sender();
	//button->setText("Continue");

	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	indigo_property *p = properties.get(selected_agent, AGENT_PAUSE_PROCESS_PROPERTY_NAME);
	if (p == nullptr || p->count != 1) return;

	change_agent_pause_process_property(selected_agent);
}

void ImagerWindow::on_camera_selected(int index) {
	static char selected_camera[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
	QString q_camera_str = m_camera_select->currentText();
	int idx = q_camera_str.indexOf(" @ ");
	if (idx >=0) q_camera_str.truncate(idx);
	if (q_camera_str.compare("No camera") == 0) {
		strcpy(selected_camera, "NONE");
	} else {
		strncpy(selected_camera, q_camera_str.toUtf8().constData(), INDIGO_NAME_SIZE);
	}
	strncpy(selected_agent, m_camera_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);

	indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_camera);
	static const char * items[] = { selected_camera };
	static bool values[] = { true };
	indigo_change_switch_property(nullptr, selected_agent, FILTER_CCD_LIST_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::on_wheel_selected(int index) {
	static char selected_wheel[INDIGO_NAME_SIZE], selected_agent[INDIGO_NAME_SIZE];
	QString q_wheel_str = m_wheel_select->currentText();
	int idx = q_wheel_str.indexOf(" @ ");
	if (idx >=0) q_wheel_str.truncate(idx);
	if (q_wheel_str.compare("No wheel") == 0) {
		strcpy(selected_wheel, "NONE");
	} else {
		strncpy(selected_wheel, q_wheel_str.toUtf8().constData(), INDIGO_NAME_SIZE);
	}
	strncpy(selected_agent, m_wheel_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);

	indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_wheel);
	static const char * items[] = { selected_wheel };

	static bool values[] = { true };
	indigo_change_switch_property(nullptr, selected_agent, FILTER_WHEEL_LIST_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::on_ccd_mode_selected(int index) {
	static char selected_agent[INDIGO_NAME_SIZE];

	get_selected_agent(selected_agent);

	indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
	change_ccd_mode_property(selected_agent);
}

void ImagerWindow::on_frame_type_selected(int index) {
	static char selected_agent[INDIGO_NAME_SIZE];

	get_selected_agent(selected_agent);

	indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
	change_ccd_frame_type_property(selected_agent);
}

void ImagerWindow::on_filter_selected(int index) {
	static char selected_agent[INDIGO_NAME_SIZE];

	get_selected_agent(selected_agent);

	indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
	change_wheel_slot_property(selected_agent);
}

void ImagerWindow::on_cooler_onoff(bool state) {
	static char selected_agent[INDIGO_NAME_SIZE];

	get_selected_agent(selected_agent);

	indigo_debug("[SELECTED] %s '%s'\n", __FUNCTION__, selected_agent);
	change_cooler_onoff_property(selected_agent);
}

void ImagerWindow::on_teperature_set() {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static char selected_agent[INDIGO_NAME_SIZE];
	get_selected_agent(selected_agent);

	change_ccd_temperature_property(selected_agent);
}
