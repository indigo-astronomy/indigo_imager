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

#ifndef _SEQUENCE_MODEL_H
#define _SEQUENCE_MODEL_H

#include <QtGui>
#include <QtWidgets>

class Batch {
	QString m_name, m_mode, m_count, m_filter, m_frame;
public:
	Batch(
		const QString & name,
		const QString & mode,
		const QString & count,
		const QString & filter,
		const QString & frame
	) :
		m_name{name},
		m_mode{mode},
		m_count{count},
		m_filter{filter},
		m_frame{frame}
		{}
	QString name() const { return m_name; }
	QString mode() const { return m_mode; }
	QString count() const { return m_count; }
	QString filter() const { return m_filter; }
	QString frame() const { return m_frame; }
};

class SequenceModel : public QAbstractTableModel {
	QList<Batch> m_data;
public:
	SequenceModel(QObject * parent = {}) : QAbstractTableModel{parent} {}
	int rowCount(const QModelIndex &) const override { return m_data.count(); }
	int columnCount(const QModelIndex &) const override { return 5; }
	QVariant data(const QModelIndex &index, int role) const override {
		if (role != Qt::DisplayRole && role != Qt::EditRole) return {};
		const auto &sequence = m_data[index.row()];
		switch (index.column()) {
			case 0: return sequence.name();
			case 1: return sequence.mode();
			case 2: return sequence.count();
			case 3: return sequence.filter();
			case 4: return sequence.frame();
			default: return {};
		};
	}

	QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
		if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
		switch (section) {
			case 0: return "Name";
			case 1: return "Mode";
			case 2: return "Count";
			case 3: return "Filter";
			case 4: return "Frame";
			default: return {};
		}
	}

	void append(const Batch & batch) {
		beginInsertRows({}, m_data.count(), m_data.count());
		m_data.append(batch);
		endInsertRows();
	}
};

class SequenceViewer : public QWidget {
   QGridLayout m_layout{this};
   QTableView m_view;
   SequenceModel m_model;
   //QSortFilterProxyModel m_proxy;
   QInputDialog m_dialog;
public:
   SequenceViewer() {
      m_layout.addWidget(&m_view, 0, 0, 1, 1);
      //m_layout.addWidget(&m_button, 1, 0, 1, 1);
      //connect(&m_button, SIGNAL(clicked()), &m_dialog, SLOT(open()));
      m_model.append({"M13", "800x600", "5", "Lum", "FITS"});
	  m_model.append({"M13", "800x600", "5", "R", "FITS"});
	  m_model.append({"M13", "800x600", "5", "G", "FITS"});
	  m_model.append({"M13", "800x600", "5", "B", "FITS"});
      //m_proxy.setSourceModel(&m_model);
      //m_proxy.setFilterKeyColumn(2);
      m_view.setModel(&m_model);
	  //m_dialog.setLabelText("Enter registration number fragment to filter on. Leave empty to clear filter.");
	  //m_dialog.setInputMode(QInputDialog::TextInput);
	  //connect(&m_dialog, SIGNAL(textValueSelected(QString)),
	  //&m_proxy, SLOT(setFilterFixedString(QString)));
   }
};

#endif /* _SEQUENCE_MODEL_H */
