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

#include <sequence_editor.h>
#include <indigo/indigo_bus.h>

#if defined(INDIGO_WINDOWS)
#define PATH_LEN 4096
#else
#define PATH_LEN PATH_MAX
#endif

SequenceEditor::SequenceEditor() {
	//Batch b("name=alaba nica;exposure=4;delay=3;count=3;focus=2;");
	int row = 0;
	int col = 0;
	m_layout.addWidget(&m_view, row, col, 1, 8);
	m_layout.setColumnStretch(0, 1);
	m_layout.setColumnStretch(1, 1);
	m_layout.setColumnStretch(2, 1);
	m_layout.setColumnStretch(3, 1);
	m_layout.setColumnStretch(4, 1);
	m_layout.setColumnStretch(5, 1);
	m_layout.setColumnStretch(6, 1);
	m_layout.setColumnStretch(7, 1);
	m_view.setSelectionBehavior(QAbstractItemView::SelectRows);
	m_view.setSelectionMode(QAbstractItemView::SingleSelection);
	m_view.horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_view.setModel(&m_model);

	row++;
	col=0;
	m_name_edit = new QLineEdit();
	m_name_edit->setPlaceholderText("File prefix e.g. M16");
	m_layout.addWidget(m_name_edit, row, col, 1, 2);

	col += 2;
	m_cooler_off_cbox = new QCheckBox("Turn cooler off");
	m_cooler_off_cbox->setToolTip("Turn camera cooler off when finished");
	m_cooler_off_cbox->setEnabled(true);
	m_cooler_off_cbox->setChecked(false);
	m_layout.addWidget(m_cooler_off_cbox, row, col, 1, 2);
	connect(m_cooler_off_cbox, &QPushButton::clicked, this, &SequenceEditor::on_park_cooler_clicked);

	col += 2;
	m_park_cbox = new QCheckBox("Park mount");
	m_park_cbox->setToolTip("Park mount when finished");
	m_park_cbox->setEnabled(true);
	m_park_cbox->setChecked(false);
	m_layout.addWidget(m_park_cbox, row, col, 1, 2);
	connect(m_park_cbox, &QPushButton::clicked, this, &SequenceEditor::on_park_cooler_clicked);

	col+=2;
	QLabel *label = new QLabel("Repeat:");
	label->setToolTip("Reperat sequence");
	m_layout.addWidget(label, row, col);
	col++;
	m_repeat_box = new QSpinBox();
	m_repeat_box->setMaximum(10);
	m_repeat_box->setMinimum(1);
	m_repeat_box->setMinimumWidth(50);
	m_repeat_box->setValue(1);
	m_layout.addWidget(m_repeat_box, row, col);
	connect(m_repeat_box, QOverload<int>::of(&QSpinBox::valueChanged), this, &SequenceEditor::on_repeat_changed);

	row++;
	QSpacerItem *spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	m_layout.addItem(spacer, row, 0);

	row++;
	col = 0;
	label = new QLabel("Batch description:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	label->setToolTip("Batch description");
	m_layout.addWidget(label, row, col, 1, 2);

	col += 2;
	m_filter_select = new QComboBox();
	m_filter_select->setToolTip("Filter to be used for the batch.");
	m_layout.addWidget(m_filter_select, row, col, 1, 2);

	col += 2;
	m_mode_select = new QComboBox();
	m_mode_select->setToolTip("Frame mode: gray/RGB, bit depth, resolution etc.");
	m_layout.addWidget(m_mode_select, row, col, 1, 2);

	col+=2;
	m_frame_select = new QComboBox();
	m_frame_select->setToolTip("Frame type: Light, Dark, Flat frame etc.");
	m_layout.addWidget(m_frame_select, row, col, 1, 2);

	row++;
	col = 0;
	label = new QLabel("Exposure (s):");
	m_layout.addWidget(label, row, col);

	col++;
	m_exposure_box = new QDoubleSpinBox();
	m_exposure_box->setToolTip("Exposure time for each frame in the batch.");
	m_exposure_box->setMaximum(10000);
	m_exposure_box->setMinimum(0);
	m_exposure_box->setValue(1);
	m_layout.addWidget(m_exposure_box, row, col);

	col++;
	label = new QLabel("Delay (s):");
	m_layout.addWidget(label, row, col);

	col++;
	m_delay_box = new QDoubleSpinBox();
	m_delay_box->setToolTip("Delay between exposures in the batch.");
	m_delay_box->setMaximum(10000);
	m_delay_box->setMinimum(0);
	m_delay_box->setValue(0);
	m_layout.addWidget(m_delay_box, row, col);

	col++;
	label = new QLabel("Count:");
	m_layout.addWidget(label, row, col);

	col++;
	m_count_box = new QSpinBox();
	m_count_box->setToolTip("Number of frames in the batch.");
	m_count_box->setMaximum(10000);
	m_count_box->setMinimum(1);
	m_count_box->setValue(1);
	m_layout.addWidget(m_count_box, row, col);

	col++;
	label = new QLabel("Focus (s):");
	m_layout.addWidget(label, row, col);

	col++;
	m_focus_exp_box = new QDoubleSpinBox();
	m_focus_exp_box->setToolTip("Focusing exposure time (0 = skip). Focusing will be performed before starting the batch.");
	m_focus_exp_box->setMaximum(10000);
	m_focus_exp_box->setMinimum(0);
	m_focus_exp_box->setValue(0);
	m_focus_exp_box->setSpecialValueText("* (skip)");
	m_layout.addWidget(m_focus_exp_box, row, col);

	row++;
	spacer = new QSpacerItem(1, 10, QSizePolicy::Expanding, QSizePolicy::Maximum);
	m_layout.addItem(spacer, row, 0);

	row++;
	QWidget *toolbar = new QWidget;
	QHBoxLayout *toolbox = new QHBoxLayout(toolbar);
	toolbar->setContentsMargins(1,1,1,1);
	toolbox->setContentsMargins(1,1,1,1);
	m_layout.addWidget(toolbar, row, 0, 1, 8);

	m_add_button = new QToolButton(this);
	m_add_button->setIcon(QIcon(":resource/zoom-in.png"));
	m_add_button->setToolTip("Add batch to the sequence");
	toolbox->addWidget(m_add_button);
	connect(m_add_button, &QToolButton::clicked, this, &SequenceEditor::on_add_sequence);

	m_rm_button = new QToolButton(this);
	m_rm_button->setIcon(QIcon(":resource/zoom-out.png"));
	m_rm_button->setToolTip("Remove selected batch from the sequence");
	toolbox->addWidget(m_rm_button);
	connect(m_rm_button, &QToolButton::clicked, this, &SequenceEditor::on_remove_sequence);

	m_up_button = new QToolButton(this);
	m_up_button->setIcon(QIcon(":resource/arrow-up.png"));
	m_up_button->setToolTip("Move selected batch up");
	toolbox->addWidget(m_up_button);
	connect(m_up_button, &QToolButton::clicked, this, &SequenceEditor::on_move_up_sequence);

	m_down_button = new QToolButton(this);
	m_down_button->setIcon(QIcon(":resource/arrow-down.png"));
	m_down_button->setToolTip("Move selected batch down");
	toolbox->addWidget(m_down_button);
	connect(m_down_button, &QToolButton::clicked, this, &SequenceEditor::on_move_down_sequence);

	m_update_button = new QToolButton(this);
	m_update_button->setIcon(QIcon(":resource/edit.png"));
	m_update_button->setToolTip("Update selected batch");
	toolbox->addWidget(m_update_button);
	connect(m_update_button, &QToolButton::clicked, this, &SequenceEditor::on_update_sequence);

	toolbox->addStretch(1);

	m_download_sequence_button = new QToolButton(this);
	m_download_sequence_button->setIcon(QIcon(":resource/download.png"));
	m_download_sequence_button->setToolTip("Download the sequence from the Imager Agent");
	toolbox->addWidget(m_download_sequence_button);
	connect(m_download_sequence_button, &QToolButton::clicked, this, &SequenceEditor::on_update_sequence);

	m_load_sequence_button = new QToolButton(this);
	m_load_sequence_button->setIcon(QIcon(":resource/folder.png"));
	m_load_sequence_button->setToolTip("Load sequence from file");
	toolbox->addWidget(m_load_sequence_button);
	connect(m_load_sequence_button, &QToolButton::clicked, this, &SequenceEditor::on_load_sequence);

	m_save_sequence_button = new QToolButton(this);
	m_save_sequence_button->setIcon(QIcon(":resource/save.png"));
	m_save_sequence_button->setToolTip("Save sequence to file");
	toolbox->addWidget(m_save_sequence_button);
	connect(m_save_sequence_button, &QToolButton::clicked, this, &SequenceEditor::on_save_sequence);

	connect(this, &SequenceEditor::populate_filter_select, this, &SequenceEditor::on_populate_filter_select);
	connect(this, &SequenceEditor::populate_mode_select, this, &SequenceEditor::on_populate_mode_select);
	connect(this, &SequenceEditor::populate_frame_select, this, &SequenceEditor::on_populate_frame_select);
	connect(this, &SequenceEditor::clear_filter_select, this, &SequenceEditor::on_clear_filter_select);
	connect(this, &SequenceEditor::clear_mode_select, this, &SequenceEditor::on_clear_mode_select);
	connect(this, &SequenceEditor::clear_frame_select, this, &SequenceEditor::on_clear_frame_select);
	connect(this, &SequenceEditor::set_sequence_name, this, &SequenceEditor::on_set_sequence_name);
	connect(&m_view, &QTableView::doubleClicked, this, &SequenceEditor::on_double_click);

	QItemSelectionModel *selection_model = m_view.selectionModel();
	connect(selection_model, &QItemSelectionModel::currentRowChanged, this, &SequenceEditor::on_row_changed);
	connect(this, &SequenceEditor::clear_selection, selection_model, &QItemSelectionModel::clearSelection);

	clear_filter_select();
	clear_mode_select();
	clear_frame_select();
}

SequenceEditor::~SequenceEditor() {
}

void SequenceEditor::on_double_click(const QModelIndex& idx) {
	Q_UNUSED(idx);
	QItemSelectionModel *selection_model = m_view.selectionModel();
	selection_model->clearSelection();
	m_view.reset();
}

void SequenceEditor::on_park_cooler_clicked(bool state) {
	Q_UNUSED(state);
	emit(sequence_updated());
}

void SequenceEditor::on_repeat_changed(int value) {
	Q_UNUSED(value);
	emit(sequence_updated());
}

void SequenceEditor::on_row_changed(const QModelIndex &current, const QModelIndex &previous) {
	Q_UNUSED(previous);

	int row = current.row();
	if(row < 0) return;

	Batch b = m_model.get_batch(row);

	int index = m_filter_select->findData(b.filter());
	if ( index != -1 ) {
		m_filter_select->setCurrentIndex(index);
	}

	index = m_mode_select->findData(b.mode());
	if ( index != -1 ) {
		m_mode_select->setCurrentIndex(index);
	}

	index = m_frame_select->findData(b.frame());
	if ( index != -1 ) {
		m_frame_select->setCurrentIndex(index);
	}

	m_exposure_box->setValue(b.exposure().toFloat());
	m_delay_box->setValue(b.delay().toFloat());
	m_count_box->setValue(b.count().toInt());
	m_focus_exp_box->setValue(b.focus().toFloat());
}

void SequenceEditor::on_move_up_sequence() {
	QModelIndexList selection = m_view.selectionModel()->selectedRows();
	if (selection.count() != 1) return;
	int row = selection[0].row();

	if(row <= 0) return;

	Batch b1 = m_model.get_batch(row);
	Batch b2 = m_model.get_batch(row - 1);
	m_model.set_batch(b1, row - 1);
	m_model.set_batch(b2, row);

	QModelIndex index = m_view.model()->index(row - 1, 0);
	m_view.setCurrentIndex(index);
	emit(sequence_updated());
}

void SequenceEditor::on_move_down_sequence() {
	QModelIndexList selection = m_view.selectionModel()->selectedRows();
	if (selection.count() != 1) return;
	int row = selection[0].row();

	if (row >= m_view.model()->rowCount() - 1) return;

	Batch b1 = m_model.get_batch(row);
	Batch b2 = m_model.get_batch(row + 1);
	m_model.set_batch(b1, row + 1);
	m_model.set_batch(b2, row);

	QModelIndex index = m_view.model()->index(row + 1, 0);
	m_view.setCurrentIndex(index);
	emit(sequence_updated());
}

void  SequenceEditor::on_remove_sequence() {
	bool removed = false;
	QModelIndexList selection = m_view.selectionModel()->selectedRows();
	for(int i = 0; i < selection.count(); ++i) {
		QModelIndex index = selection.at(i);
		m_model.remove(index.row());
		removed = true;
	}
	if (removed) {
		emit(sequence_updated());
	}
}

void SequenceEditor::on_add_sequence() {
	Batch b;
	b.set_filter(m_filter_select->currentData().toString());
	b.set_mode(m_mode_select->currentData().toString());
	b.set_frame(m_frame_select->currentData().toString());
	b.set_exposure(QString::number(m_exposure_box->value()));
	b.set_delay(QString::number(m_delay_box->value()));
	b.set_count(QString::number(m_count_box->value()));

	QString focus("*");
	if (m_focus_exp_box->value() > 0) {
		focus = QString::number(m_focus_exp_box->value());
	}
	b.set_focus(focus);

	m_model.append(b);
	emit(sequence_updated());
}

void SequenceEditor::on_update_sequence() {
	Batch b;
	int row = m_view.currentIndex().row();
	if (row < 0) return;

	b.set_filter(m_filter_select->currentData().toString());
	b.set_mode(m_mode_select->currentData().toString());
	b.set_frame(m_frame_select->currentData().toString());
	b.set_exposure(QString::number(m_exposure_box->value()));
	b.set_delay(QString::number(m_delay_box->value()));
	b.set_count(QString::number(m_count_box->value()));

	QString focus("*");
	if (m_focus_exp_box->value() > 0) {
		focus = QString::number(m_focus_exp_box->value());
	}
	b.set_focus(focus);

	m_model.set_batch(b, row);
	m_view.update();
	emit(sequence_updated());
}

void SequenceEditor::on_save_sequence() {
	QString qlocation = QDir::toNativeSeparators(QDir::homePath());
	QString file_name = QFileDialog::getSaveFileName(this,
		tr("Save sequence"), qlocation,
		QString("Ain sequence (*.seq)"));

	if (file_name == "") return;

	if (!file_name.endsWith(".seq", Qt::CaseInsensitive)) file_name += ".seq";

	if (!save_sequence(file_name)) {
		char msg[PATH_LEN];
		snprintf(msg, PATH_LEN, "Error saving: '%s'\nMake sure you have write permissions and the sequence is not empty!", QDir::toNativeSeparators(file_name).toUtf8().data());
		show_message("Error!", msg);
	}
}

void SequenceEditor::on_load_sequence() {
	QString qlocation = QDir::toNativeSeparators(QDir::homePath());
	QString file_name = QFileDialog::getOpenFileName(this,
		tr("Load sequence"), qlocation,
		QString("Ain sequence (*.seq)"));

	if (file_name == "") return;

	if (!load_sequence(file_name)) {
		char msg[PATH_LEN];
		snprintf(msg, PATH_LEN, "Error opening: '%s'\nMake sure you have read permissions!", QDir::toNativeSeparators(file_name).toUtf8().data());
		show_message("Error!", msg);
	}
}

void SequenceEditor::populate_combobox(QComboBox *combobox, const char *items[255], const int count) {
	if (combobox == nullptr) return;
	clear_combobox(combobox);
	if (items == nullptr) return;

	for (int i = 0; i < count; i++) {
		QString item = QString(items[i]);
		if (combobox->findText(item) < 0) {
			combobox->addItem(item, item);
			indigo_debug("[ADD] %s\n", item.toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE] %s\n", item.toUtf8().data());
		}
	}
}

void SequenceEditor::populate_combobox(QComboBox *combobox, QList<QString> &items) {
	if (combobox == nullptr) return;
	clear_combobox(combobox);

	QList<QString>::iterator item;
	for (item = items.begin(); item != items.end(); ++item) {
		if (combobox->findText(*item) < 0) {
			combobox->addItem(*item, *item);
			indigo_debug("[ADD] %s\n", item->toUtf8().data());
		} else {
			indigo_debug("[DUPLICATE] %s\n", item->toUtf8().data());
		}
	}
}

void SequenceEditor::clear_combobox(QComboBox *combobox) {
	if (combobox == nullptr) return;
	combobox->clear();
	combobox->addItem("* (use current)", "*");
}

void SequenceEditor::generate_sequence(QString &sequence, QList<QString> &batches) {
	int row_count = m_view.model()->rowCount();
	batches.clear();
	sequence.clear();

	if (row_count <= 0) {
		return;
	}

	for (int row = 0; row < row_count; row++) {
		Batch b = m_model.get_batch(row);

		QString batch_str = b.to_property_value();
		if (!batch_str.isEmpty()) {
			sequence.append(QString().number(row+1) + ";");
			batches.append(batch_str);
		}
	}

	int repeat = m_repeat_box->value();
	QString seq_str;
	for (int i = 0; i < repeat; i++) {
		seq_str += sequence;
	}
	sequence = seq_str;

	if (m_cooler_off_cbox->checkState() == Qt::Checked) {
		sequence += "cooler=off;";
	}

	if (m_park_cbox->checkState() == Qt::Checked) {
		sequence += "park;";
	}
}

bool SequenceEditor::save_sequence(QString filename) {
	char file_name[PATH_MAX];
	QString sequence;
	QList<QString> batches;

	QString sequence_name = m_name_edit->text();
	generate_sequence(sequence, batches);

	if (batches.count() <= 0) {
		return false;
	}

	strcpy(file_name, filename.toUtf8().constData());
	FILE *fp = fopen(file_name, "w+");
	if (fp == nullptr) {
		return false;
	}
	fprintf(fp, "# Ain INDIGO Imager sequence file. DO NOT EDIT MANUALLY!\n");
	fprintf(fp, "%s\n", sequence_name.toStdString().c_str());
	fprintf(fp, "%s\n", sequence.toStdString().c_str());
	for (int i = 0; i < batches.count(); i++) {
		fprintf(fp, "%s\n", batches[i].toStdString().c_str());
	}
	fclose(fp);
	return true;
}

bool SequenceEditor::load_sequence(QString filename) {
	char file_name[PATH_MAX];
	char line[PATH_MAX];

	strcpy(file_name, filename.toUtf8().constData());
	FILE *fp = fopen(file_name, "r");
	if (fp == nullptr) {
		return false;
	}

	m_model.clear();
	m_view.reset();

	int ln = 0;
	while (fgets(line, PATH_MAX, fp)) {
		QString line_str = QString(line).trimmed();
		if (line_str.startsWith("#")) {
			continue;
		}
		if (ln == 0) {
			m_name_edit->setText(line_str);
		} else if (ln == 1) {
			if(line_str.contains("cooler=off;", Qt::CaseInsensitive)) {
				m_cooler_off_cbox->setCheckState(Qt::Checked);
			} else {
				m_cooler_off_cbox->setCheckState(Qt::Unchecked);
			}
			if(line_str.contains("park;", Qt::CaseInsensitive)) {
				m_park_cbox->setCheckState(Qt::Checked);
			} else {
				m_park_cbox->setCheckState(Qt::Unchecked);
			}
			int repeat = line_str.split(";1;").length();
			m_repeat_box->blockSignals(true);
			m_repeat_box->setValue(repeat);
			m_repeat_box->blockSignals(false);
		} else {
			Batch b(line_str);
			if (!b.is_empty()) {
				m_model.append(b);
			}
		}
		ln++;
    }
	fclose(fp);
	emit(sequence_updated());
	return true;
}

double SequenceEditor::approximate_duration() {
	int row_count = m_view.model()->rowCount();
	double duration = 0;

	if (row_count <= 0) {
		return duration;
	}

	for (int row = 0; row < row_count; row++) {
		Batch b = m_model.get_batch(row);
		duration += (b.exposure().toDouble() + b.delay().toDouble()) * b.count().toDouble() + 10 * b.focus().toDouble();
	}

	int repeat = m_repeat_box->value();
	if (repeat >= 0) {
		duration *= repeat;
	} else {
		duration = 0;
	}
	return duration / 3600.0;
}

void SequenceEditor::show_message(const char *title, const char *message, QMessageBox::Icon icon) {
	QMessageBox msgBox(this);
	indigo_error(message);
	msgBox.setWindowTitle(title);
	msgBox.setIcon(icon);
	msgBox.setText(message);
	msgBox.exec();
}
