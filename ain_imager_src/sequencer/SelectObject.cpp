// Copyright (c) 2025 Rumen G.Bogdanovski
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

#include <QMessageBox>
#include "SelectObject.h"
#include "indigo_cat_data.h"

SelectObjectWidget::SelectObjectWidget(QWidget *parent) : QFrame(parent) {
	m_layout = new QGridLayout();
	m_layout->setAlignment(Qt::AlignTop);
	setLayout(m_layout);
	setFrameShape(QFrame::StyledPanel);
	setContentsMargins(0, 0, 0, 0);

	int row = 0;
	QLabel *label = new QLabel("Search: ");
	m_layout->addWidget(label, row, 0);

	m_object_search_line = new QLineEdit();
	m_object_search_line->setPlaceholderText("E.g. M42, Ain, Vega ...");
	m_layout->addWidget(m_object_search_line, row, 1, 1, 4);
	connect(m_object_search_line, &QLineEdit::textEdited, this, &SelectObjectWidget::onSearchTextChanged);
	connect(m_object_search_line, &QLineEdit::returnPressed, this, &SelectObjectWidget::onSearchEntered);

	row++;
	m_custom_objects_only_cbox = new QCheckBox("Custom objects only");
	m_layout->addWidget(m_custom_objects_only_cbox, row, 1, 1, 4, Qt::AlignRight);
	connect(m_custom_objects_only_cbox, &QCheckBox::clicked, this, &SelectObjectWidget::onCustomObjectsOnlyChecked);

	row++;
	m_object_list = new QListWidget();
	m_object_list->setStyleSheet("QListWidget {border: 1px solid #404040;}");
	m_layout->addWidget(m_object_list, row, 0, 1, 5);
	connect(m_object_list, &QListWidget::itemSelectionChanged, this, &SelectObjectWidget::onObjectSelected);
	connect(m_object_list, &QListWidget::itemClicked, this, &SelectObjectWidget::onObjectClicked);

	m_customObjectModel = new CustomObjectModel();
	m_customObjectModel->loadObjects();
}

void SelectObjectWidget::onSearchTextChanged(const QString &text) {
	updateObjectList(text);
}

void SelectObjectWidget::onSearchEntered() {
	if (m_object_list->count() == 0) return;
	m_object_list->setCurrentRow(0);
	m_object_list->setFocus();
	indigo_debug("%s -> 0\n", __FUNCTION__);
}

void SelectObjectWidget::onCustomObjectsOnlyChecked(bool checked) {
	Q_UNUSED(checked);
	updateObjectList(m_object_search_line->text());
}

void SelectObjectWidget::onObjectSelected() {
	onObjectClicked(m_object_list->currentItem());
}

void SelectObjectWidget::onObjectClicked(QListWidgetItem *item) {
	auto object = item->data(Qt::UserRole).value<CustomObject*>();
	if (object) {
		emit objectSelected(object->m_name, object->m_ra, object->m_dec);
	} else {
		auto dso = item->data(Qt::UserRole).value<indigo_dso_entry*>();
		if (dso) {
			emit objectSelected(dso->id, dso->ra, dso->dec);
		} else {
			auto star = item->data(Qt::UserRole).value<indigo_star_entry*>();
			if (star) {
				emit objectSelected("HIP" + QString::number(star->hip), star->ra, star->dec);
			}
		}
	}
}

void SelectObjectWidget::updateObjectList(const QString &text) {
	m_object_list->clear();
	QString obj_name_c = text.trimmed();
	if (obj_name_c.isEmpty()) return;

	char tooltip_c[INDIGO_VALUE_SIZE];

	auto objects = m_customObjectModel->m_objects;
	for (auto i = objects.constBegin(); i != objects.constEnd(); ++i) {
		auto object = *i;
		if (object->matchObject(obj_name_c)) {
			QListWidgetItem *item = new QListWidgetItem(object->m_name);
			snprintf(
				tooltip_c,
				INDIGO_VALUE_SIZE,
				"<b>%s</b> (Custom DB object)<p>α: %s<br>δ: %s<br>Apparent magnitude: %.1f<sup>m</sup><br><nobr>Description: %s</nobr></p>\n",
				object->m_name.toUtf8().constData(),
				indigo_dtos(object->m_ra, "%d:%02d:%04.1f"),
				indigo_dtos(object->m_dec, "+%d:%02d:%04.1f"),
				object->m_mag,
				object->m_description.toUtf8().constData()
			);
			item->setToolTip(tooltip_c);
			item->setData(Qt::UserRole, QVariant::fromValue(object));
			m_object_list->addItem(item);
		}
	}

	if (m_custom_objects_only_cbox->isChecked()) return;

	indigo_dso_entry *dso = &indigo_dso_data[0];
	while (dso->id) {
		if (
			QString(dso->id).contains(obj_name_c, Qt::CaseInsensitive) ||
			QString(dso->name).contains(obj_name_c, Qt::CaseInsensitive)
		) {
			QString data = QString(dso->id);
			QString name = dso->name[0] == '\0' ? QString(dso->id) : QString(dso->id) + ", " + dso->name;
			QListWidgetItem *item = new QListWidgetItem(name);
			snprintf(
				tooltip_c,
				INDIGO_VALUE_SIZE,
				"<b>%s</b> (%s)<p>α: %s<br>δ: %s<br>Apparent size: %.1f' x %.1f'<br>Apparent magnitude: %.1f<sup>m</sup><br><nobr>Names: %s</nobr></p>\n",
				dso->id,
				indigo_dso_type_description[(int)dso->type],
				indigo_dtos(dso->ra, "%d:%02d:%04.1f"),
				indigo_dtos(dso->dec, "+%d:%02d:%04.1f"),
				dso->r1, dso->r2,
				dso->mag,
				dso->name
			);
			item->setToolTip(tooltip_c);
			item->setData(Qt::UserRole, QVariant::fromValue(dso));
			m_object_list->addItem(item);
		}
		dso++;
	}

	indigo_star_entry *star = &indigo_star_data[0];
	QRegularExpression hip_re(QRegularExpression::anchoredPattern("HIP\\d+"), QRegularExpression::CaseInsensitiveOption);
	while (star->hip) {
		QString star_name = "HIP" + QString::number(star->hip);
		if (
			(star->name && QString(star->name).contains(obj_name_c, Qt::CaseInsensitive)) ||
			(hip_re.match(obj_name_c).hasMatch() && star_name.contains(obj_name_c, Qt::CaseInsensitive))
		) {
			QString data = star_name;
			QString name = star->name == nullptr || star->name[0] == '\0' ? star_name : star_name + ", " + star->name;
			QListWidgetItem *item = new QListWidgetItem(name);
			snprintf(
				tooltip_c,
				INDIGO_VALUE_SIZE,
				"<b>HIP%d</b> (Star)<p>α: %s<br>δ: %s<br>Apparent magnitude: %.1f<sup>m</sup><br><nobr>Names: %s</nobr></p>\n",
				star->hip,
				indigo_dtos(star->ra, "%d:%02d:%04.1f"),
				indigo_dtos(star->dec, "+%d:%02d:%04.1f"),
				star->mag,
				star->name ? star->name : ""
			);
			item->setToolTip(tooltip_c);
			item->setData(Qt::UserRole, QVariant::fromValue(star));
			m_object_list->addItem(item);
		}
		star++;
	}
}

void SelectObjectWidget::showEvent(QShowEvent *event) {
	QFrame::showEvent(event);
	m_object_search_line->setFocus();
}