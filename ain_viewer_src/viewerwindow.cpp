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

ViewerWindow::ViewerWindow(QWidget *parent) : QMainWindow(parent) {
	setWindowTitle(tr("Ain Viewer"));
	resize(1024, 768);

	m_image_data = nullptr;
	m_image_size = 0;
	m_image_formrat = nullptr;
	m_image_path[0] = '\0';
	m_preview_image = nullptr;

	QIcon icon(":resource/ain_viewer.png");
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
	QAction *act;

	act = menu->addAction(tr("&Open Image..."));
	act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_O));
	act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_open_act);

	act = menu->addAction(tr("&Next Image"));
	act->setShortcut(QKeySequence(Qt::Key_Right));
	act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_next_act);

	act = menu->addAction(tr("&Previois Image"));
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_prev_act);
	act->setShortcut(QKeySequence(Qt::Key_Left));
	act->setShortcutVisibleInContextMenu(true);

	menu->addSeparator();

	act = menu->addAction(tr("&Close Image"));
	act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
	act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_close_act);

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

	act = menu->addAction(tr("Enable &antialiasing"));
	act->setCheckable(true);
	act->setChecked(conf.antialiasing_enabled);
	connect(act, &QAction::toggled, this, &ViewerWindow::on_antialias_view);

	menu_bar->addMenu(menu);

	menu = new QMenu("&Help");

	act = menu->addAction(tr("&About"));
	connect(act, &QAction::triggered, this, &ViewerWindow::on_about_act);
	menu_bar->addMenu(menu);

	rootLayout->addWidget(menu_bar);

	QWidget *form_panel = new QWidget();
	QVBoxLayout *form_layout = new QVBoxLayout();
	form_layout->setSpacing(5);
	form_layout->setContentsMargins(5, 5, 5, 5);
	form_panel->setLayout(form_layout);

	// Image viewer
	m_imager_viewer = new ImageViewer(this, true);
	m_imager_viewer->setToolBarMode(ImageViewer::ToolBarMode::Visible);
	form_layout->addWidget((QWidget*)m_imager_viewer);
	m_imager_viewer->setMinimumWidth(PROPERTY_AREA_MIN_WIDTH);
	rootLayout->addWidget(form_panel);

	m_imager_viewer->setStretch(conf.preview_stretch_level);

	connect(m_imager_viewer, &ImageViewer::stretchChanged, this, &ViewerWindow::on_stretch_changed);
	connect(m_imager_viewer, &ImageViewer::previousRequested, this, &ViewerWindow::on_image_prev_act);
	connect(m_imager_viewer, &ImageViewer::nextRequested, this, &ViewerWindow::on_image_next_act);

	//QShortcut *next_sc = new QShortcut(QKeySequence(Qt::Key_Right), this);
	//QShortcut *prev_sc = new QShortcut(QKeySequence(Qt::Key_Left), this);

	//QObject::connect(next_sc, &QShortcut::activated, this, &ViewerWindow::on_image_next_act);
	//QObject::connect(prev_sc, &QShortcut::activated, this, &ViewerWindow::on_image_prev_act);

	m_imager_viewer->enableAntialiasing(conf.antialiasing_enabled);
	if (conf.file_open[0] != '\0') open_image(conf.file_open);
}

ViewerWindow::~ViewerWindow () {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	free(m_image_data);
	delete m_preview_image;
	delete m_imager_viewer;
}

/* C++ looks for method close - maybe name collision so... */
void close_fd(int fd) {
	close(fd);
}

void ViewerWindow::open_image(QString file_name) {
	char msg[PATH_LEN];
	if (file_name == "") return;
	FILE *file;
	strncpy(m_image_path, file_name.toUtf8().data(), PATH_LEN);
	file = fopen(m_image_path, "rb");
	if (file) {
		fseek(file, 0, SEEK_END);
		m_image_size = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);
		if (m_image_data == nullptr) {
			m_image_data = (unsigned char *)malloc(m_image_size + 1);
		} else {
			m_image_data = (unsigned char *)realloc(m_image_data, m_image_size + 1);
		}
		fread(m_image_data, m_image_size, 1, file);
		fclose(file);
	} else {
		snprintf(msg, PATH_LEN, "File '%s'\nCan not be open for reading.", QDir::toNativeSeparators(m_image_path).toUtf8().data());
		show_error("Error!", msg);
	}

	if (m_preview_image) {
		delete(m_preview_image);
		m_preview_image = nullptr;
	}

	m_image_formrat = strrchr(m_image_path, '.');
	m_preview_image = create_preview(m_image_data, m_image_size, (const char*)m_image_formrat,  preview_stretch_lut[conf.preview_stretch_level]);
	if (m_preview_image) {
		m_imager_viewer->setImage(*m_preview_image);
		setWindowTitle(tr("Ain Viewer - ") + QString(m_image_path));
		char info[256] = {};
		int w = m_preview_image->width();
		int h = m_preview_image->height();
		sprintf(info, "%s [%d x %d]", basename(m_image_path), w, h);
		m_imager_viewer->setText(info);
	} else {
		snprintf(msg, PATH_LEN, "File: '%s'\nDoes not seem to be a supported image format.", QDir::toNativeSeparators(m_image_path).toUtf8().data());
		show_error("Error!", msg);
	}
}

void ViewerWindow::on_image_open_act() {
	char path[PATH_LEN];
	strncpy(path, m_image_path, PATH_LEN);
	QString qlocation(dirname(path));
	if (m_image_path[0] == '\0') qlocation = QDir::toNativeSeparators(QDir::homePath());
	QString file_name = QFileDialog::getOpenFileName(
		this,
		tr("Open Image"),
		qlocation,
		QString("FITS (*.fit *.fits *.fts);;Indigo RAW (*.raw);;FITS / Indigo RAW (*.fit *.fits *.fts *.raw);;JPEG / TIFF / PNG (*.jpg *.jpeg *.tif *.tiff *.png);;All Files (*)"),
		&m_selected_filter
	);
	open_image(file_name);
	QDir directory(dirname(file_name.toUtf8().data()));
	QString pattern = "*" + QString(m_image_formrat);
	m_image_list = directory.entryList(QStringList() << pattern, QDir::Files);
}

void ViewerWindow::on_image_next_act() {
	char path[PATH_LEN];

	if (m_image_path[0] == '\0') return;
	strncpy(path, m_image_path, PATH_LEN);

	char *file_name = basename(path);
	if (file_name == nullptr) return;
	int index = m_image_list.indexOf(file_name) + 1;

	if (index <= 0) return;

	if (index >= m_image_list.size()) {
		index = 0;
	}

	QString next_file = QDir::toNativeSeparators(QString(dirname(path)) + "/" + m_image_list.at(index));
	indigo_debug("next_index = %d, %s\n", index, next_file.toUtf8().data());

	open_image(next_file.toUtf8().data());
}

void ViewerWindow::on_image_prev_act() {
	char path[PATH_LEN];

	if (m_image_path[0] == '\0') return;
	strncpy(path, m_image_path, PATH_LEN);

	char *file_name = basename(path);
	if (file_name == nullptr) return;
	int index = m_image_list.indexOf(file_name) - 1;

	if (index < -1) return;

	if (index == -1) {
		index = m_image_list.size() - 1;
	}

	QString next_file = QDir::toNativeSeparators(QString(dirname(path)) + "/" + m_image_list.at(index));
	indigo_debug("prev_index = %d, %s\n", index, next_file.toUtf8().data());

	open_image(next_file.toUtf8().data());
}

void ViewerWindow::on_image_close_act() {
	setWindowTitle(tr("Ain Viewer"));
	m_imager_viewer->setText("");
	m_imager_viewer->setToolTip("");
	if (m_preview_image) {
		delete(m_preview_image);
		m_preview_image = nullptr;
	}
	preview_image *pi = new preview_image();
	m_imager_viewer->setImage(*pi);
	delete pi;
	if (m_image_data) {
		free(m_image_data);
		m_image_data = nullptr;
	}
	m_image_list.clear();
	m_image_size = 0;
	m_image_path[0] = '\0';
	m_image_formrat = nullptr;
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
}

void ViewerWindow::on_stretch_changed(int level) {
	conf.preview_stretch_level = (preview_stretch)level;
	write_conf();
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

void ViewerWindow::show_error(const char *title, const char *message) {
	QMessageBox msgBox(this);
	indigo_error(message);
	msgBox.setWindowTitle(title);
	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(message);
	msgBox.exec();
}
