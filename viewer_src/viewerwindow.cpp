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

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <libgen.h>

#include <utils.h>
#include <viewerwindow.h>
#include <imagepreview.h>
#include "conf.h"
#include "version.h"
#include <imageviewer.h>

void write_conf();

ImageViewer *m_imager_viewer;

ViewerWindow::ViewerWindow(QWidget *parent) : QMainWindow(parent) {
	setWindowTitle(tr("Ain Image Viewer"));
	resize(1200, 768);

	QIcon icon(":resource/appicon.png");
	this->setWindowIcon(icon);

	QFile f(":resource/control_panel.qss");
	f.open(QFile::ReadOnly | QFile::Text);
	QTextStream ts(&f);
	this->setStyleSheet(ts.readAll());
	f.close();

	//  Set central widget of window
	QWidget *central = new QWidget;
	setCentralWidget(central);

	//  Set the root layout to be a VBox
	QVBoxLayout *rootLayout = new QVBoxLayout;
	rootLayout->setSpacing(0);
	rootLayout->setContentsMargins(0, 0, 0, 0);
	rootLayout->setSizeConstraint(QLayout::SetMinimumSize);
	central->setLayout(rootLayout);

	// Create menubar
	QMenuBar *menu_bar = new QMenuBar;
	QMenu *menu = new QMenu("&File");
	QMenu *sub_menu;
	QAction *act;

	act = menu->addAction(tr("&Save Image As..."));
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_save_act);

	menu->addSeparator();

	act = menu->addAction(tr("&Exit"));
	connect(act, &QAction::triggered, this, &ViewerWindow::on_exit_act);
	menu_bar->addMenu(menu);



	menu = new QMenu("&Settings");

	act = menu->addAction(tr("Use &locale specific decimal separator"));
	act->setCheckable(true);
	act->setChecked(conf.use_system_locale);
	connect(act, &QAction::toggled, this, &ViewerWindow::on_use_system_locale_changed);

	menu->addSeparator();
	sub_menu = menu->addMenu("&Capture Image Preview");

	act = sub_menu->addAction(tr("Enable &antialiasing"));
	act->setCheckable(true);
	act->setChecked(conf.antialiasing_enabled);
	connect(act, &QAction::toggled, this, &ViewerWindow::on_antialias_view);

	sub_menu->addSeparator();

	QActionGroup *stretch_group = new QActionGroup(this);
	stretch_group->setExclusive(true);

	act = sub_menu->addAction("Stretch: N&one");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_NONE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_no_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Slight");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_SLIGHT) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_slight_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Moderate");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_MODERATE) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_moderate_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Normal");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_NORMAL) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_normal_stretch);
	stretch_group->addAction(act);

	act = sub_menu->addAction("Stretch: &Hard");
	act->setCheckable(true);
	if (conf.preview_stretch_level == STRETCH_HARD) act->setChecked(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_hard_stretch);
	stretch_group->addAction(act);

	menu_bar->addMenu(menu);

	menu = new QMenu("&Help");

	act = menu->addAction(tr("&About"));
	connect(act, &QAction::triggered, this, &ViewerWindow::on_about_act);
	menu_bar->addMenu(menu);

	rootLayout->addWidget(menu_bar);

	// Image viewer
	m_imager_viewer = new ImageViewer(this);
	m_imager_viewer->setText("No Image");
	m_imager_viewer->setToolTip("No Image");
	m_imager_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	rootLayout->addWidget((QWidget*)m_imager_viewer);
	m_imager_viewer->setMinimumWidth(PROPERTY_AREA_MIN_WIDTH);

	//connect(m_imager_viewer, &ImageViewer::mouseRightPress, this, &ViewerWindow::on_image_right_click);

	m_imager_viewer->enableAntialiasing(conf.antialiasing_enabled);
}


ViewerWindow::~ViewerWindow () {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	delete m_imager_viewer;
}


/* C++ looks for method close - maybe name collision so... */
void close_fd(int fd) {
	close(fd);
}


void ViewerWindow::on_image_save_act() {
	/*
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
		on_window_log(NULL, message);
	} else {
		snprintf(message, sizeof(message), "Can not save '%s'", file_name.toUtf8().data());
		on_window_log(NULL, message);
	}
	*/
}

void ViewerWindow::on_exit_act() {
	QApplication::quit();
}

void ViewerWindow::on_use_system_locale_changed(bool status) {
	conf.use_system_locale = status;
	write_conf();
	if (conf.use_system_locale){
		//on_window_log(nullptr, "Locale specific decimal separator will be used on next application start");
	} else {
		//on_window_log(nullptr, "Dot decimal separator will be used on next application start");
	}
	//indigo_debug("%s\n", __FUNCTION__);
}


void ViewerWindow::on_antialias_view(bool status) {
	conf.antialiasing_enabled = status;
	m_imager_viewer->enableAntialiasing(status);
	write_conf();
	//indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::on_no_stretch() {
	conf.preview_stretch_level = STRETCH_NONE;
	//preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	// show image
	write_conf();
	//indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::on_slight_stretch() {
	conf.preview_stretch_level = STRETCH_SLIGHT;
	//preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	// show image
	write_conf();
	//indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::on_moderate_stretch() {
	conf.preview_stretch_level = STRETCH_MODERATE;
	//preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	//show_preview_in_imager_viewer(m_image_key);
	write_conf();
	//indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::on_normal_stretch() {
	conf.preview_stretch_level = STRETCH_NORMAL;
	//preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	//show_preview_in_imager_viewer(m_image_key);
	write_conf();
	//indigo_debug("%s\n", __FUNCTION__);
}


void ViewerWindow::on_hard_stretch() {
	conf.preview_stretch_level = STRETCH_HARD;
	//preview_cache.recreate(m_image_key, m_indigo_item, preview_stretch_lut[conf.preview_stretch_level]);
	//show_preview_in_imager_viewer(m_image_key);
	write_conf();
	//indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::on_about_act() {
	QMessageBox msgBox(this);
	int platform_bits = sizeof(void*) * 8;
	QPixmap pixmap(":resource/indigo_logo.png");
	msgBox.setWindowTitle("About Ain Image Viewer");
	msgBox.setTextFormat(Qt::RichText);
	msgBox.setIconPixmap(pixmap.scaledToWidth(96, Qt::SmoothTransformation));
	msgBox.setText(
		"<b>Ain Image Viewer</b><br>"
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
	//indigo_debug("%s\n", __FUNCTION__);
}
