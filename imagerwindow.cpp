// Copyright (c) 2019 Rumen G.Bogdanovski & David Hulse
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


#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeView>
#include <QMenuBar>
#include <QProgressBar>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QActionGroup>
#include <QLineEdit>
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
	resize(1024, 768);

	QIcon icon(":resource/appicon.png");
	this->setWindowIcon(icon);

	QFile f(":resource/control_panel.qss");
	f.open(QFile::ReadOnly | QFile::Text);
	QTextStream ts(&f);
	this->setStyleSheet(ts.readAll());
	f.close();

	mIndigoServers = new QIndigoServers(this);

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

	// Create Camera Control Frame
	QFrame *camera_frame = new QFrame();
	QWidget *camera_panel = new QWidget();
	QVBoxLayout *camera_panel_layout = new QVBoxLayout();
	camera_frame->setFrameShape(QFrame::StyledPanel);

	camera_panel_layout->setSpacing(0);
	camera_panel_layout->setContentsMargins(0, 0, 1, 0);
	//camera_panel_layout->setMargin(0);
	camera_panel->setLayout(camera_panel_layout);
	camera_panel_layout->addWidget(camera_frame);

	QGridLayout *camera_frame_layout = new QGridLayout();
	camera_frame_layout->setAlignment(Qt::AlignTop);
	camera_frame->setLayout(camera_frame_layout);
	int row = 0;
	// camera selection
	m_camera_select = new QComboBox();
	camera_frame_layout->addWidget(m_camera_select, row, 0, 1, 2);

	// frame type
	row++;
	QLabel *label = new QLabel("Frame type:");
	camera_frame_layout->addWidget(label, row, 0);
	m_frame_type_select = new QComboBox();
	camera_frame_layout->addWidget(m_frame_type_select, row, 1);

	// frame size
	row++;
	label = new QLabel("Frame size:");
	camera_frame_layout->addWidget(label, row, 0);
	m_frame_size_select = new QComboBox();
	camera_frame_layout->addWidget(m_frame_size_select, row, 1);

	// Exposure time
	row++;
	label = new QLabel("Exposure time (s):");
	camera_frame_layout->addWidget(label, row, 0);
	m_exposure_time = new QDoubleSpinBox();
	camera_frame_layout->addWidget(m_exposure_time, row, 1);

	// Frame count
	row++;
	label = new QLabel("Number of frames:");
	camera_frame_layout->addWidget(label, row, 0);
	m_frame_count = new QSpinBox();
	camera_frame_layout->addWidget(m_frame_count, row, 1);

	// Frame prefix
	row++;
	label = new QLabel("Frame prefix:");
	camera_frame_layout->addWidget(label, row, 0);
	QLineEdit *edit = new QLineEdit();
	camera_frame_layout->addWidget(edit, row, 1);


	// Frame prefix
	row++;
	QPushButton *Start = new QPushButton("Start");
	camera_frame_layout->addWidget(Start, row, 1);
	connect(Start, &QPushButton::clicked, this, &ImagerWindow::on_start);

	row++;
	m_exposure_progress = new QProgressBar();
	camera_frame_layout->addWidget(m_exposure_progress, row, 0, 1, 2);
	m_exposure_progress->setFormat("Exposure: Idle");
	m_exposure_progress->setMaximum(1);
	m_exposure_progress->setValue(0);

	row++;
	m_process_progress = new QProgressBar();
	camera_frame_layout->addWidget(m_process_progress, row, 0, 1, 2);
	m_process_progress->setMaximum(1);
	m_process_progress->setValue(0);
	m_process_progress->setFormat("Process: Idle");

	// image area
	//mImage = new QLabel();
	//mImage->setBackgroundRole(QPalette::Base);
	//mImage->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	//mImage->setScaledContents(true);

	m_viewer = new pal::ImageViewer(this);
	m_viewer->setText("A test viewer");
	m_viewer->setToolBarMode(pal::ImageViewer::ToolBarMode::Visible);
	//QString path("/home/rumen/m51_small.png");
	//m_image = new QImage(path);
	//m_viewer->setImage(m_image);


	mScrollArea = new QScrollArea();
	mScrollArea->setObjectName("PROPERTY_AREA");
	mScrollArea->setWidgetResizable(true);
	mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	mScrollArea->setWidget((QWidget*)m_viewer);
	form_layout->addWidget(mScrollArea);
	mScrollArea->setMinimumWidth(PROPERTY_AREA_MIN_WIDTH);

	QSplitter* hSplitter = new QSplitter;
	hSplitter->addWidget(camera_panel);
	hSplitter->addWidget(form_panel);
	hSplitter->setStretchFactor(0, 45);
	hSplitter->setStretchFactor(2, 55);
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

	//connect(&IndigoClient::instance(), &IndigoClient::property_defined, mPropertyModel, &PropertyModel::define_property);
	//connect(&IndigoClient::instance(), &IndigoClient::property_changed, mPropertyModel, &PropertyModel::update_property);
	//connect(&IndigoClient::instance(), &IndigoClient::property_deleted, mPropertyModel, &PropertyModel::delete_property);

	//connect(mPropertyModel, &PropertyModel::property_defined, this, &ImagerWindow::on_property_define);
	//connect(mPropertyModel, &PropertyModel::property_deleted, this, &ImagerWindow::on_property_delete);

	connect(&Logger::instance(), &Logger::do_log, this, &ImagerWindow::on_window_log);

	connect(m_camera_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_camera_selected);
	connect(m_frame_size_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_ccd_mode_selected);
	connect(m_frame_type_select, QOverload<int>::of(&QComboBox::activated), this, &ImagerWindow::on_frame_type_selected);

	//connect(mProperties->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ImagerWindow::on_selection_changed);
	//connect(this, &ImagerWindow::enable_blobs, mPropertyModel, &PropertyModel::enable_blobs);
	//connect(this, &ImagerWindow::rebuild_blob_previews, mPropertyModel, &PropertyModel::rebuild_blob_previews);

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
}

void ImagerWindow::on_start(bool clicked) {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	static double exposure_time;
	static double frame_count;
	static char selected_agent[INDIGO_NAME_SIZE];
	exposure_time = m_exposure_time->value();
	frame_count = (double)m_frame_count->value();
	get_selected_agent(selected_agent);
	static const char *items[] = { AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME };
	static double values[2];
	values[0] = exposure_time;
	values[1] = frame_count;
	indigo_change_number_property(nullptr, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME, 2, items, values);


	static const char * items2[] = { AGENT_IMAGER_START_EXPOSURE_ITEM_NAME };
	static bool values2[] = { true };
	indigo_change_switch_property(nullptr, selected_agent, AGENT_START_PROCESS_PROPERTY_NAME, 1, items2, values2);
}

void ImagerWindow::on_create_preview(indigo_property *property, indigo_item *item){
	preview_cache.create(property, item);

	QImage *image = preview_cache.get(property, item);
	if (image) {
		indigo_error("m_viewer = %p", m_viewer);
		m_viewer->setImage(*image);
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

void ImagerWindow::on_window_log(indigo_property* property, char *message) {
	char timestamp[16];
	char log_line[512];
	char message_line[512];
	struct timeval tmnow;

	if (!message) return;

	gettimeofday(&tmnow, NULL);
#if defined(INDIGO_WINDOWS)
	struct tm *lt;
	time_t rawtime;
	lt = localtime((const time_t *) &(tmnow.tv_sec));
	if (lt == NULL) {
		time(&rawtime);
		lt = localtime(&rawtime);
	}
	strftime(timestamp, sizeof(log_line), "%H:%M:%S", lt);
#else
	strftime(timestamp, sizeof(log_line), "%H:%M:%S", localtime((const time_t *) &tmnow.tv_sec));
#endif
	snprintf(timestamp + 8, sizeof(timestamp) - 8, ".%03ld", tmnow.tv_usec/1000);

	if (property) {
		snprintf(message_line, 512, "%s.%s: %s", property->device, property->name, message);
		switch (property->state) {
		case INDIGO_ALERT_STATE:
			snprintf(log_line, 512, "<font color = \"#E00000\">%s %s<\font>", timestamp, message_line);
			break;
		case INDIGO_BUSY_STATE:
			snprintf(log_line, 512, "<font color = \"orange\">%s %s<\font>", timestamp, message_line);
			break;
		default:
			snprintf(log_line, 512, "%s %s", timestamp, message_line);
			break;
		}
		indigo_debug("[message] %s\n", message_line);
	} else {
		snprintf(log_line, 512, "%s %s", timestamp, message);
		indigo_debug("[message] %s\n", message);
	}
	mLog->appendHtml(log_line); // Adds the message to the widget
}

void ImagerWindow::on_property_define(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE];
	if (!get_selected_agent(selected_agent)) return;
	if (strncmp(property->device, "Imager Agent",12)) return;

	if (client_match_device_property(property, nullptr, FILTER_CCD_LIST_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			QString item_name = QString(property->items[i].name);
			QString domain = QString(property->device);
			domain.remove(0, domain.indexOf(" @ "));
			QString device = QString(property->items[i].label) + domain;
			if (m_camera_select->findText(device) < 0) {
				m_camera_select->addItem(device, QString(property->device));
				indigo_debug("[ADD device] %s\n", device.toUtf8().data());
				if (property->items[i].sw.value) {
					m_camera_select->setCurrentIndex(m_camera_select->findText(device));
				}
			} else {
				indigo_debug("[DUPLICATE device] %s\n", device.toUtf8().data());
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		m_frame_size_select->clear();
		for (int i = 0; i < property->count; i++) {
			QString mode = QString(property->items[i].label);
			if (m_frame_size_select->findText(mode) < 0) {
				m_frame_size_select->addItem(mode, QString(property->items[i].name));
				indigo_debug("[ADD mode] %s\n", mode.toUtf8().data());
				if (property->items[i].sw.value) {
					m_frame_size_select->setCurrentIndex(m_frame_size_select->findText(mode));
				}
			} else {
				indigo_debug("[DUPLICATE mode] %s\n", mode.toUtf8().data());
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		m_frame_type_select->clear();
		for (int i = 0; i < property->count; i++) {
			QString type = QString(property->items[i].label);
			if (m_frame_type_select->findText(type) < 0) {
				m_frame_type_select->addItem(type, QString(property->items[i].name));
				indigo_debug("[ADD mode] %s\n", type.toUtf8().data());
				if (property->items[i].sw.value) {
					m_frame_type_select->setCurrentIndex(m_frame_type_select->findText(type));
				}
			} else {
				indigo_debug("[DUPLICATE mode] %s\n", type.toUtf8().data());
			}
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		indigo_debug("Set %s", property->name);
		for (int i = 0; i < property->count; i++) {
			indigo_debug("Set %s = %f", property->items[i].name, property->items[i].number.value);
			if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME)) {
				m_exposure_time->setValue(property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME)) {
				m_frame_count->setValue((int)property->items[i].number.value);
			}
		}
	}
	properties.create(property);
}

void ImagerWindow::on_property_change(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE];
	if (!get_selected_agent(selected_agent)) return;
	if (strncmp(property->device, "Imager Agent",12)) return;

	if (client_match_device_property(property, nullptr, FILTER_CCD_LIST_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			QString item_name = QString(property->items[i].name);
			if (property->items[i].sw.value) {
				QString domain = QString(property->device);
				domain.remove(0, domain.indexOf(" @ "));
				QString device = QString(property->items[i].label) + domain;
				m_camera_select->setCurrentIndex(m_camera_select->findText(device));
				indigo_debug("[ADD device] %s\n", device.toUtf8().data());
				break;
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			if (property->items[i].sw.value) {
				m_frame_size_select->setCurrentIndex(m_frame_size_select->findText(property->items[i].label));
				indigo_debug("[SELECT mode] %s\n", property->items[i].label);
				break;
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			QString type = QString(property->items[i].label);
			if (property->items[i].sw.value) {
				m_frame_type_select->setCurrentIndex(m_frame_type_select->findText(property->items[i].label));
				indigo_debug("[SELECT mode] %s\n", property->items[i].label);
				break;
			}
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_BATCH_PROPERTY_NAME)) {
		for (int i = 0; i < property->count; i++) {
			if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_EXPOSURE_ITEM_NAME)) {
				m_exposure_time->setValue(property->items[i].number.value);
			} else if (!strcmp(property->items[i].name, AGENT_IMAGER_BATCH_COUNT_ITEM_NAME)) {
				m_frame_count->setValue((int)property->items[i].number.value);
			}
		}
	}
	if (client_match_device_property(property, selected_agent, AGENT_IMAGER_STATS_PROPERTY_NAME)) {
		double exp_elapsed, exp_time;
		int frames_complete, frames_total;

		if(property->state == INDIGO_BUSY_STATE) {
			exp_time = m_exposure_time->value();
			for (int i = 0; i < property->count; i++) {
				if (!strcmp(property->items[i].name, AGENT_IMAGER_STATS_EXPOSURE_ITEM_NAME)) {
					exp_elapsed = exp_time - property->items[i].number.value;
				} else if (!strcmp(property->items[i].name, AGENT_IMAGER_STATS_FRAME_ITEM_NAME)) {
					frames_complete = (int)property->items[i].number.value;
				} else if (!strcmp(property->items[i].name, AGENT_IMAGER_STATS_FRAMES_ITEM_NAME)) {
					frames_total = (int)property->items[i].number.value;
				}
			}
			m_exposure_progress->setMaximum(exp_time);
			m_exposure_progress->setValue(exp_elapsed);
			m_exposure_progress->setFormat("Exposure: %v of %m seconds elapsed...");

			m_process_progress->setMaximum(frames_total);
			m_process_progress->setValue(frames_complete);
			m_process_progress->setFormat("Process: exposure %v of %m in progress...");

		} else if (property->state == INDIGO_OK_STATE) {
			m_exposure_progress->setMaximum(100);
			m_exposure_progress->setValue(100);
			m_exposure_progress->setFormat("Exposure: Complete");
			m_process_progress->setMaximum(100);
			m_process_progress->setValue(100);
			m_process_progress->setFormat("Process: Complete");
		} else {
			m_exposure_progress->setFormat("Exposure: Failed");
			m_process_progress->setFormat("Process: Failed");
		}
	}
	properties.create(property);
}


void ImagerWindow::on_property_delete(indigo_property* property, char *message) {
	char selected_agent[INDIGO_VALUE_SIZE];
	if (!get_selected_agent(selected_agent)) return;
	if (strncmp(property->device, "Imager Agent",12)) return;

	if (client_match_device_property(property, nullptr, FILTER_CCD_LIST_PROPERTY_NAME) || property->name[0] == '\0') {
		indigo_debug("[REMOVE REMOVE] %s\n", property->device);
		indigo_property *p = properties.get(property->device, FILTER_CCD_LIST_PROPERTY_NAME);
		if (p) {
			for (int i = 0; i < p->count; i++) {
				QString device = QString(p->device);
				int index = m_camera_select->findData(device);
				if (index >= 0) {
					m_camera_select->removeItem(index);
					indigo_debug("[REMOVE device] %s at index\n", device.toUtf8().data(), index);
				} else {
					indigo_debug("[No device] %s\n", device.toUtf8().data());
				}
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_MODE_PROPERTY_NAME) || property->name[0] == '\0') {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		indigo_property *p = properties.get(property->device, CCD_MODE_PROPERTY_NAME);
		if (p) {
			QString device = QString(p->device);
			int index = m_frame_size_select->findData(device);
			if (index >= 0) {
				m_frame_size_select->clear();
				indigo_debug("[REMOVE frame] %s\n", device.toUtf8().data());
			} else {
				indigo_debug("[No frame] %s\n", device.toUtf8().data());
			}
		}
	}
	if (client_match_device_property(property, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME) || property->name[0] == '\0') {
		indigo_debug("[REMOVE REMOVE] %s.%s\n", property->device, property->name);
		indigo_property *p = properties.get(property->device, CCD_FRAME_TYPE_PROPERTY_NAME);
		if (p) {
			QString device = QString(p->device);
			indigo_debug("[@@@@ ######## REMOVE frame] %s\n", m_frame_type_select->currentData().toString().toUtf8().data());
			int index = m_frame_type_select->findData(device);
			if (index >= 0) {
				m_frame_type_select->clear();
				indigo_debug("[REMOVE frame] %s\n", device.toUtf8().data());
			} else {
				indigo_debug("[No frame] %s\n", device.toUtf8().data());
			}
		}
	}
	properties.remove(property);
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

void ImagerWindow::on_ccd_mode_selected(int index) {
	static char selected_mode[INDIGO_NAME_SIZE];
	static char selected_agent[INDIGO_NAME_SIZE];

	strncpy(selected_mode, m_frame_size_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	strncpy(selected_agent, m_camera_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);

	indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_mode);
	static const char * items[] = { selected_mode };
	static bool values[] = { true };
	indigo_change_switch_property(nullptr, selected_agent, CCD_MODE_PROPERTY_NAME, 1, items, values);
}

void ImagerWindow::on_frame_type_selected(int index) {
	static char selected_type[INDIGO_NAME_SIZE];
	static char selected_agent[INDIGO_NAME_SIZE];

	strncpy(selected_type, m_frame_type_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);
	strncpy(selected_agent, m_camera_select->currentData().toString().toUtf8().constData(), INDIGO_NAME_SIZE);

	indigo_debug("[SELECTED] %s '%s' '%s'\n", __FUNCTION__, selected_agent, selected_type);
	static const char * items[] = { selected_type };
	static bool values[] = { true };
	indigo_change_switch_property(nullptr, selected_agent, CCD_FRAME_TYPE_PROPERTY_NAME, 1, items, values);
}


void ImagerWindow::clear_window() {
	indigo_debug("CLEAR_WINDOW!\n");
	delete mScrollArea;

	mScrollArea = new QScrollArea();
	mScrollArea->setObjectName("PROPERTY_AREA");
	mScrollArea->setWidgetResizable(true);
	mScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
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
	if(status) on_window_log(NULL, "BLOBs enabled");
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
	msgBox.setWindowTitle("About INDIGO Imager");
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
