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
#include <sys/stat.h>
#include <sys/time.h>
#include <libgen.h>

#include <utils.h>
#include "imagerwindow.h"
#include "qservicemodel.h"
#include "indigoclient.h"
#include "propertycache.h"
#include "qindigoservers.h"
#include "blobpreview.h"
#include "logger.h"
#include "conf.h"
#include "version.h"
#include <imageviewer.h>

void write_conf();

//ImageViewer *m_imager_viewer;

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

	save_blob = false;
	guider_running = false;
	m_indigo_item = nullptr;
	m_guide_log = nullptr;
	m_guider_process = 0;
	m_stderr = dup(STDERR_FILENO);

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
	mLog = new QTextEdit;
	mLog->setReadOnly(true);

	on_indigo_save_log(conf.indigo_save_log);
	on_guider_save_log(conf.guider_save_log);

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

	act = menu->addAction(tr("Select &Data Directroy..."));
	connect(act, &QAction::triggered, this, &ImagerWindow::on_data_directory_prefix_act);

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
	connect(act, &QAction::triggered, mLog, &QTextEdit::clear);
	menu_bar->addMenu(menu);

	menu = new QMenu("&Settings");

	act = menu->addAction(tr("Enable &BLOBs"));
	act->setCheckable(true);
	act->setChecked(conf.blobs_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_blobs_changed);

	act = menu->addAction(tr("Save &noname images"));
	act->setCheckable(true);
	act->setChecked(conf.save_noname_images);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_save_noname_images_changed);

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

	act = menu->addAction(tr("Enable image &antialiasing"));
	act->setCheckable(true);
	act->setChecked(conf.antialiasing_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_antialias_view);

	act = menu->addAction(tr("Enable guider &antialiasing"));
	act->setCheckable(true);
	act->setChecked(conf.guider_antialiasing_enabled);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_antialias_guide_view);

	menu->addSeparator();

	sub_menu = menu->addMenu("&Focuser Graph");

	QActionGroup *graph_group = new QActionGroup(this);
	graph_group->setExclusive(true);

	act = sub_menu->addAction("&FWHM");
	act->setCheckable(true);
	if (conf.focuser_display == SHOW_FWHM) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_focus_show_fwhm);
	graph_group->addAction(act);

	act = sub_menu->addAction("&HFD");
	act->setCheckable(true);
	if (conf.focuser_display == SHOW_HFD) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_focus_show_hfd);
	graph_group->addAction(act);

	sub_menu = menu->addMenu("&Guider Graph");

	graph_group = new QActionGroup(this);
	graph_group->setExclusive(true);

	act = sub_menu->addAction("&RA / Dec Drift");
	act->setCheckable(true);
	if (conf.guider_display == SHOW_RA_DEC_DRIFT) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_guide_show_rd_drift);
	graph_group->addAction(act);

	act = sub_menu->addAction("RA / Dec &Pulses");
	act->setCheckable(true);
	if (conf.guider_display == SHOW_RA_DEC_PULSE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_guide_show_rd_pulse);
	graph_group->addAction(act);

	act = sub_menu->addAction("&X / Y Drift");
	act->setCheckable(true);
	if (conf.guider_display == SHOW_X_Y_DRIFT) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ImagerWindow::on_guide_show_xy_drift);
	graph_group->addAction(act);

	menu->addSeparator();

	act = menu->addAction(tr("&Save Guiding Log"));
	act->setCheckable(true);
	act->setChecked(conf.guider_save_log);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_guider_save_log);

	act = menu->addAction(tr("Save &INDGO Log"));
	act->setCheckable(true);
	act->setChecked(conf.indigo_save_log);
	connect(act, &QAction::toggled, this, &ImagerWindow::on_indigo_save_log);

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
	m_tools_tabbar = new QTabWidget;
	tools_panel_layout->addWidget(m_tools_tabbar);

	QFrame *capture_frame = new QFrame();
	m_tools_tabbar->addTab(capture_frame, "&Capture");
	create_imager_tab(capture_frame);

	QFrame *focuser_frame = new QFrame;
	m_tools_tabbar->addTab(focuser_frame, "F&ocus");
	create_focuser_tab(focuser_frame);
	//m_tools_tabbar->setTabEnabled(1, false);

	QFrame *guider_frame = new QFrame;
	m_tools_tabbar->addTab(guider_frame, "&Guide");
	create_guider_tab(guider_frame);

	QFrame *telescope_frame = new QFrame;
	m_tools_tabbar->addTab(telescope_frame, "&Telescope");
	create_telescope_tab(telescope_frame);

	QFrame *solver_frame = new QFrame;
	m_tools_tabbar->addTab(solver_frame, "&Solver");
	create_solver_tab(solver_frame);

	/*
	QFrame *sequence_frame = new QFrame;
	tools_tabbar->addTab(sequence_frame, "&Sequence");
	//create_telescope_tab(sequence_frame);
	*/

	connect(m_tools_tabbar, &QTabWidget::currentChanged, this, &ImagerWindow::on_tab_changed);

	// Image viewer
	m_imager_viewer = new ImageViewer(this);
	m_imager_viewer->setText("No Image");
	m_imager_viewer->setToolTip("No Image");
	m_imager_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	form_layout->addWidget((QWidget*)m_imager_viewer);
	m_imager_viewer->setMinimumWidth(PROPERTY_AREA_MIN_WIDTH);
	m_imager_viewer->setStretch(conf.preview_stretch_level);
	m_imager_viewer->setBalance(conf.preview_color_balance);
	m_visible_viewer = m_imager_viewer;

	// Image guide viewer
	m_guider_viewer = new ImageViewer(this);
	m_guider_viewer->setText("Guider Image");
	m_guider_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	m_guider_viewer->setStretch(conf.guider_stretch_level);
	m_guider_viewer->setBalance(conf.guider_color_balance);
	m_guider_viewer->setVisible(false);

	QSplitter* hSplitter = new QSplitter;
	hSplitter->addWidget(tools_panel);
	hSplitter->addWidget(form_panel);
	hSplitter->setStretchFactor(0, 10);
	hSplitter->setStretchFactor(1, 90);

	propertyLayout->addWidget(hSplitter, 85);
	propertyLayout->addWidget(mLog, 15);

	select_focuser_data(conf.focuser_display);
	select_guider_data(conf.guider_display);

	mServiceModel = new QServiceModel("_indigo._tcp");
	mServiceModel->enable_auto_connect(conf.auto_connect);

	connect(m_imager_viewer, &ImageViewer::stretchChanged, this, &ImagerWindow::on_imager_stretch_changed);
	connect(m_imager_viewer, &ImageViewer::BalanceChanged, this, &ImagerWindow::on_imager_cb_changed);
	connect(m_guider_viewer, &ImageViewer::stretchChanged, this, &ImagerWindow::on_guider_stretch_changed);
	connect(m_guider_viewer, &ImageViewer::BalanceChanged, this, &ImagerWindow::on_guider_cb_changed);

	connect(this, &ImagerWindow::move_resize_focuser_selection, m_imager_viewer, &ImageViewer::moveResizeSelection);
	connect(this, &ImagerWindow::show_focuser_selection, m_imager_viewer, &ImageViewer::showSelection);
	connect(this, &ImagerWindow::move_resize_guider_selection, m_guider_viewer, &ImageViewer::moveResizeSelection);
	connect(this, &ImagerWindow::move_resize_guider_extra_selection, m_guider_viewer, &ImageViewer::moveResizeExtraSelection);
	connect(this, &ImagerWindow::show_guider_extra_selection, m_guider_viewer, &ImageViewer::showExtraSelection);
	connect(this, &ImagerWindow::show_guider_selection, m_guider_viewer, &ImageViewer::showSelection);
	connect(this, &ImagerWindow::move_guider_reference, m_guider_viewer, &ImageViewer::moveReference);
	connect(this, &ImagerWindow::show_guider_reference, m_guider_viewer, &ImageViewer::showReference);
	connect(this, &ImagerWindow::resize_guider_edge_clipping, m_guider_viewer, &ImageViewer::resizeEdgeClipping);
	connect(this, &ImagerWindow::show_guider_edge_clipping, m_guider_viewer, &ImageViewer::showEdgeClipping);

	connect(this, &ImagerWindow::set_combobox_current_text, this, &ImagerWindow::on_set_combobox_current_text);
	connect(this, &ImagerWindow::set_combobox_current_index, this, &ImagerWindow::on_set_combobox_current_index);
	connect(this, &ImagerWindow::add_combobox_item, this, &ImagerWindow::on_add_combobox_item);
	connect(this, &ImagerWindow::remove_combobox_item, this, &ImagerWindow::on_remove_combobox_item);
	connect(this, &ImagerWindow::clear_combobox, this, &ImagerWindow::on_clear_combobox);

	connect(this, &ImagerWindow::set_enabled, this, &ImagerWindow::on_set_enabled);
	connect(this, &ImagerWindow::set_widget_state, this, &ImagerWindow::on_set_widget_state);
	connect(this, &ImagerWindow::set_guider_label, this, &ImagerWindow::on_set_guider_label);
	connect(this, &ImagerWindow::set_lcd, this, &ImagerWindow::on_set_lcd);
	connect(this, &ImagerWindow::set_lineedit_text, this, &ImagerWindow::on_set_lineedit_text);
	connect(this, QOverload<QDoubleSpinBox*, double>::of(&ImagerWindow::set_spinbox_value), this, QOverload<QDoubleSpinBox*, double>::of(&ImagerWindow::on_set_spinbox_value));
	connect(this, QOverload<QSpinBox*, double>::of(&ImagerWindow::set_spinbox_value), this, QOverload<QSpinBox*, double>::of(&ImagerWindow::on_set_spinbox_value));
	connect(this, QOverload<QDoubleSpinBox*, indigo_item*, int>::of(&ImagerWindow::configure_spinbox), this, QOverload<QDoubleSpinBox*, indigo_item*, int>::of(&ImagerWindow::on_configure_spinbox));
	connect(this, QOverload<QSpinBox*, indigo_item*, int>::of(&ImagerWindow::configure_spinbox), this, QOverload<QSpinBox*, indigo_item*, int>::of(&ImagerWindow::on_configure_spinbox));
	connect(this, QOverload<QLabel*, QString>::of(&ImagerWindow::set_text), this, QOverload<QLabel*, QString>::of(&ImagerWindow::on_set_text));
	connect(this, QOverload<QLineEdit*, QString>::of(&ImagerWindow::set_text), this, QOverload<QLineEdit*, QString>::of(&ImagerWindow::on_set_text));
	connect(this, QOverload<QPushButton*, QString>::of(&ImagerWindow::set_text), this, QOverload<QPushButton*, QString>::of(&ImagerWindow::on_set_text));
	connect(this, QOverload<QCheckBox*, QString>::of(&ImagerWindow::set_text), this, QOverload<QCheckBox*, QString>::of(&ImagerWindow::on_set_text));
	connect(this, &ImagerWindow::set_checkbox_checked, this, &ImagerWindow::on_set_checkbox_checked);
	connect(this, &ImagerWindow::set_checkbox_state, this, &ImagerWindow::on_set_checkbox_state);

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
	//connect(&IndigoClient::instance(), &IndigoClient::obsolete_preview, this, &ImagerWindow::on_obsolete_preview, Qt::BlockingQueuedConnection);
	connect(&IndigoClient::instance(), &IndigoClient::remove_preview, this, &ImagerWindow::on_remove_preview, Qt::BlockingQueuedConnection);

	connect(&Logger::instance(), &Logger::do_log, this, &ImagerWindow::on_window_log);

	connect(m_imager_viewer, &ImageViewer::mouseRightPress, this, &ImagerWindow::on_image_right_click);
	connect(m_imager_viewer, &ImageViewer::mouseRightPressRADec, this, &ImagerWindow::on_image_right_click_ra_dec);
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

void ImagerWindow::window_log(char *message, int state) {
	char timestamp[255];
	char log_line[512];

	if (!message || !mLog) return;

	get_time(timestamp);

	switch (state) {
	case INDIGO_ALERT_STATE:
		mLog->setTextColor(QColor::fromRgb(224, 0, 0));
		break;
	case INDIGO_BUSY_STATE:
		mLog->setTextColor(QColor::fromRgb(255, 165, 0));
		break;
	default:
		mLog->setTextColor(Qt::white);
		break;
	}
	snprintf(log_line, 512, "%s %s", timestamp, message);
	indigo_log("[message] %s\n", log_line);
	mLog->append(log_line);
	mLog->verticalScrollBar()->setValue(mLog->verticalScrollBar()->maximum());
}

bool ImagerWindow::show_preview_in_imager_viewer(QString &key) {
	preview_image *image = preview_cache.get(key);
	if (image) {
		m_imager_viewer->setImage(*image);
		m_image_key = key;
		indigo_debug("IMAGER PREVIEW: %s\n", key.toUtf8().constData());
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

void ImagerWindow::show_selected_preview_in_solver_tab(QString &solver_source) {
	if (solver_source.startsWith("Guider Agent") && m_visible_viewer != m_guider_viewer) {
		m_visible_viewer->parentWidget()->layout()->replaceWidget(m_visible_viewer, m_guider_viewer);
		m_visible_viewer = m_guider_viewer;
		m_guider_viewer->setVisible(true);
		m_imager_viewer->setVisible(false);
	} else if (!solver_source.startsWith("Guider Agent") && m_visible_viewer != m_imager_viewer) {
		m_visible_viewer->parentWidget()->layout()->replaceWidget(m_visible_viewer, m_imager_viewer);
		m_visible_viewer = m_imager_viewer;
		m_imager_viewer->setVisible(true);
		m_guider_viewer->setVisible(false);
	}
	m_visible_viewer->showWCS(true);
}

void ImagerWindow::on_tab_changed(int index) {
	if (index == 0 || index == 1 || index == 3) {
		if (m_visible_viewer != m_imager_viewer) {
			m_visible_viewer->parentWidget()->layout()->replaceWidget(m_visible_viewer, m_imager_viewer);
			m_visible_viewer = m_imager_viewer;
			m_imager_viewer->setVisible(true);
			m_guider_viewer->setVisible(false);
		}
		if (index == 3) m_visible_viewer->showWCS(true);
		else m_visible_viewer->showWCS(false);
	} else if (index == 2) {
		if (m_visible_viewer != m_guider_viewer) {
			m_visible_viewer->parentWidget()->layout()->replaceWidget(m_visible_viewer, m_guider_viewer);
			m_visible_viewer = m_guider_viewer;
			m_guider_viewer->setVisible(true);
			m_imager_viewer->setVisible(false);
		}
		m_visible_viewer->showWCS(false);
	} else if (index == 4) {
		QString solver_source = m_solver_source_select1->currentText();
		show_selected_preview_in_solver_tab(solver_source);
	}
	if (index == 1) m_imager_viewer->showSelection(true);
	else m_imager_viewer->showSelection(false);
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
		const stretch_config_t sconfig = {conf.preview_stretch_level, conf.preview_color_balance};
		preview_cache.create(property, m_indigo_item, sconfig);
		QString key = preview_cache.create_key(property, m_indigo_item);
		//preview_image *image = preview_cache.get(m_image_key);
		if (show_preview_in_imager_viewer(key)) {
			indigo_debug("m_imager_viewer = %p", m_imager_viewer);
			m_imager_viewer->setText(QString("Unsaved") + QString(m_indigo_item->blob.format));
			m_imager_viewer->setToolTip(QString("Unsaved") + QString(m_indigo_item->blob.format));
		}
		if (save_blob) save_blob_item(m_indigo_item);
	} else if (get_selected_guider_agent(selected_agent)) {
		if ((client_match_device_property(property, selected_agent, CCD_IMAGE_PROPERTY_NAME) && conf.guider_save_bandwidth == 0) ||
			(client_match_device_property(property, selected_agent, CCD_PREVIEW_IMAGE_PROPERTY_NAME) && conf.guider_save_bandwidth > 0)) {
			const stretch_config_t sconfig = {conf.guider_stretch_level, conf.guider_color_balance};
			preview_cache.create(property, item, sconfig);
			QString key = preview_cache.create_key(property, item);
			preview_image *image = preview_cache.get(key);
			if (show_preview_in_guider_viewer(key)) {
				indigo_debug("m_guider_viewer = %p", m_guider_viewer);
				//m_guider_viewer->setText(QString("Guider: image") + QString(item->blob.format));
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

		if (m_object_name->text().trimmed() == "" && !conf.save_noname_images) {
			snprintf(message, sizeof(message), "Image not saved, provide object name");
			window_log(message, INDIGO_BUSY_STATE);
			return;
		}
		get_current_output_dir(location, conf.data_dir_prefix);
		if (save_blob_item_with_prefix(item, location, file_name)) {
			m_imager_viewer->setText(basename(file_name));
			m_imager_viewer->setToolTip(file_name);
			snprintf(message, sizeof(message), "Image saved to '%s'", file_name);
			window_log(message);
		} else {
			snprintf(message, sizeof(message), "Can not save '%s'", file_name);
			window_log(message, INDIGO_ALERT_STATE);
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
	if (object_name == "") object_name = "noname";
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
		sprintf(file_name, "%s%s_%03d%s", prefix, object_name.toUtf8().constData(), file_no++, item->blob.format);
		fd = open(file_name, O_CREAT | O_WRONLY | O_EXCL | O_BINARY, S_IRUSR | S_IWUSR);
#else
		sprintf(file_name, "%s%s_%03d%s", prefix, object_name.toUtf8().constData(), file_no++, item->blob.format);
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
	fd = open(file_name, O_CREAT | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
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
	QString qlocation = QDir::toNativeSeparators(QDir::homePath());
	QString file_name = QFileDialog::getSaveFileName(this,
		tr("Save image"), qlocation,
		QString("Image (*") + format + QString(")"));

	if (file_name == "") return;

	if (!file_name.endsWith(m_indigo_item->blob.format,Qt::CaseInsensitive)) file_name += m_indigo_item->blob.format;

	if (save_blob_item(m_indigo_item, file_name.toUtf8().data())) {
		m_imager_viewer->setText(basename(file_name.toUtf8().data()));
		m_imager_viewer->setToolTip(file_name);
		snprintf(message, sizeof(message), "Image saved to '%s'", file_name.toUtf8().data());
		window_log(message);
	} else {
		snprintf(message, sizeof(message), "Can not save '%s'", file_name.toUtf8().data());
		window_log(message, INDIGO_ALERT_STATE);
	}
}

void ImagerWindow::on_data_directory_prefix_act() {
	char message[PATH_LEN+100];
	QString qlocation = QDir::toNativeSeparators(QDir::homePath());
	if (conf.data_dir_prefix[0] != '\0') qlocation = QString(conf.data_dir_prefix);

	QString dir = QFileDialog::getExistingDirectory(this, tr("Select output data directory..."),
		qlocation,
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	if (dir == "") return;
	QFileInfo dir_info(QDir::toNativeSeparators(dir));
	if(!dir_info.isWritable()) {
		snprintf(message, sizeof(message), "Selected directory '%s' is not writable. Using the old one.", dir.toUtf8().data());
		window_log(message, INDIGO_ALERT_STATE);
		return;
	}

	strncpy(conf.data_dir_prefix, dir.toUtf8().data(), PATH_LEN);
	snprintf(message, sizeof(message), "Data will be saved to: '%s'", conf.data_dir_prefix);
	window_log(message);
	write_conf();
}

void ImagerWindow::on_exit_act() {
	QApplication::quit();
}


void ImagerWindow::on_blobs_changed(bool status) {
	conf.blobs_enabled = status;
	IndigoClient::instance().enable_blobs(status);
	emit(enable_blobs(status));
	if (status) window_log("BLOBs enabled");
	else window_log("BLOBs disabled");
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_save_noname_images_changed(bool status) {
	conf.save_noname_images = status;
	mServiceModel->enable_auto_connect(conf.save_noname_images);
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
		window_log("Locale specific decimal separator will be used on next application start");
	} else {
		window_log("Dot decimal separator will be used on next application start");
	}
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_imager_stretch_changed(int level) {
	conf.preview_stretch_level = (preview_stretch)level;
	const stretch_config_t sc = {conf.preview_stretch_level, conf.preview_color_balance};
	preview_cache.recreate(m_image_key, m_indigo_item, sc);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_imager_cb_changed(int balance) {
	conf.preview_color_balance = (color_balance)balance;
	const stretch_config_t sc = {conf.preview_stretch_level, conf.preview_color_balance};
	preview_cache.recreate(m_image_key, m_indigo_item, sc);
	show_preview_in_imager_viewer(m_image_key);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_guider_stretch_changed(int level) {
	conf.guider_stretch_level = (preview_stretch)level;
	QtConcurrent::run([=]() {
		static char selected_agent[INDIGO_NAME_SIZE];
		get_selected_guider_agent(selected_agent);
		setup_preview(selected_agent);
	});
	indigo_error("GUIDER -> %d\n", guider_running);
	if (!guider_running) {
		const stretch_config_t sc = {conf.guider_stretch_level, conf.guider_color_balance};
		preview_cache.stretch(m_guider_key, sc);
		show_preview_in_guider_viewer(m_guider_key);
	}
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_guider_cb_changed(int balance) {
	conf.guider_color_balance = (color_balance)balance;
	if (!guider_running) {
		const stretch_config_t sc = {conf.guider_stretch_level, conf.guider_color_balance};
		preview_cache.stretch(m_guider_key, sc);
		show_preview_in_guider_viewer(m_guider_key);
	}
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

void ImagerWindow::on_focus_show_fwhm() {
	conf.focuser_display = SHOW_FWHM;
	select_focuser_data(conf.focuser_display);
	if (m_focus_display_data) m_focus_graph->redraw_data(*m_focus_display_data);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}


void ImagerWindow::on_focus_show_hfd() {
	conf.focuser_display = SHOW_HFD;
	select_focuser_data(conf.focuser_display);
	if (m_focus_display_data) m_focus_graph->redraw_data(*m_focus_display_data);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_guide_show_rd_drift() {
	conf.guider_display = SHOW_RA_DEC_DRIFT;
	select_guider_data(conf.guider_display);
	if (m_guider_data_1 && m_guider_data_2) m_guider_graph->redraw_data2(*m_guider_data_1, *m_guider_data_2);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_guide_show_rd_pulse() {
	conf.guider_display = SHOW_RA_DEC_PULSE;
	select_guider_data(conf.guider_display);
	if (m_guider_data_1 && m_guider_data_2) m_guider_graph->redraw_data2(*m_guider_data_1, *m_guider_data_2);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_guide_show_xy_drift() {
	conf.guider_display = SHOW_X_Y_DRIFT;
	select_guider_data(conf.guider_display);
	if (m_guider_data_1 && m_guider_data_2) m_guider_graph->redraw_data2(*m_guider_data_1, *m_guider_data_2);
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_guider_save_log(bool status) {
	conf.guider_save_log = status;
	write_conf();
	char time_str[255];
	if (conf.guider_save_log) {
		if (m_guide_log == nullptr) {
			char file_name[255];
			char path[PATH_LEN];
			get_date_jd(time_str);
			get_current_output_dir(path, conf.data_dir_prefix);
			snprintf(file_name, sizeof(file_name), "%s" AIN_GUIDER_LOG_NAME_FORMAT, path, time_str);
			m_guide_log = fopen(file_name, "a+");
			if (m_guide_log) {
				get_timestamp(time_str);
				fprintf(m_guide_log, "\nLog started at %s\n", time_str);
				fflush(m_guide_log);
			} else {
				window_log("Can not open guider log file.", INDIGO_ALERT_STATE);
			}
		}
	} else {
		if (m_guide_log) {
			get_timestamp(time_str);
			fprintf(m_guide_log, "Log finished at %s\n", time_str);
			fclose(m_guide_log);
			m_guide_log = nullptr;
		}
	}
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_indigo_save_log(bool status) {
	conf.indigo_save_log = status;
	write_conf();

	if (status) {
		char time_str[PATH_LEN];
		char file_name[255];
		char path[PATH_LEN];
		get_date_jd(time_str);
		get_current_output_dir(path, conf.data_dir_prefix);
		snprintf(file_name, sizeof(file_name), "%s" AIN_INDIGO_LOG_NAME_FORMAT, path, time_str);
		freopen(file_name,"a+", stderr);
	} else {
#ifndef INDIGO_WINDOWS
		fflush(stderr);
		dup2(m_stderr, STDERR_FILENO);
		setvbuf(stderr, NULL, _IONBF, 0);
#else
		indigo_log("On Windows INDIGO log can not be stopped for the current session, it will take effect on Ain restart.");
#endif
	}
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
		window_log(message);
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
		window_log(message);
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
		window_log(message);
	}
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_acl_clear_act() {
	indigo_clear_device_tokens();
	window_log("Device ACL cleared");
	indigo_debug("%s\n", __FUNCTION__);
}

void ImagerWindow::on_about_act() {
	QMessageBox msgBox(this);
	int platform_bits = sizeof(void*) * 8;
	QPixmap pixmap(":resource/indigo_logo.png");
	msgBox.setWindowTitle("About Ain Imager");
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setIconPixmap(pixmap.scaledToWidth(96, Qt::SmoothTransformation));
	msgBox.setText(
		"<b>Ain INDIGO Imager</b><br>"
		"Version "
		AIN_VERSION
		" (" + QString::number(platform_bits) + "bit) <br>"
		"<br>"
		"Author:<br>"
		"Rumen G.Bogdanovski<br>"
		"You can use this software under the terms of <b>INDIGO Astronomy open-source license</b><br><br>"
		"Copyright ©2020-2021, The INDIGO Initiative.<br>"
		"<a href='http://www.indigo-astronomy.org'>http://www.indigo-astronomy.org</a>"
	);
	msgBox.exec();
	indigo_debug("%s\n", __FUNCTION__);
}
