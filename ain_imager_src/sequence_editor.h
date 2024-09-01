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

#ifndef _SEQUENCE_EDITOR_H
#define _SEQUENCE_EDITOR_H

#include <QtGui>
#include <QtWidgets>
#include <QTime>

class Batch {
	bool m_empty;
	QString m_filter, m_exposure, m_delay, m_count, m_mode, m_frame, m_focus;
public:
	Batch():
		m_empty{true},
		m_filter{""},
		m_exposure{""},
		m_delay{""},
		m_count{""},
		m_mode{""},
		m_frame{""},
		m_focus{""}
	{}

	Batch(
		const QString & filter,
		const QString & exposure,
		const QString & delay,
		const QString & count,
		const QString & mode,
		const QString & frame,
		const QString & focus
	):
		m_empty{false},
		m_filter{filter},
		m_exposure{exposure},
		m_delay{delay},
		m_count{count},
		m_mode{mode},
		m_frame{frame},
		m_focus{focus}
	{}

	Batch (QString batch_string):
		m_empty{true},
		m_filter{"*"},
		m_exposure{"1"},
		m_delay{"0"},
		m_count{"1"},
		m_mode{"*"},
		m_frame{"*"},
		m_focus{"*"}
	{
		QStringList list = batch_string.split(';', Qt::SkipEmptyParts);
		for(int i = 0; i < list.length(); i++) {
			QStringList key_val = list[i].split('=', Qt::SkipEmptyParts);
			if (key_val.length() <= 2) {
				QString key = key_val[0].trimmed();
				if (!key.compare("filter")) {
					m_filter = key_val[1].trimmed();
					m_empty = false;
				}
				if (!key.compare("exposure")) {
					m_exposure = key_val[1].trimmed();
					m_empty = false;
				}
				if (!key.compare("delay")) {
					m_delay = key_val[1].trimmed();
					m_empty = false;
				}
				if (!key.compare("count")) {
					m_count = key_val[1].trimmed();
					m_empty = false;
				}
				if (!key.compare("mode")) {
					m_mode = key_val[1].trimmed();
					m_empty = false;
				}
				if (!key.compare("frame")) {
					m_frame = key_val[1].trimmed();
					m_empty = false;

				}
				if (!key.compare("focus")) {
					m_focus = key_val[1].trimmed();
					m_empty = false;
				}
			}
		}
		//printf("m_filter= %s\nm_exposure = %s\nm_delay = %s\nm_count = %s\nm_mode = %s\nm_frame = %s\nm_focus = %s\n",
		//m_filter.toStdString().c_str(), m_exposure.toStdString().c_str(), m_delay.toStdString().c_str(),
		//m_count.toStdString().c_str(), m_mode.toStdString().c_str(), m_frame.toStdString().c_str(), m_focus.toStdString().c_str());
	}

	QString filter() const { return m_filter; }
	QString exposure() const { return m_exposure; }
	QString delay() const { return m_delay; }
	QString count() const { return m_count; }
	QString mode() const { return m_mode; }
	QString frame() const { return m_frame; }
	QString focus() const { return m_focus; }

	double approximate_duration() const {
		return (m_exposure.toDouble() + m_delay.toDouble()) * m_count.toDouble() + 10 * m_focus.toDouble();
	}

	QString to_property_value() const {
		QString batch_str;
		if (!m_filter.isEmpty() && m_filter != "*") {
			batch_str.append("filter=" + m_filter + ";");
		}
		if (!m_exposure.isEmpty() && m_exposure != "*") {
			batch_str.append("exposure=" + m_exposure + ";");
		}
		if (!m_delay.isEmpty() && m_delay != "*") {
			batch_str.append("delay=" + m_delay + ";");
		}
		if (!m_count.isEmpty() && m_count != "*") {
			batch_str.append("count=" + m_count + ";");
		}
		if (!m_mode.isEmpty() && m_mode != "*") {
			batch_str.append("mode=" + m_mode + ";");
		}
		if (!m_frame.isEmpty() && m_frame != "*") {
			batch_str.append("frame=" + m_frame + ";");
		}
		if (!m_focus.isEmpty() && m_focus != "*" && m_focus.toDouble() > 0) {
			batch_str.append("focus=" + m_focus + ";");
		}
		return batch_str;
	};

	bool is_empty() const {
		return m_empty;
	}

	void set_filter(QString s) {
		m_empty = false;
		m_filter = s;
	}
	void set_exposure(QString s) {
		m_empty = false;
		m_exposure = s;
	}
	void set_delay(QString s) {
		m_empty = false;
		m_delay = s;
	}
	void set_count(QString s) {
		m_empty = false;
		m_count = s;
	}
	void set_mode(QString s) {
		m_empty = false;
		m_mode = s;
	}
	void set_frame(QString s) {
		m_empty = false;
		m_frame = s;
	}
	void set_focus(QString s) {
		m_empty = false;
		m_focus = s;
	}
};

/*
| focus=XXX | execute autofocus routine with XXX seconds exposure time |
| count=XXX | set AGENT_IMAGER_BATCH.COUNT to XXX |
| exposure=XXX | set AGENT_IMAGER_BATCH.EXPOSURE to XXX |
| delay=XXX | set AGENT_IMAGER_BATCH.DELAY to XXX |
| filter=XXX | set WHEEL_SLOT.SLOT to the value corresponding to name XXX |
| mode=XXX | find and select CCD_MODE item with label XXX |
| name=XXX | set CCD_LOCAL_MODE.PREFIX to XXX |
| gain=XXX | set CCD_GAIN.GAIN to XXX |
| offset=XXX | set CCD_OFFSET.OFFSET to XXX |
| gamma=XXX | set CCD_GAMMA.GAMMA to XXX |
| temperature=XXX | set CCD_TEMPERATURE.TEMPERATURE to XXX |
| cooler=XXX | find and select CCD_COOLER item with label XXX |
| frame=XXX | find and select CCD_FRAME_TYPE item with label XXX |
| aperture=XXX | find and select DSLR_APERTURE item with label XXX |
| shutter=XXX | find and select DSLR_SHUTTER item with label XXX |
| iso=XXX | find and select DSLR_ISO item with label XXX |
*/

class SequenceModel : public QAbstractTableModel {
	QList<Batch> m_data;
	int current_section;
public:
	SequenceModel(QObject * parent = {}) : QAbstractTableModel{parent}, current_section(0) {}

	int rowCount(const QModelIndex &) const override {
		return m_data.count();
	}

	int columnCount(const QModelIndex &) const override {
		return 7;
	}

	int currentSection() {
		return current_section;
	}

	void setCurrentSection(int section) {
		current_section = section;
	}

	QVariant data(const QModelIndex &index, int role) const override {
		if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) return {};
		const auto &batch = m_data[index.row()];
		if (batch.is_empty()) {
			return {};
		}
		switch (index.column()) {
			case 0: return batch.filter();
			case 1: return batch.exposure();
			case 2: return batch.delay();
			case 3: return batch.count();
			case 4: return batch.mode();
			case 5: return batch.frame();
			case 6: return batch.focus();
			default: return {};
		};
		return {};
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
		if ((orientation != Qt::Horizontal && orientation != Qt::Vertical) || (role != Qt::DisplayRole && role != Qt::ToolTipRole)) return {};
		if (orientation == Qt::Horizontal && role != Qt::ToolTipRole) {
			switch (section) {
				case 0: return "Filter";
				case 1: return "Exposure";
				case 2: return "Delay";
				case 3: return "Count";
				case 4: return "Mode";
				case 5: return "Frame";
				case 6: return "Focus";
				default: return {};
			}
		}
		if (orientation == Qt::Vertical) {
			if (role == Qt::ToolTipRole) {
				const auto &batch = m_data[section];
				if (batch.is_empty()) {
					return {};
				}
				double duration = batch.approximate_duration();
				QTime time = QTime(0, 0).addSecs(duration);
				QString duration_string = time.toString("hh:mm:ss");
				return QString("Batch %1 approximate duration: %2").arg(QString::number(section + 1), duration_string);
			} else {
				if (section + 1 == current_section) {
					return  QString().number(section + 1) + " 🠶";
					//return  QString().number(section + 1) + " ▶";
				} else {
					return  QString().number(section + 1) + "   ";
				}
			}
		}
		return {};
	}

	void append(const Batch & batch) {
		beginInsertRows({}, m_data.count(), m_data.count());
		m_data.append(batch);
		endInsertRows();
	}

	void remove(int index) {
		if (index < 0) return;
		beginRemoveRows({}, index, index);
		m_data.removeAt(index);
		endRemoveRows();
	}

	void clear() {
		int index = m_data.count() - 1;
		beginRemoveRows({}, 0, index);
		while (index >= 0) {
			m_data.removeAt(index);
			index--;
		}
		endRemoveRows();
	}

	void set_batch(const Batch & batch, int pos = -1) {
		if (pos < 0) {
			beginInsertRows({}, m_data.count(), m_data.count());
			m_data.append(batch);
			endInsertRows();
		} else {
			m_data.replace(pos, batch);
		}
	}
	Batch get_batch(int pos) {
		return m_data.value(pos);
	}
};

class SequenceEditor : public QWidget {
	Q_OBJECT

public:
	SequenceEditor();
	~SequenceEditor();
	void generate_sequence(QString &sequence, QList<QString> &batches);
	double approximate_duration();
	QString get_sequence_name() {
		return m_name_edit->text().trimmed();
	}

private:
	QGridLayout m_layout{this};
	QTableView m_view;
	SequenceModel m_model;

	QLineEdit *m_name_edit;
	QSpinBox *m_repeat_box;
	QCheckBox *m_cooler_off_cbox;
	QCheckBox *m_park_cbox;

	QComboBox *m_filter_select;
	QDoubleSpinBox *m_exposure_box;
	QDoubleSpinBox *m_delay_box;
	QSpinBox *m_count_box;
	QComboBox *m_mode_select;
	QComboBox *m_frame_select;
	QDoubleSpinBox *m_focus_exp_box;
	QToolButton *m_add_button;
	QToolButton *m_rm_button;
	QToolButton *m_up_button;
	QToolButton *m_down_button;
	QToolButton *m_update_button;
	QToolButton *m_download_sequence_button;
	QToolButton *m_load_sequence_button;
	QToolButton *m_save_sequence_button;

	void populate_sequence_settings(QString &sequence_line);
	void populate_combobox(QComboBox *combobox, const char *items[255], const int count);
	void populate_combobox(QComboBox *combobox, QList<QString> &items);
	void clear_combobox(QComboBox *combobox);
	bool save_sequence(QString filename);
	bool load_sequence(QString filename);
	void show_message(const char *title, const char *message, QMessageBox::Icon icon = QMessageBox::Warning);

signals:
	void request_sequence();
	void set_sequence(QString name, QString sequence, QList<QString> batches);
	void populate_filter_select(QList<QString> &items);
	void populate_mode_select(QList<QString> &items);
	void populate_frame_select(QList<QString> &items);
	void clear_filter_select();
	void clear_mode_select();
	void clear_frame_select();
	void sequence_updated();
	void set_sequence_name(QString name);
	void sequence_name_set(const QString &name);
	void clear_selection();
	void set_current_batch(int batch_index);

public slots:
	void on_add_sequence();
	void on_update_sequence();
	void on_remove_sequence();
	void on_move_up_sequence();
	void on_move_down_sequence();
	void on_row_changed(const QModelIndex &current, const QModelIndex &previous);
	void on_double_click(const QModelIndex& idx);
	void on_park_cooler_clicked(bool state);
	void on_repeat_changed(int value);
	void on_save_sequence();
	void on_load_sequence();
	void on_set_sequence(QString name, QString sequence, QList<QString> batches);

	void on_set_sequence_name(QString name) {
		m_name_edit->setText(name);
	}

	void on_populate_filter_select(QList<QString> &items) {
		populate_combobox(m_filter_select, items);
	}

	void on_populate_mode_select(QList<QString> &items) {
		populate_combobox(m_mode_select, items);
	}

	void on_populate_frame_select(QList<QString> &items) {
		populate_combobox(m_frame_select, items);
	}

	void on_clear_filter_select() {
		clear_combobox(m_filter_select);
	}

	void on_clear_mode_select() {
		clear_combobox(m_mode_select);
	}

	void on_clear_frame_select() {
		clear_combobox(m_frame_select);
	}

	void on_set_current_batch(int batch_index) {
		m_model.setCurrentSection(batch_index);
		m_view.repaint();
	}
};

#endif /* _SEQUENCE_EDITOR_H */
