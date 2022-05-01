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
#include <raw_to_fits.h>
#include <dslr_raw.h>
#include <xisf.h>


void write_conf();

ViewerWindow::ViewerWindow(QWidget *parent) : QMainWindow(parent) {
	setWindowTitle(tr("Ain Viewer"));
	if (!conf.restore_window_size || conf.window_width == 0 || conf.window_height == 0) {
		resize(1024, 768);
	} else {
		resize(conf.window_width, conf.window_height);
	}

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

	m_image_info_dlg = new TextDialog("Image Info", this);

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
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_open_act);

	act = menu->addAction(tr("&Image Info"));
	act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_I));
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_info_act);

	act = menu->addAction(tr("&Next Image"));
	act->setShortcut(QKeySequence(Qt::Key_Right));
	act->setAutoRepeat(false);
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_next_act);

	act = menu->addAction(tr("&Previous Image"));
	act->setShortcut(QKeySequence(Qt::Key_Left));
	act->setAutoRepeat(false);
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_prev_act);

	act = menu->addAction(tr("&Close Image"));
	act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_close_act);

	menu->addSeparator();

	act = menu->addAction(tr("Convert &RAW to FITS"));
	act->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_image_raw_to_fits);

	menu->addSeparator();

	act = menu->addAction(tr("&Delete File"));
	act->setShortcut(QKeySequence(Qt::Key_Delete));
	//act->setShortcutVisibleInContextMenu(true);
	connect(act, &QAction::triggered, this, &ViewerWindow::on_delete_current_image_act);

	menu->addSeparator();

	act = menu->addAction(tr("&Exit"));
	connect(act, &QAction::triggered, this, &ViewerWindow::on_exit_act);
	menu_bar->addMenu(menu);

	menu = new QMenu("&Settings");

	act = menu->addAction(tr("Open &last file at startup"));
	act->setCheckable(true);
	act->setChecked(conf.reopen_file_at_start);
	connect(act, &QAction::toggled, this, &ViewerWindow::on_reopen_file_changed);

	act = menu->addAction(tr("&Restore window size at start"));
	act->setCheckable(true);
	act->setChecked(conf.restore_window_size);
	connect(act, &QAction::toggled, this, &ViewerWindow::on_restore_window_size_changed);

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
	m_imager_viewer->setBalance(conf.preview_color_balance);

	connect(m_imager_viewer, &ImageViewer::stretchChanged, this, &ViewerWindow::on_stretch_changed);
	connect(m_imager_viewer, &ImageViewer::BalanceChanged, this, &ViewerWindow::on_cb_changed);
	connect(m_imager_viewer, &ImageViewer::previousRequested, this, &ViewerWindow::on_image_prev_act);
	connect(m_imager_viewer, &ImageViewer::nextRequested, this, &ViewerWindow::on_image_next_act);

	m_imager_viewer->enableAntialiasing(conf.antialiasing_enabled);
	if (conf.file_open[0] != '\0') open_image(conf.file_open);
}

ViewerWindow::~ViewerWindow () {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	QSize wsize = size();
	conf.window_width = wsize.width();
	conf.window_height = wsize.height();
	write_conf();
	if (m_image_data) free(m_image_data);
	delete m_preview_image;
	delete m_imager_viewer;
	delete m_image_info_dlg;
}

/* C++ looks for method close - maybe name collision so... */
void close_fd(int fd) {
	close(fd);
}

void ViewerWindow::open_image(QString file_name) {
	char msg[PATH_LEN];
	if (file_name == "") return;
	FILE *file;
	block_scrolling(true);
	strncpy(m_image_path, file_name.toUtf8().data(), PATH_LEN);
	strncpy(conf.file_open, file_name.toUtf8().data(), PATH_LEN);
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
		block_scrolling(false);
		snprintf(msg, PATH_LEN, "File '%s'\nCan not be open for reading.", QDir::toNativeSeparators(m_image_path).toUtf8().data());
		show_message("Error!", msg);
		return;
	}

	if (m_preview_image) {
		delete(m_preview_image);
		m_preview_image = nullptr;
	}

	m_image_formrat = strrchr(m_image_path, '.');
	const stretch_config_t sc = {(uint8_t)conf.preview_stretch_level, (uint8_t)conf.preview_color_balance };
	m_preview_image = create_preview(m_image_data, m_image_size, (const char*)m_image_formrat, sc);

	if (m_preview_image) {
		m_imager_viewer->setImage(*m_preview_image);
		char info[256] = {};
		int w = m_preview_image->width();
		int h = m_preview_image->height();
		sprintf(info, "%s [%d x %d]", basename(m_image_path), w, h);
		setWindowTitle(tr("Ain Viewer - ") + QString(m_image_path));
		m_imager_viewer->setText(info);
	} else {
		block_scrolling(false);
		snprintf(msg, PATH_LEN, "File: '%s'\nDoes not seem to be a supported image format.", QDir::toNativeSeparators(m_image_path).toUtf8().data());
		show_message("Error!", msg);
	}
	block_scrolling(false);
	QDir directory(dirname(file_name.toUtf8().data()));
	QString pattern = "*" + QString(m_image_formrat);
	m_image_list = directory.entryList(QStringList() << pattern, QDir::Files);
}

void ViewerWindow::on_image_info_act() {
	char *card = (char*)m_image_data;
	char *end = card + m_image_size;
	if (m_image_data == nullptr) return;
	if (!strncmp(card, "SIMPLE", 6)) {
		if (m_image_size < 2880) return;
		m_image_info_dlg->setWindowTitle(QString("FITS Header: ") + QString(basename(m_image_path)));
		auto text = m_image_info_dlg->textWidget();
		text->clear();
 		while (card <= end) {
			char card_line[81];
			strncpy(card_line, card, 80);
			card_line[80] ='\0';
			//printf("%s\n", card_line);
			text->append(card_line);
			if (!strncmp(card, "END", 3)) break;
			card+=80;
		}
		m_image_info_dlg->show();
		m_image_info_dlg->scrollTop();
	} else if (!strncmp(card, "XISF0100", 8)) {
		/*

			int data_size;
			int uncompressed_data_size;
			int shuffle_size;
			char compression[50];

		*/
		xisf_metadata metadata;
		xisf_read_metadata((uint8_t *)m_image_data, m_image_size, &metadata);

		m_image_info_dlg->setWindowTitle(QString("Image Info: ") + QString(basename(m_image_path)));
		auto text = m_image_info_dlg->textWidget();
		text->clear();

		if (metadata.camera_name[0] != '\0') {
			text->append(QString("<b>Camera:</b> ") + metadata.camera_name);
		}

		text->append(QString("<b>Image Dimensions:</b> ") + QString::number(metadata.width) + " x " + QString::number(metadata.height));
		text->append(QString("<b>Channels:</b> ") + QString::number(metadata.channels));

		switch (metadata.bitpix) {
		case 8:
			text->append(QString("<b>Pixel Format:</b> 8-bit unsigned integer"));
			break;
		case 16:
			text->append(QString("<b>Pixel Format:</b> 16-bit unsigned integer"));
			break;
		case 32:
			text->append(QString("<b>Pixel Format:</b> 32-bit unsigned integer"));
			break;
		case -32:
			text->append(QString("<b>Pixel Format:</b> 32-bit IEEE 754 floating point"));
			break;
		case -64:
			text->append(QString("<b>Pixel Format:</b> 64-bit IEEE 754 floating point"));
			break;
		}

		if (metadata.big_endian) {
			text->append(QString("<b>Byte Order:</b> Big endian"));
		} else {
			text->append(QString("<b>Byte Order:</b> Little endian"));
		}

		text->append(QString("<b>Color space:</b> ") + metadata.color_space);
		if (!strcmp(metadata.color_space, "RGB")) {
			if (metadata.normal_pixel_storage) {
				text->append(QString("<b>Pixel Storage:</b> Normal (One RGB channel)"));
			} else {
				text->append(QString("<b>Pixel Storage:</b> Planar (R, G and B channels)"));
			}
		}

		if (metadata.bayer_pattern[0] != '\0') {
			text->append(QString("<b>Bayer Pattern:</b> ") + metadata.bayer_pattern);
		}

		text->append(QString("<b>Image Type:</b> ") + metadata.image_type);

		if (metadata.exposure_time >= 0) {
			text->append(QString("<b>Exposure Time:</b> ") + QString::number(metadata.exposure_time) + " sec");
		}

		if (metadata.sensor_temperature >= 0) {
			text->append(QString("<b>Seensor Temperature:</b> ") + QString::number(metadata.sensor_temperature) + "°C");
		}

		text->append(QString("<b>Data offset:</b> ") + QString::number(metadata.data_offset));

		if(metadata.compression[0] == '\0') {
			text->append(QString("<b>Data size:</b> ") + QString::number(metadata.data_size) + " (" + QString::number(metadata.data_size/(1024.0*1024)) + " MB)");
		} else {
			text->append(QString("<b>Compression:</b> ") + metadata.compression);
			if (metadata.shuffle_size) {
				text->append(QString("<b>Shuffle:</b> ") + QString::number(metadata.shuffle_size));
			}
			text->append(QString("<b>Data size:</b> ") + QString::number(metadata.uncompressed_data_size) + " (" + QString::number(metadata.uncompressed_data_size/(1024.0*1024)) + " MB)");
			text->append(QString("<b>Compressed size:</b> ") + QString::number(metadata.data_size) + " (" + QString::number(metadata.data_size/(1024.0*1024)) + " MB)");
			text->append(QString("<b>Compression rate:</b> ") + QString::number((float)metadata.data_size/metadata.uncompressed_data_size * 100, 'f', 0) + "%");
		}

		m_image_info_dlg->show();
		m_image_info_dlg->scrollTop();
	} else {
		dslr_raw_image_info_s image_info;
		int rc = dslr_raw_image_info((void *)m_image_data, m_image_size, &image_info);
		if (rc == LIBRAW_SUCCESS) {
			m_image_info_dlg->setWindowTitle(QString("Image Info: ") + QString(basename(m_image_path)));
			auto text = m_image_info_dlg->textWidget();
			text->clear();
			text->append(QString("<b>Camera Model:</b> ") + image_info.camera_make + " " + image_info.camera_model);
			if (
				strncmp(image_info.camera_make, image_info.normalized_camera_make, sizeof(image_info.camera_make)) ||
				strncmp(image_info.camera_model, image_info.normalized_camera_model, sizeof(image_info.camera_model))
			) {
				text->append(QString("<b>Camera Model (real):</b> ") + image_info.normalized_camera_make + " " + image_info.normalized_camera_model);
			}
			if (image_info.lens_make[0] != '\0' || image_info.lens[0] != '\0') {
				text->append(QString("<b>Lens Model:</b> ") + image_info.lens_make + " " + image_info.lens);
			}
			if (image_info.focal_len != 0) {
				text->append(QString("<b>Focal length:</b> ") + QString::number(image_info.focal_len, 'f', 0) + " mm");
			}
			if (image_info.aperture != 0) {
				text->append(QString("<b>Aperture:</b> ") + QString::number(image_info.aperture, 'f',2));
			}
			int demonimator = 1;
			if (image_info.shutter > 0 && image_info.shutter < 1) {
				int demon = rint(1 / image_info.shutter);
				text->append(QString("<b>Shutter speed:</b> ") + "1/" + QString::number(demon) + " (" + QString::number(image_info.shutter, 'f', 4) + ") sec");
			} else {
				text->append(QString("<b>Shutter speed:</b> ") + QString::number(image_info.shutter, 'f', 4) + " sec");
			}
			text->append(QString("<b>ISO Speed:</b> ") + QString::number(image_info.iso_speed, 'f', 0));
			text->append(QString("<b>Image Dimensions:</b> ") + QString::number(image_info.iwidth) + " x " + QString::number(image_info.iheight));
			if (image_info.timestamp != 0) {
				char time_str[64];
				struct tm tm;
				localtime_r(&image_info.timestamp, &tm);
				strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", &tm);
				text->append(QString("<b>Acquisition time:</b> ") + time_str);
				text->append("");
			}
			if (image_info.artist[0] != '\0') {
					text->append(QString("<b>Artist:</b> ") + image_info.artist);
			}
			if (image_info.desc[0] != '\0') {
				text->append(QString("<b>Description:</b> ") + image_info.desc);
			}
			m_image_info_dlg->show();
			m_image_info_dlg->scrollTop();
		} else {
			show_message("Error!", "Not FITS, XISF or camera raw file!");
		}
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
		QString(
			"FITS (*.fit *.FIT *.fits *.FITS *.fts *.FTS );;"
			"Indigo RAW (*.raw *.RAW);;"
			"XISF (*.xisf *.XISF);;"
			"FITS / Indigo RAW / XISF (*.fit *FIT *.fits *.FITS *.fts *.FTS *.raw *.RAW *.raw *.RAW);;"
			"Nikon NEF / NRW (*.nef *.NEF *.nrw *.NRW);;"
			"Canon CRW / CR2 (*.crw *.CRW *.cr2 *.CR2);;"
			"Sony ARW / SR2 (*.arw *.ARW *.sr2 *.SR2);;"
			"Pentax PEF (*.pef *.PEF);;"
			"Panasonic RW2 / RAW (*.rw2 *.RW2 *.raw *.RAW);;"
			"Olympus ORF (*.orf *.ORF);;"
			"Digital negative (*.dng *.DNG);;"
			"Other RAW formats (*.3fr *.3FR *.mef *.MEF *.mrw *.MRW);;"
			"JPEG / TIFF / PNG (*.jpg *.JPG *.jpeg *.JPEG *.jpe *.JPE *.tif *.TIF *.tiff *.TIFF *.png *.PNG);;"
			"All Files (*)"
		),
		&m_selected_filter
	);
	open_image(file_name);
}

void ViewerWindow::on_delete_current_image_act() {
	char path[PATH_LEN];

	if (m_image_path[0] == '\0') return;
	strncpy(path, m_image_path, PATH_LEN);

	QMessageBox msgBox;
	msgBox.setWindowTitle("Delete file");
	msgBox.setText(QString("Do you want to delete '") + basename(m_image_path) + "' ?");
	msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);
	int ret = msgBox.exec();

	if (ret == QMessageBox::Yes) {
		indigo_debug("Removing file");
		on_image_next_act();

		QFile::remove(path);

		QDir directory(dirname(path));
		QString pattern = "*" + QString(m_image_formrat);
		m_image_list = directory.entryList(QStringList() << pattern, QDir::Files);

		if (0 == m_image_list.size()) {
			on_image_close_act();
		}
	} else {
		indigo_debug("File not removed");
	}
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

void ViewerWindow::on_image_raw_to_fits() {
	char path[PATH_LEN];
	strncpy(path, m_image_path, PATH_LEN);
	QString qlocation(dirname(path));
	if (m_image_path[0] == '\0') qlocation = QDir::toNativeSeparators(QDir::homePath());
	QStringList file_names = QFileDialog::getOpenFileNames(
		this,
		tr("Select RAW Images to convert to FITS..."),
		qlocation,
		QString("Indigo RAW (*.raw *.RAW);;All Files (*)")
	);

	int file_num = file_names.size();
	printf("size = %d\n", file_num);
	if (file_num == 0) {
		return;
	}
	QProgressDialog progress("", "Abort", 0, file_num, this);
	progress.setMinimumWidth(350);
	progress.setMinimumDuration(0);
	progress.setWindowModality(Qt::WindowModal);
	int failed = 0;
	char file_name[PATH_MAX];
	for (int i = 0; i < file_num; i++) {
		progress.setValue(i);
		if (progress.wasCanceled())
			break;

		strncpy(file_name, file_names.at(i).toUtf8().data(), PATH_MAX);
		char message[500];
		snprintf(message, 500, "Converting '%s'... (%d of %d)", basename(file_name), i, file_num);
		progress.setLabelText(message);
		int res = convert_raw_to_fits(file_name);
		printf("file '%s' -> %d\n", file_name ,res);
		if (res < 0) {
			failed ++;
		}
	}
	progress.setValue(file_num);

	if (failed) {
		char message[100];
		snprintf(
			message,
			100,
			"%d file(s) succeessfully converted.\n%d file(s) failed to convert.",
			file_num - failed,
			failed
		);
		show_message("RAW to FITS conversion results", message);
	} else {
		char message[100];
		snprintf(
			message,
			100,
			"%d file(s) succeessfully converted.",
			file_num
		);
		show_message("RAW to FITS conversion results", message, QMessageBox::Information);
	}
}

void ViewerWindow::on_exit_act() {
	QApplication::quit();
}

void ViewerWindow::on_reopen_file_changed(bool status) {
	conf.reopen_file_at_start = status;
	write_conf();
}

void ViewerWindow::on_restore_window_size_changed(bool status) {
	conf.restore_window_size = status;
	write_conf();
	indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::on_antialias_view(bool status) {
	conf.antialiasing_enabled = status;
	m_imager_viewer->enableAntialiasing(status);
	write_conf();
}

void ViewerWindow::on_stretch_changed(int level) {
	conf.preview_stretch_level = (preview_stretch)level;
	if (m_preview_image) {
		block_scrolling(true);
		int width = m_preview_image->width();
		int height = m_preview_image->height();
		int pix_format = m_preview_image->m_pix_format;
		const stretch_config_t sc = {(uint8_t)conf.preview_stretch_level, (uint8_t)conf.preview_color_balance};
		preview_image *new_preview = create_preview(width, height, pix_format, m_preview_image->m_raw_data, sc);
		if (new_preview) {
			delete m_preview_image;
			m_preview_image = new_preview;
			m_imager_viewer->setImage(*m_preview_image);
		}
		block_scrolling(false);
	}
	write_conf();
}

void ViewerWindow::on_cb_changed(int balance) {
	conf.preview_color_balance = (color_balance)balance;
	if (m_preview_image) {
		block_scrolling(true);
		int width = m_preview_image->width();
		int height = m_preview_image->height();
		int pix_format = m_preview_image->m_pix_format;
		const stretch_config_t sc = {(uint8_t)conf.preview_stretch_level, (uint8_t)conf.preview_color_balance};
		preview_image *new_preview = create_preview(width, height, pix_format, m_preview_image->m_raw_data, sc);
		if (new_preview) {
			delete m_preview_image;
			m_preview_image = new_preview;
			m_imager_viewer->setImage(*m_preview_image);
		}
		block_scrolling(false);
	}
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
		"Copyright ©2020-" + YEAR_NOW + ", The INDIGO Initiative.<br>"
		"<a href='http://www.indigo-astronomy.org'>http://www.indigo-astronomy.org</a>"
	);
	msgBox.exec();
	//indigo_debug("%s\n", __FUNCTION__);
}

void ViewerWindow::show_message(const char *title, const char *message, QMessageBox::Icon icon) {
	QMessageBox msgBox(this);
	indigo_error(message);
	msgBox.setWindowTitle(title);
	msgBox.setIcon(icon);
	msgBox.setText(message);
	msgBox.exec();
}
