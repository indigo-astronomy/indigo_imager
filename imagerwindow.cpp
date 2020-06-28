// Copyright (c) 2020 Rumen G.Bogdanovski & David Hulse
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

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "imagerwindow.h"
#include "qservicemodel.h"
#include "indigoclient.h"
#include "propertycache.h"
#include "qindigoservers.h"
#include "blobpreview.h"
#include "logger.h"
#include "conf.h"
#include "version.h"
#include "image-viewer.h"

void write_conf();

pal::ImageViewer *m_viewer;

ImagerWindow::ImagerWindow(QWidget *parent) : QMainWindow(parent) {
	setWindowTitle(tr("Ain INDIGO Imager"));
	resize(1200, 768);

	QIcon icon(":resource/appicon.png");
	this->setWindowIcon(icon);

	QFile f(":resource/control_panel.qss");
	f.open(QFile::ReadOnly | QFile::Text);
	QTextStream ts(&f);
	this->setStyleSheet(ts.readAll());
	f.close();

	mIndigoServers = new QIndigoServers(this);

	m_preview = true;

	//  Set central widget of window
	QWidget *central = new QWidget;
	setCentralWidget(central);

	//  Set the root layout to be a VBox
	QVBoxLayout *rootLayout = new QVBoxLayout;
	rootLayout->setSpacing(0);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
	central->setLayout(rootLayout);

	//  Create log viewer
	mLog = new QPlainTextEdit;
	mLog->setReadOnly(true);

	// Create menubar
	QMenuBar *menu_bar = new QMenuBar;
	QMenu *menu = new QMenu("&File");
	QAction *act;

	act = menu->addAction(tr("&Manage Services"));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_servers_act);

	menu->addSeparator();

	act = menu->addAction(tr("&Exit"));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_exit_act);

	menu_bar->addMenu(menu);

	menu = new QMenu("&Edit");
	act = menu->addAction(tr("Clear &Messages"));
	connect(act, &QAction::triggered, mLog, &QPlainTextEdit::clear);
	menu_bar->addMenu(menu);

	menu = new QMenu("&Settings");

	act = menu->addAction(tr("Ebable &BLOBs"));
	act->setCheckable(true);
	act->setChecked(conf.blobs_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_blobs_changed);

	act = menu->addAction(tr("Enable auto &connect"));
	act->setCheckable(true);
	act->setChecked(conf.auto_connect);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_bonjour_changed);

	act = menu->addAction(tr("&Use host suffix"));
	act->setCheckable(true);
	act->setChecked(conf.indigo_use_host_suffix);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_use_suffix_changed);

	act = menu->addAction(tr("Use property state &icons"));
	act->setCheckable(true);
	act->setChecked(conf.use_state_icons);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_use_state_icons_changed);

	act = menu->addAction(tr("Use locale specific &decimal separator"));
	act->setCheckable(true);
	act->setChecked(conf.use_system_locale);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_use_system_locale_changed);

	menu->addSeparator();
	QActionGroup *stretch_group = new QActionGroup(this);
	stretch_group->setExclusive(true);

	act = menu->addAction("Preview Levels Stretch: N&one");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_NONE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_no_stretch);
	stretch_group->addAction(act);

	act = menu->addAction("Preview Levels Stretch: &Normal");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_NORMAL) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_normal_stretch);
	stretch_group->addAction(act);

	act = menu->addAction("Preview Levels Stretch: &Hard");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_HARD) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_hard_stretch);
	stretch_group->addAction(act);

	menu->addSeparator();
	QActionGroup *log_group = new QActionGroup(this);
	log_group->setExclusive(true);

	act = menu->addAction("Log &Error");
	act->setCheckable(true);
	if (conf.indigo_log_level == INDIGO_LOG_ERROR) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_log_error);
	log_group->addAction(act);

	act = menu->addAction("Log &Info");
	act->setCheckable(true);
	if (conf.indigo_log_level == INDIGO_LOG_INFO) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_log_info);
	log_group->addAction(act);

	act = menu->addAction("Log &Debug");
	act->setCheckable(true);
	if (conf.indigo_log_level == INDIGO_LOG_DEBUG) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_log_debug);
	log_group->addAction(act);

	act = menu->addAction("Log &Trace");
	act->setCheckable(true);
	if (conf.indigo_log_level == INDIGO_LOG_TRACE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_log_trace);
	log_group->addAction(act);

	menu_bar->addMenu(menu);

	menu = new QMenu("&Help");

	act = menu->addAction(tr("&About"));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_about_act);
	menu_bar->addMenu(menu);

	rootLayout->addWidget(menu_bar);

	// Create properties viewing area
	QWidget *view = new QWidget;
	QVBoxLayout *propertyLayout = new QVBoxLayout;
	propertyLayout->setSpacing(5);
	propertyLayout->setContentsMargins(5, 5, 5, 5);
	propertyLayout->setSizeConstraint(QLayout::SetMinimumSize);
	view->setLayout(propertyLayout);
	rootLayout->addWidget(view);

	QWidget *form_panel = new QWidget();
	QVBoxLayout *form_layout = new QVBoxLayout();
	form_layout->setSpacing(0);
	form_layout->setContentsMargins(1, 0, 0, 0);
	//form_layout->setMargin(0);
	form_panel->setLayout(form_layout);

	QTabWidget *tabWidget = new QTabWidget;
	// Create Camera Control Frame
	QFrame *camera_frame = new QFrame();
	QFrame *focuser_frame = new QFrame();
	QFrame *guider_frame = new QFrame();
	QWidget *camera_panel = new QWidget();
	QVBoxLayout *camera_panel_layout = new QVBoxLayout();
	camera_frame->setFrameShape(QFrame::StyledPanel);
	camera_frame->setMinimumWidth(CAMERA_FRAME_MIN_WIDTH);
	camera_frame->setContentsMargins(0, 0, 0, 0);

	camera_panel_layout->setSpacing(0);
	camera_panel_layout->setContentsMargins(0, 0, 1, 0);
	//camera_panel_layout->setMargin(0);
	camera_panel->setLayout(camera_panel_layout);
	tabWidget->addTab(camera_frame, "Capture");
	tabWidget->addTab(focuser_frame, "Focus");
	tabWidget->addTab(guider_frame, "Guide");
	camera_panel_layout->addWidget(tabWidget);



	QGridLayout *camera_frame_layout = new QGridLayout();
	camera_frame_layout->setAlignment(Qt::AlignTop);
	camera_frame->setLayout(camera_frame_layout);
	int row = 0;
	// camera selection
	QLabel *label = new QLabel("Camera:");
	camera_frame_layout->addWidget(label, row, 0);
	m_camera_select = new QComboBox();
	camera_frame_layout->addWidget(m_camera_select, row, 1, 1, 3);
	connect(m_camera_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_camera_selected);

	// Filter wheel selection
	row++;
	label = new QLabel("Wheel:");
	camera_frame_layout->addWidget(label, row, 0);
	m_wheel_select = new QComboBox();
	camera_frame_layout->addWidget(m_wheel_select, row, 1, 1, 3);
	connect(m_wheel_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_wheel_selected);

	//row++;
	//QFrame* line = new QFrame();
	//line->setFrameShape(QFrame::HLine);
	//line->setFrameShadow(QFrame::Plain);
	//camera_frame_layout->addWidget(line, row, 0, 1, 4);

	// frame type
	row++;
	label = new QLabel("Frame:");
	camera_frame_layout->addWidget(label, row, 0);
	m_frame_size_select = new QComboBox();
	camera_frame_layout->addWidget(m_frame_size_select, row, 1, 1, 2);
	connect(m_frame_size_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_ccd_mode_selected);
	m_frame_type_select = new QComboBox();
	camera_frame_layout->addWidget(m_frame_type_select, row, 3);
	connect(m_frame_type_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_frame_type_selected);

	// ROI
	row++;
	label = new QLabel("ROI X:");
	camera_frame_layout->addWidget(label, row, 0);
	m_roi_x = new QSpinBox();
	m_roi_x->setMaximum(100000);
	m_roi_x->setMinimum(0);
	m_roi_x->setValue(0);
	m_roi_x->setEnabled(false);
	camera_frame_layout->addWidget(m_roi_x , row, 1);

	label = new QLabel("W:");
	camera_frame_layout->addWidget(label, row, 2);
	m_roi_w = new QSpinBox();
	m_roi_w->setMaximum(100000);
	m_roi_w->setMinimum(0);
	m_roi_w->setValue(0);
	m_roi_w->setEnabled(false);
	camera_frame_layout->addWidget(m_roi_w, row, 3);

	// ROI
	row++;
	label = new QLabel("ROI Y:");
	camera_frame_layout->addWidget(label, row, 0);
	m_roi_y = new QSpinBox();
	m_roi_y->setMaximum(100000);
	m_roi_y->setMinimum(0);
	m_roi_y->setValue(0);
	m_roi_y->setEnabled(false);
	camera_frame_layout->addWidget(m_roi_y , row, 1);

	label = new QLabel("H:");
	camera_frame_layout->addWidget(label, row, 2);
	m_roi_h = new QSpinBox();
	m_roi_h->setMaximum(100000);
	m_roi_h->setMinimum(0);
	m_roi_h->setValue(0);
	m_roi_h->setEnabled(false);
	camera_frame_layout->addWidget(m_roi_h, row, 3);

	// Exposure time
	row++;
	label = new QLabel("Exposure (s):");
	camera_frame_layout->addWidget(label, row, 0);
	m_exposure_time = new QDoubleSpinBox();
	m_exposure_time->setMaximum(10000);
	m_exposure_time->setMinimum(0);
	m_exposure_time->setValue(1);
	camera_frame_layout->addWidget(m_exposure_time, row, 1);

	//label = new QLabel(QChar(0x0394)+QString("t:"));
	label = new QLabel("Delay (s):");
	camera_frame_layout->addWidget(label, row, 2);
	m_exposure_delay = new QDoubleSpinBox();
	m_exposure_delay->setMaximum(10000);
	m_exposure_delay->setMinimum(0);
	m_exposure_delay->setValue(0);
	//m_exposure_delay->setEnabled(false);
	camera_frame_layout->addWidget(m_exposure_delay, row, 3);

	// Frame count
	row++;
	label = new QLabel("No frames:");
	camera_frame_layout->addWidget(label, row, 0);
	m_frame_count = new QSpinBox();
	m_frame_count->setMaximum(100000);
	m_frame_count->setMinimum(-1);
	m_frame_count->setValue(1);
	camera_frame_layout->addWidget(m_frame_count, row, 1);

	label = new QLabel("Filter:");
	camera_frame_layout->addWidget(label, row, 2);
	m_filter_select = new QComboBox();
	camera_frame_layout->addWidget(m_filter_select, row, 3);
	connect(m_filter_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_filter_selected);

	// Frame prefix
	row++;
	label = new QLabel("Object:");
	camera_frame_layout->addWidget(label, row, 0);
	m_object_name = new QLineEdit();
	camera_frame_layout->addWidget(m_object_name, row, 1, 1, 3);

	//row++;
	//line = new QFrame();
	//line->setFrameShape(QFrame::HLine);
	//line->setFrameShadow(QFrame::Plain);
	//camera_frame_layout->addWidget(line, row, 0, 1, 4);

	// Buttons
	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	camera_frame_layout->addWidget(toolbar, row, 0, 1, 4);


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
	camera_frame_layout->addWidget(m_exposure_progress, row, 0, 1, 4);
	m_exposure_progress->setFormat("Exposure: Idle");
	m_exposure_progress->setMaximum(1);
	m_exposure_progress->setValue(0);

	row++;
	m_process_progress = new QProgressBar();
	camera_frame_layout->addWidget(m_process_progress, row, 0, 1, 4);
	m_process_progress->setMaximum(1);
	m_process_progress->setValue(0);
	m_process_progress->setFormat("Process: Idle");


	m_viewer = new pal::ImageViewer(this);
	m_viewer->setText("No Image");
	m_viewer->setToolBarMode(pal::ImageViewer::ToolBarMode::Visible);
	form_layout->addWidget((QWidget*)m_viewer);
	m_viewer->setMinimumWidth(PROPERTY_AREA_MIN_WIDTH);

	QSplitter* hSplitter = new QSplitter;
	hSplitter->addWidget(camera_panel);
	hSplitter->addWidget(form_panel);
	hSplitter->setStretchFactor(0, 25);
	hSplitter->setStretchFactor(1, 55);
	propertyLayout->addWidget(hSplitter, 85);

	propertyLayout->addWidget(mLog, 15);

	mServiceModel = new QServiceModel("_indigo._tcp");
	mServiceModel->enable_auto_connect(conf.auto_connect);

	connect(mServiceModel, &QServiceModel::serviceAdded, mIndigoServers, &QIndigoServers::onAddService);
	connect(mServiceModel, &QServiceModel::serviceRemoved, mIndigoServers, &QIndigoServers::onRemoveService);
	connect(mServiceModel, &QServiceModel::serviceConnectionChange, mIndigoServers, &QIndigoServers::onConnectionChange);

	connect(mIndigoServers, &QIndigoServers::requestConnect, mServiceModel, &QServiceModel::onRequestConnect);
	connect(mIndigoServers, &QIndigoServers::requestDisconnect, mServiceModel, &QServiceModel::onRequestDisconnect);
	connect(mIndigoServers, &QIndigoServers::requestAddManualService, mServiceModel, &QServiceModel::onRequestAddManualService);
	connect(mIndigoServers, &QIndigoServers::requestRemoveManualService, mServiceModel, &QServiceModel::onRequestRemoveManualService);

	// NOTE: logging should be before update and delete of properties as they release the copy!!!
	connect(&IndigoClient::instance(), &IndigoClient::property_defined, this, &ImagerWindow::on_message_sent);
	connect(&IndigoClient::instance(), &IndigoClient::property_changed, this, &ImagerWindow::on_message_sent);
	connect(&IndigoClient::instance(), &IndigoClient::property_deleted, this, &ImagerWindow::on_message_sent);
	connect(&IndigoClient::instance(), &IndigoClient::message_sent, this, &ImagerWindow::on_message_sent);

	connect(&IndigoClient::instance(), &IndigoClient::property_defined, this, &ImagerWindow::on_property_define);
	connect(&IndigoClient::instance(), &IndigoClient::property_changed, this, &ImagerWindow::on_property_change);
	connect(&IndigoClient::instance(), &IndigoClient::property_deleted, this, &ImagerWindow::on_property_delete);

	connect(&IndigoClient::instance(), &IndigoClient::create_preview, this, &ImagerWindow::on_create_preview);
	connect(&IndigoClient::instance(), &IndigoClient::obsolete_preview, this, &ImagerWindow::on_obsolete_preview);
	connect(&IndigoClient::instance(), &IndigoClient::remove_preview, this, &ImagerWindow::on_remove_preview);

	connect(&Logger::instance(), &Logger::do_log, this, &ImagerWindow::on_window_log);

	//preview_cache.set_stretch_level(conf.preview_stretch_level);

	//  Start up the client
	IndigoClient::instance().enable_blobs(conf.blobs_enabled);
	IndigoClient::instance().start("INDIGO Imager");

	// load manually configured services
	mServiceModel->loadManualServices();
}


ImagerWindow::~ImagerWindow () {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	delete mLog;
	delete mIndigoServers;
	delete mServiceModel;
	delete m_viewer;
	//indigo_usleep(2*ONE_SECOND_DELAY);
	IndigoClient::instance().stop();
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

void ImagerWindow::on_create_preview(indigo_property *property, indigo_item *item){
	char selected_agent[INDIGO_VALUE_SIZE];

	if (!get_selected_agent(selected_agent) || strncmp(property->device, "Imager Agent",12)) {
		return;
	}

	if (client_match_device_property(property, selected_agent, CCD_IMAGE_PROPERTY_NAME)) {
		preview_cache.create(property, item);
		//free(item->blob.value);
		//item->blob.value = nullptr;
		preview_image *image = preview_cache.get(property, item);
		if (image) {
			indigo_error("m_viewer = %p", m_viewer);
			m_viewer->setText("Unsaved" + QString(item->blob.format));
			m_viewer->setImage(*image);
		}
		if (!m_preview) save_blob_item(item);
		free(item->blob.value);
		item->blob.value = nullptr;
		free(item);
	}
}

void ImagerWindow::on_obsolete_preview(indigo_property *property, indigo_item *item){
	preview_cache.obsolete(property, item);
}

void ImagerWindow::on_remove_preview(indigo_property *property, indigo_item *item){
	preview_cache.remove(property, item);
}

void ImagerWindow::on_message_sent(indigo_property* property, char *message) {
	on_window_log(property, message);
	free(message);
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


void ImagerWindow::save_blob_item(indigo_item *item) {
	if (item->blob.value != NULL) {
		char file_name[PATH_LEN];
		char message[PATH_LEN+100];
		char location[PATH_LEN];

		if (m_object_name->text().trimmed() == "") {
			snprintf(message, sizeof(message), "Image not saved, provide object name");
			on_window_log(NULL, message);
			return;
		}

		if (QStandardPaths::displayName(QStandardPaths::PicturesLocation).length() > 0) {
			QString qlocation = QDir::toNativeSeparators(QDir::homePath() + tr("/") + QStandardPaths::displayName(QStandardPaths::PicturesLocation));
			strncpy(location, qlocation.toUtf8().constData(), PATH_LEN);
		} else {
			if (!getcwd(location, sizeof(location))) {
				location[0] = '\0';
			}
		}

		if (save_blob_item_with_prefix(item, location, file_name)) {
			m_viewer->setText(file_name);
			snprintf(message, sizeof(message), "Image saved to '%s'", file_name);
			on_window_log(NULL, message);
		} else {
			snprintf(message, sizeof(message), "Can not save '%s'", file_name);
			on_window_log(NULL, message);
		}
	}
}

/* C++ looks for method close - maybe name collision so... */
void close_fd(int fd) {
	close(fd);
}

bool ImagerWindow::save_blob_item_with_prefix(indigo_item *item, const char *prefix, char *file_name) {
	int fd;
	int file_no = 0;

	QString object_name = m_object_name->text().trimmed();

	do {

#if defined(INDIGO_WINDOWS)
		sprintf(file_name, "%s\\%s_%03d%s", prefix, object_name.toUtf8().constData(), file_no++, item->blob.format);
		fd = open(file_name, O_CREAT | O_WRONLY | O_EXCL | O_BINARY, 0);
#else
		sprintf(file_name, "%s/%s_%03d%s", prefix, object_name.toUtf8().constData(), file_no++, item->blob.format);
		fd = open(file_name, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
#endif
	} while ((fd < 0) && (errno == EEXIST));

	if (fd < 0) {
		return false;
	} else {
		write(fd, item->blob.value, item->blob.size);
		close_fd(fd);
	}
	return true;
}


void ImagerWindow::on_servers_act() {
	mIndigoServers->show();
}


void ImagerWindow::on_exit_act() {
	QApplication::quit();
}


void ImagerWindow::on_blobs_changed(bool status) {
	conf.blobs_enabled = status;
	IndigoClient::instance().enable_blobs(status);
	emit(enable_blobs(status));
	if (status) on_window_log(NULL, "BLOBs enabled");
	else on_window_log(NULL, "BLOBs disabled");
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_bonjour_changed(bool status) {
	conf.auto_connect = status;
	mServiceModel->enable_auto_connect(conf.auto_connect);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_use_suffix_changed(bool status) {
	conf.indigo_use_host_suffix = status;
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_use_state_icons_changed(bool status) {
	conf.use_state_icons = status;
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_use_system_locale_changed(bool status) {
	conf.use_system_locale = status;
	write_conf();
	if (conf.use_system_locale){
		on_window_log(nullptr, "Locale specific decimal separator will be used on next application start");
	} else {
		on_window_log(nullptr, "Dot decimal separator will be used on next application start");
	}
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_no_stretch() {
	conf.preview_stretch_level = STRETCH_NONE;
	preview_cache.set_stretch_level(conf.preview_stretch_level);
	emit(rebuild_blob_previews());
	write_conf();
	indigo_error("%s\n", __FUNCTION__);
}


void ImagerWindow::on_normal_stretch() {
	conf.preview_stretch_level = STRETCH_NORMAL;
	preview_cache.set_stretch_level(conf.preview_stretch_level);
	emit(rebuild_blob_previews());
	write_conf();
	indigo_error("%s\n", __FUNCTION__);
}


void ImagerWindow::on_hard_stretch() {
	conf.preview_stretch_level = STRETCH_HARD;
	preview_cache.set_stretch_level(conf.preview_stretch_level);
	emit(rebuild_blob_previews());
	write_conf();
	indigo_error("%s\n", __FUNCTION__);
}


void ImagerWindow::on_log_error() {
	conf.indigo_log_level = INDIGO_LOG_ERROR;
	indigo_set_log_level(conf.indigo_log_level);
	write_conf();
	indigo_error("%s\n", __FUNCTION__);
}


void ImagerWindow::on_log_info() {
	indigo_debug("%s\n", __FUNCTION__);
	conf.indigo_log_level = INDIGO_LOG_INFO;
	indigo_set_log_level(conf.indigo_log_level);
	write_conf();
}


void ImagerWindow::on_log_debug() {
	indigo_debug("%s\n", __FUNCTION__);
	conf.indigo_log_level = INDIGO_LOG_DEBUG;
	indigo_set_log_level(conf.indigo_log_level);
	write_conf();
}


void ImagerWindow::on_log_trace() {
	indigo_debug("%s\n", __FUNCTION__);
	conf.indigo_log_level = INDIGO_LOG_TRACE;
	indigo_set_log_level(conf.indigo_log_level);
	write_conf();
}


void ImagerWindow::on_about_act() {
	QMessageBox msgBox(this);
	QPixmap pixmap(":resource/indigo_logo.png");
	msgBox.setWindowTitle("About Ain Imager");
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setIconPixmap(pixmap.scaledToWidth(96, Qt::SmoothTransformation));
	msgBox.setText(
		"<b>Ain INDIGO Imager</b><br>"
		"Version "
		PANEL_VERSION
		"</b><br><br>"
		"Author:<br>"
		"Rumen G.Bogdanovski<br>"
		"You can use this software under the terms of <b>INDIGO Astronomy open-source license</b><br><br>"
		"Copyright ©2020, The INDIGO Initiative.<br>"
		"<a href='http://www.indigo-astronomy.org'>http://www.indigo-astronomy.org</a>"
	);
	msgBox.exec();
	indigo_debug("%s\n", __FUNCTION__);
}
