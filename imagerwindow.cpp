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

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <libgen.h>

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

ImageViewer *m_imager_viewer;

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

	m_save_blob = false;
	m_indigo_item = nullptr;

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
	QMenu *sub_menu;
	QAction *act;

	act = menu->addAction(tr("&Manage Services"));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_servers_act);

	menu->addSeparator();

	act = menu->addAction(tr("&Save Image As..."));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_image_save_act);

	menu->addSeparator();

	act = menu->addAction(tr("&Load Device ACL..."));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_acl_load_act);

	act = menu->addAction(tr("&Append to Device ACL..."));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_acl_append_act);

	act = menu->addAction(tr("&Save Device ACL As..."));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_acl_save_act);

	act = menu->addAction(tr("&Clear Device ACL"));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_acl_clear_act);

	menu->addSeparator();

	act = menu->addAction(tr("&Exit"));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_exit_act);

	menu_bar->addMenu(menu);

	menu = new QMenu("&Edit");
	act = menu->addAction(tr("Clear &Messages"));
	connect(act, &QAction::triggered, mLog, &QPlainTextEdit::clear);
	menu_bar->addMenu(menu);

	menu = new QMenu("&Settings");

	act = menu->addAction(tr("Enable &BLOBs"));
	act->setCheckable(true);
	act->setChecked(conf.blobs_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_blobs_changed);

	act = menu->addAction(tr("&Auto connect new services"));
	act->setCheckable(true);
	act->setChecked(conf.auto_connect);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_bonjour_changed);

	act = menu->addAction(tr("&Use host suffix"));
	act->setCheckable(true);
	act->setChecked(conf.indigo_use_host_suffix);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_use_suffix_changed);

	act = menu->addAction(tr("Use &locale specific decimal separator"));
	act->setCheckable(true);
	act->setChecked(conf.use_system_locale);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_use_system_locale_changed);

	menu->addSeparator();
	sub_menu = menu->addMenu("&Capture Image Preview");

	act = sub_menu->addAction(tr("Enable &antialiasing"));
	act->setCheckable(true);
	act->setChecked(conf.antialiasing_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_antialias_view);

	sub_menu->addSeparator();

	QActionGroup *stretch_group = new QActionGroup(this);
	stretch_group->setExclusive(true);

	act = sub_menu->addAction("Stretch: N&one");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_NONE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_no_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Slight");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_SLIGHT) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_slight_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Moderate");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_MODERATE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_moderate_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Normal");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_NORMAL) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_normal_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Hard");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_HARD) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_hard_stretch);
	stretch_group->addAction(act);

	sub_menu = menu->addMenu("&Guider Image Preview");

	act = sub_menu->addAction(tr("Enable &antialiasing"));
	act->setCheckable(true);
	act->setChecked(conf.guider_antialiasing_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_antialias_guide_view);

	sub_menu->addSeparator();

	stretch_group = new QActionGroup(this);
	stretch_group->setExclusive(true);

	act = sub_menu->addAction("Stretch: N&one");
	act->setCheckable(true);
	if (conf.guider_stretch_level == STRETCH_NONE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_no_guide_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Slight");
	act->setCheckable(true);
	if (conf.guider_stretch_level == STRETCH_SLIGHT) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_slight_guide_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Moderate");
	act->setCheckable(true);
	if (conf.guider_stretch_level == STRETCH_MODERATE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_moderate_guide_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Normal");
	act->setCheckable(true);
	if (conf.guider_stretch_level == STRETCH_NORMAL) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_normal_guide_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Hard");
	act->setCheckable(true);
	if (conf.guider_stretch_level == STRETCH_HARD) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_hard_guide_stretch);
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
	form_panel->setLayout(form_layout);

	// Tools Panel
	QWidget *tools_panel = new QWidget;
	QVBoxLayout *tools_panel_layout = new QVBoxLayout();
	tools_panel_layout->setSpacing(0);
	tools_panel_layout->setContentsMargins(0, 0, 1, 0);
	tools_panel->setLayout(tools_panel_layout);

	// Tools tabbar
	QTabWidget *tools_tabbar = new QTabWidget;
	tools_panel_layout->addWidget(tools_tabbar);

	QFrame *capture_frame = new QFrame();
	tools_tabbar->addTab(capture_frame, "&Capture");
	create_imager_tab(capture_frame);

	QFrame *focuser_frame = new QFrame;
	tools_tabbar->addTab(focuser_frame, "F&ocus");
	create_focuser_tab(focuser_frame);
	//tools_tabbar->setTabEnabled(1, false);

	QFrame *guider_frame = new QFrame;
	tools_tabbar->addTab(guider_frame, "&Guide");
	create_guider_tab(guider_frame);

	connect(tools_tabbar, &QTabWidget::currentChanged, this, &ImagerWindow::on_tab_changed);

	// Image viewer
	m_imager_viewer = new ImageViewer(this);
	m_imager_viewer->setText("No Image");
	m_imager_viewer->setToolTip("No Image");
	m_imager_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	form_layout->addWidget((QWidget*)m_imager_viewer);
	m_imager_viewer->setMinimumWidth(PROPERTY_AREA_MIN_WIDTH);
	m_visible_viewer = m_imager_viewer;

	// Image guide viewer
	m_guider_viewer = new ImageViewer(this);
	m_guider_viewer->setText("Guider Image");
	m_guider_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	m_guider_viewer->setVisible(false);

	QSplitter* hSplitter = new QSplitter;
	hSplitter->addWidget(tools_panel);
	hSplitter->addWidget(form_panel);
	hSplitter->setStretchFactor(0, 10);
	hSplitter->setStretchFactor(1, 90);

	propertyLayout->addWidget(hSplitter, 85);
	propertyLayout->addWidget(mLog, 15);

	mServiceModel = new QServiceModel("_indigo._tcp");
	mServiceModel->enable_auto_connect(conf.auto_connect);

	connect(this, &ImagerWindow::set_enabled, this, &ImagerWindow::on_set_enabled);
	connect(this, &ImagerWindow::set_widget_state, this, &ImagerWindow::on_set_widget_state);
	connect(this, QOverload<QDoubleSpinBox*, double>::of(&ImagerWindow::set_spinbox_value), this, QOverload<QDoubleSpinBox*, double>::of(&ImagerWindow::on_set_spinbox_value));
	connect(this, QOverload<QSpinBox*, double>::of(&ImagerWindow::set_spinbox_value), this, QOverload<QSpinBox*, double>::of(&ImagerWindow::on_set_spinbox_value));
	connect(this, QOverload<QDoubleSpinBox*, indigo_item*, int>::of(&ImagerWindow::configure_spinbox), this, QOverload<QDoubleSpinBox*, indigo_item*, int>::of(&ImagerWindow::on_configure_spinbox));
	connect(this, QOverload<QSpinBox*, indigo_item*, int>::of(&ImagerWindow::configure_spinbox), this, QOverload<QSpinBox*, indigo_item*, int>::of(&ImagerWindow::on_configure_spinbox));

	connect(mServiceModel, &QServiceModel::serviceAdded, mIndigoServers, &QIndigoServers::onAddService);
	connect(mServiceModel, &QServiceModel::serviceRemoved, mIndigoServers, &QIndigoServers::onRemoveService);
	connect(mServiceModel, &QServiceModel::serviceConnectionChange, mIndigoServers, &QIndigoServers::onConnectionChange);

	connect(mIndigoServers, &QIndigoServers::requestConnect, mServiceModel, &QServiceModel::onRequestConnect);
	connect(mIndigoServers, &QIndigoServers::requestDisconnect, mServiceModel, &QServiceModel::onRequestDisconnect);
	connect(mIndigoServers, &QIndigoServers::requestAddManualService, mServiceModel, &QServiceModel::onRequestAddManualService);
	connect(mIndigoServers, &QIndigoServers::requestRemoveManualService, mServiceModel, &QServiceModel::onRequestRemoveManualService);
	connect(mIndigoServers, &QIndigoServers::requestSaveServices, mServiceModel, &QServiceModel::onRequestSaveServices);

	// NOTE: logging should be before update and delete of properties as they release the copy!!!
	connect(&IndigoClient::instance(), &IndigoClient::property_defined, this, &ImagerWindow::on_message_sent);
	connect(&IndigoClient::instance(), &IndigoClient::property_changed, this, &ImagerWindow::on_message_sent);
	connect(&IndigoClient::instance(), &IndigoClient::property_deleted, this, &ImagerWindow::on_message_sent);
	connect(&IndigoClient::instance(), &IndigoClient::message_sent, this, &ImagerWindow::on_message_sent);

	connect(&IndigoClient::instance(), &IndigoClient::property_defined, this, &ImagerWindow::on_property_define, Qt::BlockingQueuedConnection);
	connect(&IndigoClient::instance(), &IndigoClient::property_changed, this, &ImagerWindow::on_property_change, Qt::BlockingQueuedConnection);
	connect(&IndigoClient::instance(), &IndigoClient::property_deleted, this, &ImagerWindow::on_property_delete, Qt::BlockingQueuedConnection);

	connect(&IndigoClient::instance(), &IndigoClient::create_preview, this, &ImagerWindow::on_create_preview, Qt::BlockingQueuedConnection);
	connect(&IndigoClient::instance(), &IndigoClient::obsolete_preview, this, &ImagerWindow::on_obsolete_preview, Qt::BlockingQueuedConnection);
	connect(&IndigoClient::instance(), &IndigoClient::remove_preview, this, &ImagerWindow::on_remove_preview, Qt::BlockingQueuedConnection);

	connect(&Logger::instance(), &Logger::do_log, this, &ImagerWindow::on_window_log);

	connect(m_imager_viewer, &ImageViewer::mouseRightPress, this, &ImagerWindow::on_image_right_click);
	connect(m_guider_viewer, &ImageViewer::mouseRightPress, this, &ImagerWindow::on_guider_image_right_click);

	m_imager_viewer->enableAntialiasing(conf.antialiasing_enabled);
	m_guider_viewer->enableAntialiasing(conf.guider_antialiasing_enabled);

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
	QtConcurrent::run([=]() {
		IndigoClient::instance().stop();
	});
	delete m_imager_viewer;
	if (m_indigo_item) {
		if (m_indigo_item->blob.value) {
			free(m_indigo_item->blob.value);
		}
		free(m_indigo_item);
		m_indigo_item = nullptr;
	}
}

bool ImagerWindow::show_preview_in_imager_viewer(QString &key) {
	preview_image *image = preview_cache.get(key);
	if (image) {
		m_imager_viewer->setImage(*image);
		m_image_key = key;
		indigo_debug("YYYYY PREVIEW: %s\n", key.toUtf8().constData());
		return true;
	}
	return false;
}

bool ImagerWindow::show_preview_in_guider_viewer(QString &key) {
	preview_image *image = preview_cache.get(key);
	if (image) {
		m_guider_viewer->setImage(*image);
		m_guider_key = key;
		indigo_debug("GUIDER PREVIEW: %s\n", key.toUtf8().constData());
		return true;
	}
	return false;
}


void ImagerWindow::on_tab_changed(int index) {
	if ((index == 0) || (index == 1)) {
		if (m_visible_viewer != m_imager_viewer) {
			m_visible_viewer->parentWidget()->layout()->replaceWidget(m_visible_viewer, m_imager_viewer);
			m_visible_viewer = m_imager_viewer;
			m_imager_viewer->setVisible(true);
			m_guider_viewer->setVisible(false);
		}
	} else if (index == 2) {
		if (m_visible_viewer != m_guider_viewer) {
			m_visible_viewer->parentWidget()->layout()->replaceWidget(m_visible_viewer, m_guider_viewer);
			m_visible_viewer = m_guider_viewer;
			m_guider_viewer->setVisible(true);
			m_imager_viewer->setVisible(false);
		}
	}
	if (index == 1) m_imager_viewer->showSelection();
	else m_imager_viewer->hideSelection();
	if (index == 2) {
		m_guider_viewer->showSelection();
		m_guider_viewer->showReference();
	} else {
		m_guider_viewer->hideSelection();
		m_guider_viewer->hideReference();
	}
}

void ImagerWindow::on_create_preview(indigo_property *property, indigo_item *item){
	char selected_agent[INDIGO_VALUE_SIZE];

	if (get_selected_imager_agent(selected_agent) && client_match_device_property(property, selected_agent, CCD_IMAGE_PROPERTY_NAME)) {
		if (m_indigo_item) {
			if (m_indigo_item->blob.value) {
				free(m_indigo_item->blob.value);
			}
			free(m_indigo_item);
			m_indigo_item = nullptr;
		}
		m_indigo_item = item;
		preview_cache.create(property, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
		QString key = preview_cache.create_key(property, m_indigo_item);
		//preview_image *image = preview_cache.get(m_image_key);
		if (show_preview_in_imager_viewer(key)) {
			indigo_debug("m_imager_viewer = %p", m_imager_viewer);
			m_imager_viewer->setText(QString("Unsaved") + QString(m_indigo_item->blob.format));
			m_imager_viewer->setToolTip(QString("Unsaved") + QString(m_indigo_item->blob.format));
		}
		if (m_save_blob) save_blob_item(m_indigo_item);
	} else if (get_selected_guider_agent(selected_agent)) {
		if ((client_match_device_property(property, selected_agent, CCD_IMAGE_PROPERTY_NAME) && conf.guider_save_bandwidth == 0) ||
			(client_match_device_property(property, selected_agent, CCD_PREVIEW_IMAGE_PROPERTY_NAME) && conf.guider_save_bandwidth > 0)) {
			preview_cache.create(property, item, preview_stretch_lut[conf.guider_stretch_level]);
			QString key = preview_cache.create_key(property, item);
			preview_image *image = preview_cache.get(key);
			if (show_preview_in_guider_viewer(key)) {
				indigo_debug("m_guider_viewer = %p", m_guider_viewer);
				m_guider_viewer->setText(QString("Guider: image") + QString(item->blob.format));
			}
		} else {
			preview_cache.remove(property, item);
		}
		free(item->blob.value);
		item->blob.value = nullptr;
		free(item);
	} else {
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
			m_imager_viewer->setText(basename(file_name));
			m_imager_viewer->setToolTip(file_name);
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
	QString filter_name = m_filter_select->currentText().trimmed();
	QString frame_type = m_frame_type_select->currentText().trimmed();
	QDateTime date = date.currentDateTime();
	QString date_str = date.toString("yyyy-MM-dd");
	if ((filter_name !=  "") && ((frame_type == "Light") || (frame_type == "Flat"))) {
		object_name = object_name + "_" + date_str + "_" + frame_type + "_" + filter_name;
	} else {
		object_name = object_name + "_" + date_str + "_" + frame_type;
	}

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

bool ImagerWindow::save_blob_item(indigo_item *item, char *file_name) {
	int fd;

#if defined(INDIGO_WINDOWS)
	fd = open(file_name, O_CREAT | O_WRONLY | O_BINARY, 0);
#else
	fd = open(file_name, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
#endif
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


void ImagerWindow::on_image_save_act() {
	if (!m_indigo_item) return;
	char message[PATH_LEN+100];
	QString format = m_indigo_item->blob.format;
	QString qlocation = QDir::toNativeSeparators(QDir::homePath() + tr("/") + QStandardPaths::displayName(QStandardPaths::PicturesLocation));
	QString file_name = QFileDialog::getSaveFileName(this,
		tr("Save image"), qlocation,
		QString("Image (*") + format + QString(")"));

	if (file_name == "") return;

	if (!file_name.endsWith(m_indigo_item->blob.format,Qt::CaseInsensitive)) file_name += m_indigo_item->blob.format;

	if (save_blob_item(m_indigo_item, file_name.toUtf8().data())) {
		m_imager_viewer->setText(basename(file_name.toUtf8().data()));
		m_imager_viewer->setToolTip(file_name);
		snprintf(message, sizeof(message), "Image saved to '%s'", file_name.toUtf8().data());
		on_window_log(NULL, message);
	} else {
		snprintf(message, sizeof(message), "Can not save '%s'", file_name.toUtf8().data());
		on_window_log(NULL, message);
	}
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
	preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_slight_stretch() {
	conf.preview_stretch_level = STRETCH_SLIGHT;
	preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_moderate_stretch() {
	conf.preview_stretch_level = STRETCH_MODERATE;
	preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_normal_stretch() {
	conf.preview_stretch_level = STRETCH_NORMAL;
	preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_hard_stretch() {
	conf.preview_stretch_level = STRETCH_HARD;
	preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_no_guide_stretch() {
	conf.guider_stretch_level = STRETCH_NONE;
	//	preview_cache.recreate(m_guider_key, m_indigo_item, preview_stretch_lut[conf.guider_stretch_level]);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		setup_preview(selected_agent);
	});
	//show_preview_in_guider_viewer(m_guider_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_slight_guide_stretch() {
	conf.guider_stretch_level = STRETCH_SLIGHT;
	//	preview_cache.recreate(m_guider_key, m_indigo_item, preview_stretch_lut[conf.guider_stretch_level]);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		setup_preview(selected_agent);
	});
	//show_preview_in_guider_viewer(m_guider_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_moderate_guide_stretch() {
	conf.guider_stretch_level = STRETCH_MODERATE;
	//	preview_cache.recreate(m_guider_key, m_indigo_item, preview_stretch_lut[conf.guider_stretch_level]);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		setup_preview(selected_agent);
	});
	//show_preview_in_guider_viewer(m_guider_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_normal_guide_stretch() {
	conf.guider_stretch_level = STRETCH_NORMAL;
	//	preview_cache.recreate(m_guider_key, m_indigo_item, preview_stretch_lut[conf.guider_stretch_level]);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		setup_preview(selected_agent);
	});
	// show_preview_in_guider_viewer(m_guider_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_hard_guide_stretch() {
	conf.guider_stretch_level = STRETCH_HARD;
	//preview_cache.recreate(m_guider_key, m_indigo_item, preview_stretch_lut[conf.guider_stretch_level]);
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		setup_preview(selected_agent);
	});
	//show_preview_in_guider_viewer(m_guider_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_antialias_view(bool status) {
	conf.antialiasing_enabled = status;
	m_imager_viewer->enableAntialiasing(status);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_antialias_guide_view(bool status) {
	conf.guider_antialiasing_enabled = status;
	m_guider_viewer->enableAntialiasing(status);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_log_error() {
	conf.indigo_log_level = INDIGO_LOG_ERROR;
	indigo_set_log_level(conf.indigo_log_level);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
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

void ImagerWindow::on_acl_load_act() {
	QString filter = "INDIGO Device Access Control (*.idac);; All files (*)";
	QString file_name = QFileDialog::getOpenFileName(this, "Load Device ACL...", QDir::currentPath(), filter);
	if (!file_name.isNull()) {
		char fname[PATH_MAX];
		strcpy(fname, file_name.toStdString().c_str());
		char message[PATH_MAX];
		indigo_clear_device_tokens();
		if (indigo_load_device_tokens_from_file(fname)) {
			snprintf(message, PATH_MAX, "Current device ACL cleared, new device ACL loaded from '%s'", fname);
		} else {
			snprintf(message, PATH_MAX, "Current device ACL cleared but failed to load device ACL from '%s'", fname);
		}
		on_window_log(NULL, message);
	}
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_acl_append_act() {
	QString filter = "INDIGO Device Access Control (*.idac);; All files (*)";
	QString file_name = QFileDialog::getOpenFileName(this, "Append to device ACL...", QDir::currentPath(), filter);
	if (!file_name.isNull()) {
		char fname[PATH_MAX];
		strcpy(fname, file_name.toStdString().c_str());
		char message[PATH_MAX];
		if (indigo_load_device_tokens_from_file(fname)) {
			snprintf(message, PATH_MAX, "Appended to device ACL from '%s'", fname);
		} else {
			snprintf(message, PATH_MAX, "Failed to append to device ACL form '%s'", fname);
		}
		on_window_log(NULL, message);
	}
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_acl_save_act() {
	QString filter = "INDIGO Device Access Control (*.idac);; All files (*)";
	QString file_name = QFileDialog::getSaveFileName(this, "Save device ACL As...", QDir::currentPath(), filter);
	if (!file_name.isNull()) {
		char fname[PATH_MAX];
		strcpy(fname, file_name.toStdString().c_str());
		char message[PATH_MAX];
		if (indigo_save_device_tokens_to_file(fname)) {
			snprintf(message, PATH_MAX, "Device ACL saved as '%s'", fname);
		} else {
			snprintf(message, PATH_MAX, "Failed to save device ACL as '%s'", fname);
		}
		on_window_log(NULL, message);
	}
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_acl_clear_act() {
	indigo_clear_device_tokens();
	on_window_log(NULL, "Device ACL cleared");
	indigo_debug("%s\n", __FUNCTION__);
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
		AIN_VERSION
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
