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

#include <sequence_model.h>
#include <indigo/indigo_bus.h>

SequenceViewer::SequenceViewer() {
	int row = 0;
	int col = 0;
	m_layout.addWidget(&m_view, row, col, 1, 8);
	m_view.setSelectionBehavior(QAbstractItemView::SelectRows);
	m_view.setSelectionMode(QAbstractItemView::SingleSelection);
	m_view.setModel(&m_model);
	//m_view.setSelectionMode(QAbstractItemView::ContiguousSelection);

	//m_layout.addWidget(&m_button, 1, 0, 1, 1);
	//connect(&m_button, SIGNAL(clicked()), &m_dialog, SLOT(open()));

	//m_model.set_batch({"M13", "800x600", "5", "Lum", "FITS","","",""});
	//m_model.set_batch({"M13", "800x600", "5", "R", "FITS","","",""});
	//m_model.append({"M13", "800x600", "5", "G", "FITS","","",""});
	//m_model.append({"M13", "800x600", "5", "B", "FITS","","",""});

	//m_model.set_batch({"M14", "800x600", "5", "R", "FITS","","",""}, 1);
	//m_proxy.setSourceModel(&m_model);
	//m_proxy.setFilterKeyColumn(2);
	//
	//m_model.set_batch({"M15", "800x600", "5", "R", "FITS","","",""}, 0);

	//Batch b = m_model.get_batch(1);
	//b.set_frame("XXXXXXXX");
	//m_model.set_batch(b);

	row++;
	col=0;
	m_name_edit = new QLineEdit();
	m_name_edit->setPlaceholderText("Object name");
	m_layout.addWidget(m_name_edit, row, col, 1, 2);

	col += 2;
	m_filter_select = new QComboBox();
	m_layout.addWidget(m_filter_select, row, col, 1, 2);

	col += 2;
	m_mode_select = new QComboBox();
	m_layout.addWidget(m_mode_select, row, col, 1, 2);

	col+=2;
	m_frame_select = new QComboBox();
	m_layout.addWidget(m_frame_select, row, col, 1, 2);

	row++;
	col = 0;
	QLabel *label = new QLabel("Exposure (s):");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	m_layout.addWidget(label, row, col);

	col++;
	m_exposure_box = new QDoubleSpinBox();
	m_exposure_box->setMaximum(10000);
	m_exposure_box->setMinimum(0);
	m_exposure_box->setValue(1);
	m_exposure_box->setKeyboardTracking(false);
	m_layout.addWidget(m_exposure_box, row, col);

	col++;
	label = new QLabel("Delay (s):");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	m_layout.addWidget(label, row, col);

	col++;
	m_delay_box = new QDoubleSpinBox();
	m_delay_box->setMaximum(10000);
	m_delay_box->setMinimum(0);
	m_delay_box->setValue(1);
	m_delay_box->setKeyboardTracking(false);
	m_layout.addWidget(m_delay_box, row, col);

	col++;
	label = new QLabel("Count:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	m_layout.addWidget(label, row, col);

	col++;
	m_count_box = new QSpinBox();
	m_count_box->setMaximum(10000);
	m_count_box->setMinimum(1);
	m_count_box->setValue(1);
	m_count_box->setKeyboardTracking(false);
	m_layout.addWidget(m_count_box, row, col);

	col++;
	label = new QLabel("Focus (s):");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	m_layout.addWidget(label, row, col);

	col++;
	m_focus_exp_box = new QDoubleSpinBox();
	m_focus_exp_box->setMaximum(10000);
	m_focus_exp_box ->setMinimum(0);
	m_focus_exp_box ->setValue(0);
	m_focus_exp_box ->setKeyboardTracking(false);
	m_layout.addWidget(m_focus_exp_box, row, col);

	row++;
	col = 0;
	m_add_button = new QPushButton("+");
	m_add_button->setStyleSheet("min-width: 30px");
	///m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	m_layout.addWidget(m_add_button, row, col);
	connect(m_add_button, &QPushButton::clicked, this, &SequenceViewer::on_add_sequence);

	col++;
	m_rm_button = new QPushButton("-");
	m_rm_button->setStyleSheet("min-width: 30px");
	///m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	m_layout.addWidget(m_rm_button, row, col);
	connect(m_rm_button, &QPushButton::clicked, this, &SequenceViewer::on_remove_sequence);

	col++;
	m_up_button = new QPushButton("Up");
	m_up_button->setStyleSheet("min-width: 30px");
	///m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	m_layout.addWidget(m_up_button, row, col);
	connect(m_up_button, &QPushButton::clicked, this, &SequenceViewer::on_move_up_sequence);

	col++;
	m_down_button = new QPushButton("Down");
	m_down_button->setStyleSheet("min-width: 30px");
	///m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	m_layout.addWidget(m_down_button, row, col);
	connect(m_down_button, &QPushButton::clicked, this, &SequenceViewer::on_move_down_sequence);

	col++;
	m_update_button = new QPushButton("Update");
	m_update_button->setStyleSheet("min-width: 30px");
	///m_focusing_preview_button->setIcon(QIcon(":resource/play.png"));
	m_layout.addWidget(m_update_button, row, col);
	connect(m_update_button, &QPushButton::clicked, this, &SequenceViewer::on_update_sequence);

	connect(this, &SequenceViewer::populate_filter_select, this, &SequenceViewer::on_populate_filter_select);
	connect(this, &SequenceViewer::populate_mode_select, this, &SequenceViewer::on_populate_mode_select);
	connect(this, &SequenceViewer::populate_frame_select, this, &SequenceViewer::on_populate_frame_select);
	connect(this, &SequenceViewer::clear_filter_select, this, &SequenceViewer::on_clear_filter_select);
	connect(this, &SequenceViewer::clear_mode_select, this, &SequenceViewer::on_clear_mode_select);
	connect(this, &SequenceViewer::clear_frame_select, this, &SequenceViewer::on_clear_frame_select);

	QItemSelectionModel *selection_model = m_view.selectionModel();
	connect(selection_model, &QItemSelectionModel::currentRowChanged, this, &SequenceViewer::on_row_changed);

	QList<QString> filters;
	filters.append("U");
	filters.append("B");
	filters.append("V");
	filters.append("R");
	filters.append("I");
	populate_filter_select(filters);

	clear_frame_select();

	QList<QString> modes;
	modes.append("800x600");
	modes.append("1280x1024");
	populate_combobox(m_mode_select, modes);
}

SequenceViewer::~SequenceViewer() {
}

void SequenceViewer::on_row_changed(const QModelIndex &current, const QModelIndex &previous) {
	Q_UNUSED(previous);

	int row = current.row();
	if(row < 0) return;

	Batch b = m_model.get_batch(row);

	m_name_edit->setText(b.name());

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

	// TESTCODE !!!!
	/*
	QList<QString> batches;
	QString s;
	generate_sequence(s, batches);
	indigo_error("[SEQUENCE] %s\n", s.toUtf8().data());
	QList<QString>::iterator item;
	for (item = batches.begin(); item != batches.end(); ++item) {
		indigo_error("[BATCH] %s\n", item->toUtf8().data());
	}
	*/
}

void SequenceViewer::on_move_up_sequence() {
	int row = m_view.currentIndex().row();
	if(row <= 0) return;

	Batch b1 = m_model.get_batch(row);
	Batch b2 = m_model.get_batch(row - 1);
	m_model.set_batch(b1, row - 1);
	m_model.set_batch(b2, row);

	QModelIndex index = m_view.model()->index(row - 1, 0);
	m_view.setCurrentIndex(index);
}

void SequenceViewer::on_move_down_sequence() {
	int row = m_view.currentIndex().row();
	if (row >= m_view.model()->rowCount() - 1) return;

	Batch b1 = m_model.get_batch(row);
	Batch b2 = m_model.get_batch(row + 1);
	m_model.set_batch(b1, row + 1);
	m_model.set_batch(b2, row);

	QModelIndex index = m_view.model()->index(row + 1, 0);
	m_view.setCurrentIndex(index);
}

void  SequenceViewer::on_remove_sequence() {
	QModelIndexList selection = m_view.selectionModel()->selectedRows();
	for(int i = 0; i < selection.count(); ++i) {
		QModelIndex index = selection.at(i);
		m_model.remove(index.row());
	}
}

void SequenceViewer::on_add_sequence() {
	Batch b;
	b.set_name(m_name_edit->text());
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
}

void SequenceViewer::on_update_sequence() {
	Batch b;
	int row = m_view.currentIndex().row();
	if (row < 0) return;

	b.set_name(m_name_edit->text());
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
}

void SequenceViewer::populate_combobox(QComboBox *combobox, const char *items[255], const int count) {
	if (combobox == nullptr) return;
	combobox->clear();
	combobox->addItem("* (no change)", "*");
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

void SequenceViewer::populate_combobox(QComboBox *combobox, QList<QString> &items) {
	if (combobox == nullptr) return;
	combobox->clear();
	combobox->addItem("* (no change)", "*");

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

void SequenceViewer::clear_combobox(QComboBox *combobox) {
	if (combobox == nullptr) return;
	combobox->clear();
	combobox->addItem("* (no change)", "*");
}

void SequenceViewer::generate_sequence(QString &sequence, QList<QString> &batches) {
	int row_count = m_view.model()->rowCount();
	batches.clear();
	sequence.clear();

	if (row_count <= 0) {
		return;
	}

	for (int row = 0; row < row_count; row++) {
		Batch b = m_model.get_batch(row);
		QString batch_str("");

		if (!b.name().isEmpty()) {
			batch_str.append("prefix=" + b.name() + ";");
		}

		if (!b.filter().isEmpty() && b.filter() != "*") {
			batch_str.append("filter=" + b.filter() + ";");
		}

		if (!b.exposure().isEmpty() && b.exposure() != "*") {
			batch_str.append("exposure=" + b.exposure() + ";");
		}

		if (!b.delay().isEmpty() && b.delay() != "*") {
			batch_str.append("delay=" + b.delay() + ";");
		}

		if (!b.count().isEmpty() && b.count() != "*") {
			batch_str.append("count=" + b.count() + ";");
		}

		if (!b.mode().isEmpty() && b.mode() != "*") {
			batch_str.append("mode=" + b.mode() + ";");
		}

		if (!b.frame().isEmpty() && b.frame() != "*") {
			batch_str.append("frame=" + b.frame() + ";");
		}

		if (!b.focus().isEmpty() && b.focus() != "*") {
			batch_str.append("focus=" + b.focus() + ";");
		}

		if (!batch_str.isEmpty()) {
			sequence.append(QString().number(row) + ";");
			batches.append(batch_str);
		}
	}
}
