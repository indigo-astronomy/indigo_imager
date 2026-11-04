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


#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QProcessEnvironment>
#include <QScrollArea>
#include <QSignalBlocker>
#include <indigo/indigo_names.h>
#include <platesolverwindow.h>

#define SOLVER_PANEL_WIDTH 400
#define LABEL_MARGIN 130
#define SOLVE_START_TIMEOUT_MS 5000
#define ARCSEC_PER_DEG 3600.0

static QString format_ra(double hours) {
	return QString(indigo_dtos(hours, "%02d:%02d:%05.2f"));
}

static QString format_dec(double degrees) {
	return QString(indigo_dtos(degrees, "%+03d:%02d:%04.1f"));
}

// a long file name would widen the panel
static void set_file_label(QLabel *label, const QString &format, const QString &path) {
	QString name = QFileInfo(path).fileName();
	label->setText(format.arg(label->fontMetrics().elidedText(name, Qt::ElideMiddle, SOLVER_PANEL_WIDTH - LABEL_MARGIN)));
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
	m_agent_loaded(false),
	m_solve_requested(false),
	m_wcs_busy(false),
	m_index_busy(false)
{
	QWidget *panel = new QWidget();
	panel->setFixedWidth(SOLVER_PANEL_WIDTH);
	QVBoxLayout *panel_layout = new QVBoxLayout(panel);
	panel_layout->setSpacing(5);
	panel_layout->setContentsMargins(0, 5, 5, 5);

	m_agent_status = new QLabel();
	m_agent_status->setWordWrap(true);
	panel_layout->addWidget(m_agent_status);

	m_tabs = new QTabWidget();
	m_tabs->addTab(create_solve_tab(), tr("Solve"));
	m_tabs->addTab(create_index_tab(), tr("Indexes"));
	panel_layout->addWidget(m_tabs, 3);

	m_log = new QPlainTextEdit();
	m_log->setReadOnly(true);
	m_log->setMaximumBlockCount(1000);
	panel_layout->addWidget(m_log, 1);

	m_content_layout->addWidget(panel);

	m_download_timer = new QTimer(this);
	m_download_timer->setInterval(1000);
	connect(m_download_timer, &QTimer::timeout, this, &PlateSolverWindow::on_download_timer);

	connect(this, &ViewerWindow::image_changed, this, &PlateSolverWindow::on_image_changed);

	m_client = new SolverClient(this);
	connect(m_client, &SolverClient::property_defined, this, &PlateSolverWindow::on_property_defined, Qt::QueuedConnection);
	connect(m_client, &SolverClient::property_updated, this, &PlateSolverWindow::on_property_updated, Qt::QueuedConnection);
	connect(m_client, &SolverClient::property_deleted, this, &PlateSolverWindow::on_property_deleted, Qt::QueuedConnection);
	connect(m_client, &SolverClient::message_received, this, &PlateSolverWindow::on_message, Qt::QueuedConnection);

	m_agent_loaded = m_client->load_agent();
	if (m_agent_loaded) {
		m_agent_status->setText(tr("%1 loaded").arg(PLATESOLVER_DEVICE_NAME));
	} else {
		m_agent_status->setText(tr("<b>%1 is not available.</b> Install an INDIGO version that provides %2, images can be viewed but not solved.").arg(PLATESOLVER_DEVICE_NAME, PLATESOLVER_DRIVER_NAME));
		log_message(m_client->last_error());
	}
	// the constructor of the viewer may have opened an image before the connection above
	on_image_changed();
}

PlateSolverWindow::~PlateSolverWindow() {
	indigo_debug("CALLED: %s\n", __FUNCTION__);
	disconnect(m_client, nullptr, this, nullptr);
	m_client->stop();
}

QWidget *PlateSolverWindow::create_solve_tab() {
	QWidget *content = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(content);
	layout->setSpacing(8);

	m_image_label = new QLabel();
	layout->addWidget(m_image_label);

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

	QGroupBox *solution = new QGroupBox(tr("Solution"));
	QFormLayout *solution_form = new QFormLayout(solution);
	m_solution_image = new QLabel();
	solution_form->addRow(m_solution_image);
	const QVector<QPair<QString, QString>> wcs_rows = {
		{AGENT_PLATESOLVER_WCS_RA_ITEM_NAME, tr("RA:")},
		{AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME, tr("Dec:")},
		{AGENT_PLATESOLVER_WCS_EPOCH_ITEM_NAME, tr("Epoch:")},
		{AGENT_PLATESOLVER_WCS_ANGLE_ITEM_NAME, tr("Angle:")},
		{AGENT_PLATESOLVER_WCS_WIDTH_ITEM_NAME, tr("Field:")},
		{AGENT_PLATESOLVER_WCS_SCALE_ITEM_NAME, tr("Scale:")},
		{AGENT_PLATESOLVER_WCS_PARITY_ITEM_NAME, tr("Parity:")},
		{AGENT_PLATESOLVER_WCS_INDEX_ITEM_NAME, tr("Index:")}
	};
	for (const QPair<QString, QString> &row : wcs_rows) {
		QLabel *value = new QLabel();
		value->setTextInteractionFlags(Qt::TextSelectableByMouse);
		solution_form->addRow(row.second, value);
		m_wcs_labels[row.first] = value;
	}
	layout->addWidget(solution);

	QGroupBox *hints = new QGroupBox(tr("Hints"));
	QFormLayout *hints_form = new QFormLayout(hints);

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
	hints_form->addRow(tr("RA:"), m_hint_ra);

	m_hint_dec = new QLineEdit();
	m_hint_dec->setPlaceholderText("dd:mm:ss");
	connect(m_hint_dec, &QLineEdit::editingFinished, this, &PlateSolverWindow::on_hints_edited);
	hints_form->addRow(tr("Dec:"), m_hint_dec);

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

	layout->addWidget(hints);
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
	m_download_progress->setRange(0, 0);
	m_download_progress->setTextVisible(false);
	m_download_progress->setVisible(false);
	layout->addWidget(m_download_progress);

	m_download_abort = new QPushButton(tr("Abort Download"));
	m_download_abort->setVisible(false);
	connect(m_download_abort, &QPushButton::clicked, this, &PlateSolverWindow::on_abort_clicked);
	layout->addWidget(m_download_abort);

	QLabel *folder = new QLabel(tr("Folder: %1").arg(QDir::toNativeSeparators(index_folder())));
	folder->setWordWrap(true);
	folder->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(folder);

	layout->addStretch(1);
	return create_scroll_area(content);
}

// where the agent keeps the indexes, used only to show the folder and the size of a running download
QString PlateSolverWindow::index_folder() const {
	QString base = QProcessEnvironment::systemEnvironment().value("INDIGO_CACHE_BASE");
	if (!base.isEmpty()) return base + "/solver";
	return QDir::homePath() + "/.indigo/solver";
}

void PlateSolverWindow::on_property_defined(const SolverProperty &property) {
	on_property_updated(property);
}

void PlateSolverWindow::on_property_updated(const SolverProperty &property) {
	if (property.name == AGENT_PLATESOLVER_WCS_PROPERTY_NAME) {
		update_wcs(property);
	} else if (property.name == AGENT_PLATESOLVER_HINTS_PROPERTY_NAME) {
		update_hints(property);
	} else if (property.name == AGENT_PLATESOLVER_USE_INDEX_PROPERTY_NAME) {
		update_use_index(property);
	} else if (property.name == SOLVER_INDEX_PROPERTY_NAME) {
		update_index(property);
	}
}

void PlateSolverWindow::on_property_deleted(const QString &name) {
	if (name == AGENT_PLATESOLVER_USE_INDEX_PROPERTY_NAME) {
		SolverProperty none;
		none.name = name;
		none.state = INDIGO_OK_STATE;
		update_use_index(none);
	}
}

void PlateSolverWindow::update_hints(const SolverProperty &property) {
	const SolverItem *item;
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_RADIUS_ITEM_NAME))) m_hint_radius->setValue(item->number);
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_RA_ITEM_NAME))) m_hint_ra->setText(format_ra(item->number));
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_DEC_ITEM_NAME))) m_hint_dec->setText(format_dec(item->number));
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_EPOCH_ITEM_NAME))) m_hint_epoch->setCurrentIndex(item->number == 0 ? 1 : 0);
	// a negative scale means the scale of the camera of a related agent, here there is none
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_SCALE_ITEM_NAME))) m_hint_scale->setValue(item->number > 0 ? item->number * ARCSEC_PER_DEG : 0);
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_PARITY_ITEM_NAME))) m_hint_parity->setCurrentIndex(item->number < 0 ? 1 : item->number > 0 ? 2 : 0);
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM_NAME))) m_hint_downsample->setValue((int)item->number);
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_DEPTH_ITEM_NAME))) m_hint_depth->setValue((int)item->number);
	if ((item = property.item(AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM_NAME))) m_hint_cpu_limit->setValue((int)item->number);
}

void PlateSolverWindow::on_hints_edited() {
	if (!m_agent_loaded) return;
	const QVector<QPair<const char *, double>> values = {
		{AGENT_PLATESOLVER_HINTS_RADIUS_ITEM_NAME, m_hint_radius->value()},
		{AGENT_PLATESOLVER_HINTS_RA_ITEM_NAME, indigo_stod(m_hint_ra->text().toUtf8().constData())},
		{AGENT_PLATESOLVER_HINTS_DEC_ITEM_NAME, indigo_stod(m_hint_dec->text().toUtf8().constData())},
		{AGENT_PLATESOLVER_HINTS_EPOCH_ITEM_NAME, m_hint_epoch->currentData().toDouble()},
		{AGENT_PLATESOLVER_HINTS_SCALE_ITEM_NAME, m_hint_scale->value() / ARCSEC_PER_DEG},
		{AGENT_PLATESOLVER_HINTS_PARITY_ITEM_NAME, m_hint_parity->currentData().toDouble()},
		{AGENT_PLATESOLVER_HINTS_DOWNSAMPLE_ITEM_NAME, (double)m_hint_downsample->value()},
		{AGENT_PLATESOLVER_HINTS_DEPTH_ITEM_NAME, (double)m_hint_depth->value()},
		{AGENT_PLATESOLVER_HINTS_CPU_LIMIT_ITEM_NAME, (double)m_hint_cpu_limit->value()}
	};
	QVector<SolverItem> items;
	for (const QPair<const char *, double> &value : values) {
		SolverItem item;
		item.name = value.first;
		item.number = value.second;
		item.on = false;
		items.append(item);
	}
	m_client->set_numbers(AGENT_PLATESOLVER_HINTS_PROPERTY_NAME, items);
}

void PlateSolverWindow::clear_solution() {
	for (QLabel *label : m_wcs_labels) {
		label->setText("-");
	}
	m_solution_image->setText(tr("Not solved"));
	m_solution_image->setToolTip(QString());
}

void PlateSolverWindow::update_wcs(const SolverProperty &property) {
	bool was_busy = m_wcs_busy;
	m_wcs_busy = property.state == INDIGO_BUSY_STATE;
	if (!m_wcs_busy) {
		m_solve_requested = false;
	}
	if (property.state == INDIGO_OK_STATE && was_busy) {
		const SolverItem *ra = property.item(AGENT_PLATESOLVER_WCS_RA_ITEM_NAME);
		const SolverItem *dec = property.item(AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME);
		const SolverItem *epoch = property.item(AGENT_PLATESOLVER_WCS_EPOCH_ITEM_NAME);
		const SolverItem *angle = property.item(AGENT_PLATESOLVER_WCS_ANGLE_ITEM_NAME);
		const SolverItem *width = property.item(AGENT_PLATESOLVER_WCS_WIDTH_ITEM_NAME);
		const SolverItem *height = property.item(AGENT_PLATESOLVER_WCS_HEIGHT_ITEM_NAME);
		const SolverItem *scale = property.item(AGENT_PLATESOLVER_WCS_SCALE_ITEM_NAME);
		const SolverItem *parity = property.item(AGENT_PLATESOLVER_WCS_PARITY_ITEM_NAME);
		const SolverItem *index = property.item(AGENT_PLATESOLVER_WCS_INDEX_ITEM_NAME);
		if (ra && dec && epoch && angle && width && height && scale && parity && index) {
			m_wcs_labels[AGENT_PLATESOLVER_WCS_RA_ITEM_NAME]->setText(format_ra(ra->number));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_DEC_ITEM_NAME]->setText(format_dec(dec->number));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_EPOCH_ITEM_NAME]->setText(epoch->number == 0 ? tr("JNow") : tr("J2000"));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_ANGLE_ITEM_NAME]->setText(QString("%1°").arg(angle->number, 0, 'f', 2));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_WIDTH_ITEM_NAME]->setText(QString("%1° x %2°").arg(width->number, 0, 'f', 3).arg(height->number, 0, 'f', 3));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_SCALE_ITEM_NAME]->setText(QString("%1 \"/px").arg(scale->number * ARCSEC_PER_DEG, 0, 'f', 3));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_PARITY_ITEM_NAME]->setText(parity->number < 0 ? tr("Normal") : tr("Mirrored"));
			m_wcs_labels[AGENT_PLATESOLVER_WCS_INDEX_ITEM_NAME]->setText(QString("%1°").arg(index->number, 0, 'g', 3));
			set_file_label(m_solution_image, "%1", m_solving_image);
		}
	} else if (property.state == INDIGO_ALERT_STATE && was_busy) {
		clear_solution();
		set_file_label(m_solution_image, tr("%1: not solved"), m_solving_image);
	}
	update_controls();
}

void PlateSolverWindow::update_use_index(const SolverProperty &property) {
	for (auto it = m_use_index_boxes.begin(); it != m_use_index_boxes.end();) {
		bool present = property.item(it.key().toUtf8().constData()) != nullptr;
		if (!present) {
			delete it.value();
			it = m_use_index_boxes.erase(it);
		} else {
			++it;
		}
	}
	for (const SolverItem &item : property.items) {
		QCheckBox *box = m_use_index_boxes.value(item.name, nullptr);
		if (box == nullptr) {
			box = new QCheckBox(item.label);
			m_use_index_layout->addWidget(box);
			m_use_index_boxes[item.name] = box;
			connect(box, &QCheckBox::clicked, this, &PlateSolverWindow::on_use_index_toggled);
		}
		QSignalBlocker blocker(box);
		box->setChecked(item.on);
	}
	m_use_index_empty->setVisible(m_use_index_boxes.isEmpty());
	update_controls();
}

void PlateSolverWindow::on_use_index_toggled() {
	QVector<SolverItem> items;
	for (auto it = m_use_index_boxes.constBegin(); it != m_use_index_boxes.constEnd(); ++it) {
		SolverItem item;
		item.name = it.key();
		item.number = 0;
		item.on = it.value()->isChecked();
		items.append(item);
	}
	m_client->set_switches(AGENT_PLATESOLVER_USE_INDEX_PROPERTY_NAME, items);
	update_controls();
}

bool PlateSolverWindow::any_use_index_selected() const {
	for (QCheckBox *box : m_use_index_boxes) {
		if (box->isChecked()) return true;
	}
	return false;
}

void PlateSolverWindow::update_index(const SolverProperty &property) {
	for (const SolverItem &item : property.items) {
		QCheckBox *box = m_index_boxes.value(item.name, nullptr);
		if (box == nullptr) {
			box = new QCheckBox(item.label);
			box->setObjectName(item.name);
			m_index_layout->addWidget(box);
			m_index_boxes[item.name] = box;
			QString name = item.name;
			connect(box, &QCheckBox::clicked, this, [this, name](bool checked) {
				on_index_toggled(name, checked);
			});
		}
		QSignalBlocker blocker(box);
		box->setChecked(item.on);
	}
	m_index_busy = property.state == INDIGO_BUSY_STATE;
	m_download_progress->setVisible(m_index_busy);
	m_download_abort->setVisible(m_index_busy);
	if (m_index_busy) {
		m_download_timer->start();
		on_download_timer();
	} else {
		m_download_timer->stop();
		m_download_status->setText(property.state == INDIGO_ALERT_STATE ? tr("The last index change failed, see the log.") : QString());
	}
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
	QVector<SolverItem> items;
	for (auto it = m_index_boxes.constBegin(); it != m_index_boxes.constEnd(); ++it) {
		SolverItem item;
		item.name = it.key();
		item.number = 0;
		item.on = it.value()->isChecked();
		items.append(item);
	}
	m_client->set_switches(SOLVER_INDEX_PROPERTY_NAME, items);
}

void PlateSolverWindow::on_download_timer() {
	QFileInfoList parts = QDir(index_folder()).entryInfoList(QStringList() << "*.part", QDir::Files);
	if (parts.isEmpty()) {
		m_download_status->setText(tr("Updating indexes..."));
	} else {
		const QFileInfo &part = parts.first();
		m_download_status->setText(tr("Downloading %1: %2 MB").arg(part.completeBaseName()).arg(part.size() / (1024 * 1024)));
	}
}

void PlateSolverWindow::on_image_changed() {
	bool has_image = m_preview_image != nullptr && m_image_data != nullptr && m_image_size > 0;
	m_current_image = has_image ? QString(m_image_path) : QString();
	if (has_image) {
		set_file_label(m_image_label, tr("Image: %1"), m_current_image);
	} else {
		m_image_label->setText(tr("Open an image to solve it."));
		m_image_label->setToolTip(QString());
	}
	if (!m_wcs_busy && !m_solve_requested) {
		clear_solution();
	}
	update_controls();
}

void PlateSolverWindow::on_solve_clicked() {
	if (!m_agent_loaded || m_current_image.isEmpty()) return;
	on_hints_edited();
	m_solving_image = m_current_image;
	m_solve_requested = true;
	update_controls();
	log_message(tr("Solving %1").arg(QFileInfo(m_solving_image).fileName()));
	if (!m_client->solve_image(m_image_data, m_image_size)) {
		m_solve_requested = false;
		log_message(tr("Failed to send the image to %1").arg(PLATESOLVER_DEVICE_NAME));
		update_controls();
		return;
	}
	// the agent does not update the WCS property if it drops the image before it starts solving
	QTimer::singleShot(SOLVE_START_TIMEOUT_MS, this, [this]() {
		if (m_solve_requested && !m_wcs_busy) {
			m_solve_requested = false;
			update_controls();
		}
	});
}

void PlateSolverWindow::on_abort_clicked() {
	if (!m_agent_loaded) return;
	m_client->abort();
	if (!m_wcs_busy) {
		m_solve_requested = false;
		update_controls();
	}
}

void PlateSolverWindow::update_controls() {
	bool solving = m_solve_requested || m_wcs_busy;
	bool can_solve = m_agent_loaded && !solving && !m_index_busy && !m_current_image.isEmpty() && any_use_index_selected();
	m_solve_button->setEnabled(can_solve);
	m_abort_button->setEnabled(m_agent_loaded && solving);
	m_tabs->setEnabled(m_agent_loaded);
	// the agent refuses index changes during a solve
	for (QCheckBox *box : m_index_boxes) {
		box->setEnabled(!solving);
	}
	if (!m_agent_loaded) {
		m_solve_button->setToolTip(QString());
	} else if (m_index_busy) {
		m_solve_button->setToolTip(tr("Index download in progress"));
	} else if (!any_use_index_selected()) {
		m_solve_button->setToolTip(tr("Select at least one index to use in the Indexes tab"));
	} else {
		m_solve_button->setToolTip(QString());
	}
}

void PlateSolverWindow::on_message(const QString &message) {
	log_message(message);
}

void PlateSolverWindow::log_message(const QString &message) {
	m_log->appendPlainText(QDateTime::currentDateTime().toString("HH:mm:ss ") + message);
}
