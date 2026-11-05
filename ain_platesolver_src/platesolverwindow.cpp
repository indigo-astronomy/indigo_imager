// Copyright (c) 2026 Rumen G.Bogdanovski
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


#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <platesolverwindow.h>

void write_conf();

#define SOLVER_PANEL_WIDTH 400
#define LOG_HEIGHT 120
#define LABEL_MARGIN 130
#define SOLVER_CONFIG_FILENAME "ain_platesolver_solver.conf"
#define ARCSEC_PER_DEG 3600.0

static QString format_ra(double hours) {
	return QString(indigo_dtos(hours, "%02d:%02d:%05.2f"));
}

static QString format_dec(double degrees) {
	return QString(indigo_dtos(degrees, "%+03d:%02d:%04.1f"));
}

// a long file name would widen the panel
static void set_file_label(QLabel *label, const QString &path) {
	QString name = QFileInfo(path).fileName();
	label->setText(label->fontMetrics().elidedText(name, Qt::ElideMiddle, SOLVER_PANEL_WIDTH - LABEL_MARGIN));
	label->setToolTip(path);
}

static QWidget *create_scroll_area(QWidget *content) {
	QScrollArea *scroll = new QScrollArea();
	scroll->setWidgetResizable(true);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidget(content);
	return scroll;
}

PlateSolverWindow::PlateSolverWindow(QWidget *parent) :
	ViewerWindow(parent, "Ain Platesolver", "Ain Plate Solver"),
	m_wcs_valid(false),
	m_showing_stack(false),
	m_wcs_jnow(false)
{
	// not needed for solving, the menu collapses the separators left around them
	delete m_raw_to_fits_act;
	delete m_quick_stack_act;
	delete m_delete_file_act;
	delete m_image_inspector_act;

	m_imager_viewer->showWCS(true);
	m_objects = new ObjectOverlay(m_imager_viewer->pixmapItem());
	connect(m_imager_viewer, &ImageViewer::zoomChanged, this, [this]() { m_objects->layout_labels(); });
	// these slots run after the viewer replaced its preview, which does not carry the solution over
	auto reapply_wcs = [this]() { apply_wcs_to_image(); };
	connect(m_imager_viewer, &ImageViewer::stretchChanged, this, reapply_wcs);
	connect(m_imager_viewer, &ImageViewer::BalanceChanged, this, reapply_wcs);
	connect(m_imager_viewer, &ImageViewer::debayerChanged, this, reapply_wcs);
	connect(m_imager_viewer, &ImageViewer::showStackChanged, this, [this](bool showing_stack) {
		m_showing_stack = showing_stack;
		apply_wcs_to_image();
	});

	QWidget *panel = new QWidget();
	panel->setFixedWidth(SOLVER_PANEL_WIDTH);
	QVBoxLayout *panel_layout = new QVBoxLayout(panel);
	panel_layout->setSpacing(5);
	panel_layout->setContentsMargins(0, 5, 5, 5);

	m_engine = new SolverEngine(this);

	m_tabs = new QTabWidget();
	m_tabs->addTab(create_solve_tab(), tr("Solve"));
	m_tabs->addTab(create_hints_tab(), tr("Hints"));
	m_tabs->addTab(create_index_tab(), tr("Indexes"));
	panel_layout->addWidget(m_tabs, 1);

	m_content_layout->addWidget(panel);

	m_log = new QPlainTextEdit();
	m_log->setReadOnly(true);
	m_log->setMaximumBlockCount(1000);
	m_log->setFixedHeight(LOG_HEIGHT);
	m_image_layout->setStretchFactor(m_imager_viewer, 1);
	m_image_layout->addWidget(m_log);

	m_download_timer = new QTimer(this);
	m_download_timer->setInterval(1000);
	connect(m_download_timer, &QTimer::timeout, this, &PlateSolverWindow::on_download_timer);

	connect(this, &ViewerWindow::image_changed, this, &PlateSolverWindow::on_image_changed);

	connect(m_engine, &SolverEngine::indexes_changed, this, &PlateSolverWindow::update_indexes);
	connect(m_engine, &SolverEngine::message_received, this, &PlateSolverWindow::log_message);
	connect(m_engine, &SolverEngine::solve_finished, this, &PlateSolverWindow::on_solve_finished);

	load_solver_settings();
	update_indexes();
	// the constructor of the viewer may have opened an image before the connection above
	on_image_changed();
}

PlateSolverWindow::~PlateSolverWindow() {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	delete m_objects;
	disconnect(m_engine, nullptr, this, nullptr);
}

QWidget *PlateSolverWindow::create_solve_tab() {
	QWidget *content = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setSpacing(8);

	QHBoxLayout *buttons = new QHBoxLayout();
	m_solve_button = new QPushButton(tr("Solve Image"));
	m_solve_button->setObjectName("solve_button");
	connect(m_solve_button, &QPushButton::clicked, this, &PlateSolverWindow::on_solve_clicked);
	buttons->addWidget(m_solve_button);
	m_abort_button = new QPushButton(tr("Abort"));
	m_abort_button->setObjectName("abort_button");
	connect(m_abort_button, &QPushButton::clicked, this, &PlateSolverWindow::on_abort_clicked);
	buttons->addWidget(m_abort_button);
	layout->addLayout(buttons);

	m_index_warning = new QLabel();
	m_index_warning->setObjectName("index_warning");
	m_index_warning->setWordWrap(true);
	m_index_warning->setStyleSheet("color: #E0A000;");
	layout->addWidget(m_index_warning);

	QHBoxLayout *objects = new QHBoxLayout();
	m_show_deep_sky = new QCheckBox(tr("Deep sky"));
	m_show_deep_sky->setObjectName("show_deep_sky");
	m_show_deep_sky->setToolTip(tr("Show the deep sky objects on a solved image"));
	m_show_deep_sky->setChecked(conf.show_deep_sky);
	connect(m_show_deep_sky, &QCheckBox::toggled, this, &PlateSolverWindow::on_show_deep_sky);
	objects->addWidget(m_show_deep_sky);
	m_show_stars = new QCheckBox(tr("Stars"));
	m_show_stars->setObjectName("show_stars");
	m_show_stars->setToolTip(tr("Show the brightest stars on a solved image"));
	m_show_stars->setChecked(conf.show_stars);
	connect(m_show_stars, &QCheckBox::toggled, this, &PlateSolverWindow::on_show_stars);
	objects->addWidget(m_show_stars);
	layout->addLayout(objects);

	QGroupBox *solution = new QGroupBox(tr("Solution"));
	QFormLayout *solution_form = new QFormLayout(solution);
	QHBoxLayout *solution_status = new QHBoxLayout();
	solution_status->setSpacing(6);
	m_solution_led = new QLabel();
	solution_status->addWidget(m_solution_led);
	m_solution_image = new QLabel();
	solution_status->addWidget(m_solution_image, 1);
	solution_form->addRow(solution_status);
	const QVector<QPair<QString, QString>> wcs_rows = {
		{"RA", tr("RA:")},
		{"DEC", tr("Dec:")},
		{"EPOCH", tr("Epoch:")},
		{"ANGLE", tr("Angle:")},
		{"FIELD", tr("Field:")},
		{"SCALE", tr("Scale:")},
		{"PARITY", tr("Parity:")},
		{"INDEX", tr("Index:")}
	};
	for (const QPair<QString, QString> &row : wcs_rows) {
		QLabel *value = new QLabel();
		value->setTextInteractionFlags(Qt::TextSelectableByMouse);
		solution_form->addRow(row.second, value);
		m_wcs_labels[row.first] = value;
		m_wcs_rows.append({row.second, value});
	}
	QHBoxLayout *copy_buttons = new QHBoxLayout();
	copy_buttons->setContentsMargins(0, 12, 0, 0);
	m_copy_coordinates_button = new QPushButton(tr("Copy Coordinates"));
	m_copy_coordinates_button->setObjectName("copy_coordinates_button");
	m_copy_coordinates_button->setToolTip(tr("Copy RA and Dec of the image center to the clipboard"));
	connect(m_copy_coordinates_button, &QPushButton::clicked, this, &PlateSolverWindow::on_copy_coordinates_clicked);
	copy_buttons->addWidget(m_copy_coordinates_button);
	m_copy_solution_button = new QPushButton(tr("Copy Solution"));
	m_copy_solution_button->setObjectName("copy_solution_button");
	m_copy_solution_button->setToolTip(tr("Copy the whole solution to the clipboard"));
	connect(m_copy_solution_button, &QPushButton::clicked, this, &PlateSolverWindow::on_copy_solution_clicked);
	copy_buttons->addWidget(m_copy_solution_button);
	solution_form->addRow(copy_buttons);
	layout->addWidget(solution);

	layout->addStretch(1);

	return create_scroll_area(content);
}

QWidget *PlateSolverWindow::create_index_tab() {
	QWidget *content = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setSpacing(8);

	QLabel *info = new QLabel(tr("Checking an index downloads it, clearing it deletes the file. Use the index whose scale is nearest to 0.7 x the size of the frame."));
	info->setWordWrap(true);
	layout->addWidget(info);

	QGroupBox *installed = new QGroupBox(tr("Installed indexes"));
	m_index_layout = new QVBoxLayout(installed);
	layout->addWidget(installed);

	QGroupBox *use_index = new QGroupBox(tr("Use indexes"));
	m_use_index_layout = new QVBoxLayout(use_index);
	m_use_index_empty = new QLabel(tr("No index installed, check one above to download it."));
	m_use_index_empty->setWordWrap(true);
	m_use_index_layout->addWidget(m_use_index_empty);
	layout->addWidget(use_index);

	m_download_status = new QLabel();
	m_download_status->setWordWrap(true);
	layout->addWidget(m_download_status);

	m_download_progress = new QProgressBar();
	m_download_progress->setRange(0, 100);
	m_download_progress->setVisible(false);
	layout->addWidget(m_download_progress);

	m_download_abort = new QPushButton(tr("Abort Download"));
	m_download_abort->setVisible(false);
	connect(m_download_abort, &QPushButton::clicked, this, &PlateSolverWindow::on_abort_clicked);
	layout->addWidget(m_download_abort);

	QLabel *folder = new QLabel(tr("Folder: %1").arg(QDir::toNativeSeparators(m_engine->index_folder())));
	folder->setWordWrap(true);
	folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(folder);

	layout->addStretch(1);
	return create_scroll_area(content);
}

QWidget *PlateSolverWindow::create_hints_tab() {
	QWidget *content = new QWidget();
	QFormLayout *hints_form = new QFormLayout(content);

	m_hint_radius = new QDoubleSpinBox();
	m_hint_radius->setRange(0, 360);
	m_hint_radius->setDecimals(1);
	m_hint_radius->setSuffix("°");
	m_hint_radius->setSpecialValueText(tr("Blind"));
	m_hint_radius->setToolTip(tr("Search around RA and Dec, blind searches the whole sky"));
	connect(m_hint_radius, &QDoubleSpinBox::editingFinished, this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Radius:"), m_hint_radius);

	m_hint_ra = new QLineEdit();
	m_hint_ra->setPlaceholderText("hh:mm:ss");
	connect(m_hint_ra, &QLineEdit::editingFinished, this, &PlateSolverWindow::on_hints_edited);

	m_hint_dec = new QLineEdit();
	m_hint_dec->setPlaceholderText("dd:mm:ss");
	connect(m_hint_dec, &QLineEdit::editingFinished, this, &PlateSolverWindow::on_hints_edited);

	QHBoxLayout *coordinates = new QHBoxLayout();
	coordinates->addWidget(m_hint_ra, 1);
	coordinates->addWidget(m_hint_dec, 1);
	hints_form->addRow(tr("RA / Dec:"), coordinates);

	m_hint_epoch = new QComboBox();
	m_hint_epoch->addItem(tr("J2000"), 2000.0);
	m_hint_epoch->addItem(tr("JNow"), 0.0);
	m_hint_epoch->setToolTip(tr("Epoch of RA and Dec above and of the solution"));
	connect(m_hint_epoch, QOverload<int>::of(&QComboBox::activated), this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Epoch:"), m_hint_epoch);

	m_hint_scale = new QDoubleSpinBox();
	m_hint_scale->setRange(0, 18000);
	m_hint_scale->setDecimals(2);
	m_hint_scale->setSuffix(" \"/px");
	m_hint_scale->setSpecialValueText(tr("Unknown"));
	m_hint_scale->setToolTip(tr("Tries the index fitting the field first"));
	connect(m_hint_scale, &QDoubleSpinBox::editingFinished, this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Pixel scale:"), m_hint_scale);

	m_hint_parity = new QComboBox();
	m_hint_parity->addItem(tr("Any"), 0.0);
	m_hint_parity->addItem(tr("Normal"), -1.0);
	m_hint_parity->addItem(tr("Mirrored"), 1.0);
	connect(m_hint_parity, QOverload<int>::of(&QComboBox::activated), this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Parity:"), m_hint_parity);

	m_hint_downsample = new QSpinBox();
	m_hint_downsample->setRange(1, 16);
	connect(m_hint_downsample, &QSpinBox::editingFinished, this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Downsample:"), m_hint_downsample);

	m_hint_depth = new QSpinBox();
	m_hint_depth->setRange(0, 1000);
	m_hint_depth->setSpecialValueText(tr("Auto"));
	m_hint_depth->setToolTip(tr("Tries the brightest stars first, then all detected stars"));
	connect(m_hint_depth, &QSpinBox::editingFinished, this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Depth:"), m_hint_depth);

	m_hint_cpu_limit = new QSpinBox();
	m_hint_cpu_limit->setRange(0, 600);
	m_hint_cpu_limit->setSuffix(" s");
	m_hint_cpu_limit->setSpecialValueText(tr("None"));
	connect(m_hint_cpu_limit, &QSpinBox::editingFinished, this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Time limit:"), m_hint_cpu_limit);

	return create_scroll_area(content);
}

static QString solver_settings_path() {
	return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/" + SOLVER_CONFIG_FILENAME;
}

void PlateSolverWindow::load_solver_settings() {
	QSettings settings(solver_settings_path(), QSettings::IniFormat);
	const SolverHints defaults;
	const double epoch = settings.value("hints/epoch", defaults.epoch).toDouble();
	const int parity = settings.value("hints/parity", defaults.parity).toInt();
	m_hint_radius->setValue(settings.value("hints/radius", defaults.radius).toDouble());
	m_hint_ra->setText(format_ra(settings.value("hints/ra", defaults.ra).toDouble()));
	m_hint_dec->setText(format_dec(settings.value("hints/dec", defaults.dec).toDouble()));
	m_hint_epoch->setCurrentIndex(epoch == 0 ? 1 : 0);
	m_hint_scale->setValue(settings.value("hints/scale", defaults.scale).toDouble() * ARCSEC_PER_DEG);
	m_hint_parity->setCurrentIndex(parity < 0 ? 1 : parity > 0 ? 2 : 0);
	m_hint_downsample->setValue(settings.value("hints/downsample", defaults.downsample).toInt());
	m_hint_depth->setValue(settings.value("hints/depth", defaults.depth).toInt());
	m_hint_cpu_limit->setValue(settings.value("hints/cpu_limit", defaults.cpu_limit).toInt());
	const QStringList used = settings.value("indexes/used").toStringList();
	for (const SolverIndex &index : m_engine->indexes()) {
		m_engine->set_index_used(index.name, used.contains(index.name));
	}
}

SolverHints PlateSolverWindow::current_hints() const {
	SolverHints hints;
	hints.radius = m_hint_radius->value();
	hints.ra = indigo_stod(m_hint_ra->text().toUtf8().constData());
	hints.dec = indigo_stod(m_hint_dec->text().toUtf8().constData());
	hints.epoch = m_hint_epoch->currentData().toDouble();
	hints.scale = m_hint_scale->value() / ARCSEC_PER_DEG;
	hints.parity = (int)m_hint_parity->currentData().toDouble();
	hints.downsample = m_hint_downsample->value();
	hints.depth = m_hint_depth->value();
	hints.cpu_limit = m_hint_cpu_limit->value();
	return hints;
}

void PlateSolverWindow::on_hints_edited() {
	const SolverHints hints = current_hints();
	QSettings settings(solver_settings_path(), QSettings::IniFormat);
	settings.setValue("hints/radius", hints.radius);
	settings.setValue("hints/ra", hints.ra);
	settings.setValue("hints/dec", hints.dec);
	settings.setValue("hints/epoch", hints.epoch);
	settings.setValue("hints/scale", hints.scale);
	settings.setValue("hints/parity", hints.parity);
	settings.setValue("hints/downsample", hints.downsample);
	settings.setValue("hints/depth", hints.depth);
	settings.setValue("hints/cpu_limit", hints.cpu_limit);
}

void PlateSolverWindow::save_used_indexes() {
	QStringList used;
	for (const SolverIndex &index : m_engine->indexes()) {
		if (index.listed && index.used) used.append(index.name);
	}
	QSettings settings(solver_settings_path(), QSettings::IniFormat);
	settings.setValue("indexes/used", used);
}

void PlateSolverWindow::set_solution_led(const char *color) {
	m_solution_led->setPixmap(QPixmap(QString(":resource/led-%1.png").arg(color)));
}

void PlateSolverWindow::clear_solution() {
	for (QLabel *label : m_wcs_labels) {
		label->setText("-");
	}
	set_solution_led("grey");
	set_file_label(m_solution_image, m_current_image);
	m_copy_coordinates_button->setEnabled(false);
	m_copy_solution_button->setEnabled(false);
}

void PlateSolverWindow::on_solve_finished(bool solved, const SolverSolution &solution, const QString &message) {
	if (!message.isEmpty()) {
		log_message(message);
	}
	if (solved) {
		m_wcs_labels["RA"]->setText(format_ra(solution.ra));
		m_wcs_labels["DEC"]->setText(format_dec(solution.dec));
		m_wcs_labels["EPOCH"]->setText(solution.epoch == 0 ? tr("JNow") : tr("J2000"));
		m_wcs_labels["ANGLE"]->setText(QString("%1°").arg(solution.angle, 0, 'f', 2));
		m_wcs_labels["FIELD"]->setText(QString("%1° x %2°").arg(solution.width, 0, 'f', 3).arg(solution.height, 0, 'f', 3));
		m_wcs_labels["SCALE"]->setText(QString("%1 \"/px").arg(solution.scale * ARCSEC_PER_DEG, 0, 'f', 3));
		m_wcs_labels["PARITY"]->setText(solution.parity < 0 ? tr("Normal") : tr("Mirrored"));
		m_wcs_labels["INDEX"]->setText(QString("%1°").arg(solution.index, 0, 'g', 3));
		set_solution_led("green");
		set_file_label(m_solution_image, m_solving_image);
		m_wcs_image = m_solving_image;
		m_wcs_ra = solution.ra;
		m_wcs_dec = solution.dec;
		m_wcs_angle = solution.angle;
		m_wcs_scale = solution.scale;
		m_wcs_parity = solution.parity;
		m_wcs_jnow = solution.epoch == 0;
		m_wcs_valid = true;
		m_copy_coordinates_button->setEnabled(true);
		m_copy_solution_button->setEnabled(true);
		apply_wcs_to_image();
	} else {
		clear_solution();
		set_solution_led("red");
		set_file_label(m_solution_image, m_solving_image);
		if (m_wcs_valid && m_wcs_image == m_solving_image) {
			remove_wcs_from_image();
		}
	}
	update_controls();
}

// the same as a file solved in Ain Imager, the viewer shows RA and Dec under the mouse if the image has WCS data
void PlateSolverWindow::apply_wcs_to_image() {
	if (m_wcs_valid && !m_showing_stack && m_preview_image != nullptr && m_wcs_image == QString(m_image_path)) {
		m_preview_image->set_wcs_data(m_wcs_ra * 15, m_wcs_dec, m_wcs_ra * 15, m_wcs_dec, m_wcs_angle, m_wcs_parity, m_wcs_scale);
		m_imager_viewer->setImage(*m_preview_image);
	}
	update_objects();
}

void PlateSolverWindow::remove_wcs_from_image() {
	m_wcs_valid = false;
	if (!m_showing_stack && m_preview_image != nullptr && m_wcs_image == QString(m_image_path)) {
		m_preview_image->set_wcs_data(0, 0, 0, 0, 0, 0, 0);
		m_imager_viewer->setImage(*m_preview_image);
	}
	update_objects();
}

void PlateSolverWindow::update_objects() {
	const bool deep_sky = m_show_deep_sky->isChecked();
	const bool stars = m_show_stars->isChecked();
	int deep_sky_count = 0;
	int star_count = 0;
	bool shown = (deep_sky || stars) && m_wcs_valid && !m_showing_stack && m_preview_image != nullptr && m_wcs_image == QString(m_image_path);
	if (shown) {
		m_objects->show(*m_preview_image, m_wcs_jnow, deep_sky, stars, &deep_sky_count, &star_count);
	} else {
		m_objects->clear();
	}
	m_show_deep_sky->setText(shown && deep_sky ? tr("Deep sky (%1)").arg(deep_sky_count) : tr("Deep sky"));
	m_show_stars->setText(shown && stars ? tr("Stars (%1)").arg(star_count) : tr("Stars"));
}

void PlateSolverWindow::on_show_deep_sky(bool show) {
	conf.show_deep_sky = show;
	write_conf();
	update_objects();
}

void PlateSolverWindow::on_show_stars(bool show) {
	conf.show_stars = show;
	write_conf();
	update_objects();
}

void PlateSolverWindow::update_indexes() {
	for (const SolverIndex &index : m_engine->indexes()) {
		QCheckBox *box = m_index_boxes.value(index.name, nullptr);
		if (box == nullptr) {
			box = new QCheckBox(index.size_label);
			box->setObjectName(index.name);
			m_index_layout->addWidget(box);
			m_index_boxes[index.name] = box;
			const QString name = index.name;
			connect(box, &QCheckBox::clicked, this, [this, name](bool checked) {
				on_index_toggled(name, checked);
			});
		}
		QSignalBlocker blocker(box);
		box->setChecked(index.wanted);

		QCheckBox *use_box = m_use_index_boxes.value(index.name, nullptr);
		if (!index.listed) {
			delete use_box;
			m_use_index_boxes.remove(index.name);
			continue;
		}
		if (use_box == nullptr) {
			use_box = new QCheckBox(index.label);
			m_use_index_boxes[index.name] = use_box;
			// the indexes stay in the order of their scale
			int position = 0;
			for (auto it = m_use_index_boxes.constBegin(); it != m_use_index_boxes.constEnd() && it.key() != index.name; ++it) {
				position++;
			}
			m_use_index_layout->insertWidget(position + 1, use_box);
			connect(use_box, &QCheckBox::clicked, this, &PlateSolverWindow::on_use_index_toggled);
		}
		QSignalBlocker use_blocker(use_box);
		use_box->setChecked(index.used);
	}
	m_use_index_empty->setVisible(m_use_index_boxes.isEmpty());
	save_used_indexes();

	const bool busy = m_engine->index_busy();
	m_download_progress->setVisible(busy);
	m_download_abort->setVisible(busy);
	if (busy) {
		m_download_timer->start();
		on_download_timer();
	} else {
		m_download_timer->stop();
		m_download_status->setText(m_engine->index_failed() ? tr("The last index change failed, see the log.") : QString());
	}
	update_controls();
}

void PlateSolverWindow::on_use_index_toggled() {
	for (auto it = m_use_index_boxes.constBegin(); it != m_use_index_boxes.constEnd(); ++it) {
		m_engine->set_index_used(it.key(), it.value()->isChecked());
	}
	save_used_indexes();
	update_controls();
}

void PlateSolverWindow::on_index_toggled(const QString &name, bool checked) {
	QCheckBox *box = m_index_boxes.value(name, nullptr);
	if (box == nullptr) return;
	if (!checked) {
		QMessageBox::StandardButton answer = QMessageBox::warning(
			this,
			tr("Delete index"),
			tr("Delete index %1?\nIt has to be downloaded again to use it.").arg(box->text()),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No
		);
		if (answer != QMessageBox::Yes) {
			box->setChecked(true);
			return;
		}
	}
	m_engine->set_index_wanted(name, checked);
}

void PlateSolverWindow::on_download_timer() {
	const int received = m_engine->download_received_mb();
	const int size = m_engine->download_size_mb();
	m_download_status->setText(tr("Downloading %1: %2 of %3 MB").arg(m_engine->download_name()).arg(received).arg(size));
	// the size is approximate, the file can be a bit larger
	m_download_progress->setValue(size > 0 ? qMin(received * 100 / size, 100) : 0);
}

void PlateSolverWindow::on_image_changed() {
	bool has_image = m_preview_image != nullptr && m_image_data != nullptr && m_image_size > 0;
	m_wcs_valid = false;
	update_objects();
	m_current_image = has_image ? QString(m_image_path) : QString();
	if (!m_engine->solving()) {
		clear_solution();
	}
	update_controls();
}

void PlateSolverWindow::on_solve_clicked() {
	if (m_current_image.isEmpty()) return;
	on_hints_edited();
	m_solving_image = m_current_image;
	log_message(tr("Solving %1").arg(QFileInfo(m_solving_image).fileName()));
	const char *format = strrchr(m_image_path, '.');
	if (!m_engine->solve(m_image_data, m_image_size, format != nullptr ? format : "", current_hints())) {
		log_message(tr("The solver is busy"));
	} else {
		// the values of the previous solution do not belong to the image being solved
		clear_solution();
		set_solution_led("orange");
		set_file_label(m_solution_image, m_solving_image);
	}
	update_controls();
}

void PlateSolverWindow::on_abort_clicked() {
	m_engine->abort();
}

void PlateSolverWindow::on_copy_coordinates_clicked() {
	QGuiApplication::clipboard()->setText(QString("%1 %2").arg(m_wcs_labels["RA"]->text(), m_wcs_labels["DEC"]->text()));
	log_message(tr("Coordinates copied to the clipboard"));
}

void PlateSolverWindow::on_copy_solution_clicked() {
	QStringList lines = {tr("Image: %1").arg(QFileInfo(m_wcs_image).fileName())};
	for (const QPair<QString, QLabel *> &row : m_wcs_rows) {
		lines.append(QString("%1 %2").arg(row.first, row.second->text()));
	}
	QGuiApplication::clipboard()->setText(lines.join("\n"));
	log_message(tr("Solution copied to the clipboard"));
}

void PlateSolverWindow::update_controls() {
	const bool solving = m_engine->solving();
	const bool index_busy = m_engine->index_busy();
	const bool any_used = m_engine->any_index_used();
	m_solve_button->setEnabled(!solving && !index_busy && !m_current_image.isEmpty() && any_used);
	m_abort_button->setEnabled(solving);
	// the indexes are in use during a solve
	for (QCheckBox *box : m_index_boxes) {
		box->setEnabled(!solving);
	}
	if (index_busy) {
		m_solve_button->setToolTip(tr("Index download in progress"));
	} else if (!any_used) {
		m_solve_button->setToolTip(tr("Select at least one index to use in the Indexes tab"));
	} else {
		m_solve_button->setToolTip(QString());
	}
	if (m_use_index_boxes.isEmpty()) {
		m_index_warning->setText(tr("No indexes installed, download the ones you need in the Indexes tab."));
	} else {
		m_index_warning->setText(tr("No indexes selected, select the ones you need in the Indexes tab."));
	}
	m_index_warning->setVisible(!any_used);
}

void PlateSolverWindow::log_message(const QString &message) {
	m_log->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss ") + message);
}
