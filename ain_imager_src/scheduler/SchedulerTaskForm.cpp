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

#include "SchedulerTaskForm.h"
#include "SchedulerTask.h"
#include "QLineEditSG.h"
#include "SexagesimalConverter.h"
#include "SelectObject.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QRegularExpression>
#include <QSpinBox>
#include <QToolButton>
#include <QWidgetAction>

#define NUMBER "([+-]?\\d+(?:\\.\\d*)?|[+-]?\\.\\d+)"

static QString number(double value) {
	return QString::number(value, 'g', 10);
}

static QDoubleSpinBox *doubleSpin(double min, double max, double step, int decimals, double value) {
	QDoubleSpinBox *spin = new QDoubleSpinBox();
	spin->setRange(min, max);
	spin->setSingleStep(step);
	spin->setDecimals(decimals);
	spin->setValue(value);
	return spin;
}

static QSpinBox *intSpin(int min, int max, int value) {
	QSpinBox *spin = new QSpinBox();
	spin->setRange(min, max);
	spin->setValue(value);
	return spin;
}

static QComboBox *combo(const QStringList& items) {
	QComboBox *box = new QComboBox();
	box->addItems(items);
	return box;
}

/* a number that fits the spin box exactly, otherwise the form can not show it */
static bool setSpin(QDoubleSpinBox *spin, const QString& text) {
	bool ok;
	double value = text.toDouble(&ok);
	if (!ok || value < spin->minimum() || value > spin->maximum()) {
		return false;
	}
	spin->setValue(value);
	return qAbs(spin->value() - value) < 1e-9;
}

static bool setSpin(QSpinBox *spin, const QString& text) {
	bool ok;
	int value = text.toInt(&ok);
	if (!ok || value < spin->minimum() || value > spin->maximum()) {
		return false;
	}
	spin->setValue(value);
	return true;
}

static bool setCombo(QComboBox *box, const QString& text) {
	int index = box->findText(text.trimmed(), Qt::MatchFixedString);
	if (index < 0) {
		return false;
	}
	box->setCurrentIndex(index);
	return true;
}

SchedulerTaskForm::SchedulerTaskForm(QWidget *parent) : QWidget(parent) {
	m_layout = new QGridLayout(this);
	m_layout->setContentsMargins(4, 4, 4, 4);
	m_layout->setVerticalSpacing(3);
	m_layout->setColumnStretch(1, 1);
	m_layout->setAlignment(Qt::AlignTop);

	// @task
	m_name = new QLineEdit();
	m_name->setMaxLength(40);
	m_name->setPlaceholderText("required, unique, without '|'");
	addField("task", "Name", "The name of the task, 1-40 bytes, without '|', unique", { m_name },
		[this]() { return m_name->text().trimmed(); },
		[this](const QString& value) { m_name->setText(value); return true; },
		[this]() { m_name->clear(); }
	);

	// @priority
	m_priority_class = combo({ "urgent", "high", "normal", "low" });
	m_priority_level = intSpin(0, 100, 50);
	connect(m_priority_class, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { updateEnabled(); });
	addField("priority", "Priority", "The class decides first; the level 0-100 and the sky bonuses decide within a class.\nThe level of an urgent task is ignored.", { m_priority_class, m_priority_level },
		[this]() {
			if (m_priority_class->currentText() == "urgent") return QString("urgent");
			return m_priority_class->currentText() + " " + QString::number(m_priority_level->value());
		},
		[this](const QString& value) {
			QStringList tokens = value.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
			if (tokens.isEmpty() || tokens.size() > 2 || !setCombo(m_priority_class, tokens[0])) return false;
			m_priority_level->setValue(50);
			return tokens.size() == 1 || setSpin(m_priority_level, tokens[1]);
		},
		[this]() { m_priority_class->setCurrentText("normal"); m_priority_level->setValue(50); }
	);

	// @target
	m_target_ra = new QLineEditSG(QLineEditSG::RA);
	m_target_ra->setPlaceholderText("RA hh:mm:ss");
	m_target_dec = new QLineEditSG(QLineEditSG::DEC);
	m_target_dec->setPlaceholderText("Dec ±dd:mm:ss");
	QToolButton *find = new QToolButton();
	find->setIcon(QIcon(":resource/find.png"));
	find->setToolTip("Select the object from the database; an empty task name is set to the object name");
	connect(find, &QToolButton::clicked, this, [this, find]() {
		QMenu *menu = new QMenu(this);
		SelectObjectWidget *select = new SelectObjectWidget();
		QWidgetAction *action = new QWidgetAction(menu);
		action->setDefaultWidget(select);
		menu->addAction(action);
		connect(select, &SelectObjectWidget::objectSelected, this, [this, menu](const QString &name, double ra, double dec) {
			m_target_ra->setValue(ra);
			m_target_dec->setValue(dec);
			if (m_name->text().trimmed().isEmpty()) {
				m_name->setText(name.left(40));
			}
			menu->close();
			emit changed();
		});
		menu->exec(find->mapToGlobal(QPoint(0, find->height())));
		delete menu;
	});
	addField("target", "Target", "J2000 coordinates, RA in hours and Dec in degrees.\nThe body uses them as scheduler_task.ra and scheduler_task.dec.", { m_target_ra, m_target_dec, find },
		[this]() {
			return SexagesimalConverter::doubleToString(m_target_ra->value(), 2) + " " + SexagesimalConverter::doubleToString(m_target_dec->value(), 2, true);
		},
		[this](const QString& value) {
			QStringList tokens = value.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
			if (tokens.size() != 2) return false;
			bool ok_ra, ok_dec;
			double ra = SexagesimalConverter::stringToDouble(tokens[0], &ok_ra);
			double dec = SexagesimalConverter::stringToDouble(tokens[1], &ok_dec);
			if (!ok_ra || !ok_dec || ra < 0 || ra >= 24 || dec < -90 || dec > 90) return false;
			m_target_ra->setValue(ra);
			m_target_dec->setValue(dec);
			return true;
		},
		[this]() { m_target_ra->clear(); m_target_dec->clear(); }
	);

	// @altitude
	QDoubleSpinBox *altitude = doubleSpin(0, 90, 1, 1, 30);
	altitude->setPrefix("> ");
	altitude->setSuffix("°");
	addField("altitude", "Altitude", "Minimum target altitude", { altitude },
		[altitude]() { return "> " + number(altitude->value()); },
		[altitude](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^>=?\\s*" NUMBER "$").match(value.trimmed());
			return match.hasMatch() && setSpin(altitude, match.captured(1));
		},
		[altitude]() { altitude->setValue(30); }
	);

	// @airmass
	QDoubleSpinBox *airmass = doubleSpin(1, 100, 0.1, 2, 2);
	airmass->setPrefix("< ");
	addField("airmass", "Airmass", "Maximum airmass", { airmass },
		[airmass]() { return "< " + number(airmass->value()); },
		[airmass](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^<=?\\s*" NUMBER "$").match(value.trimmed());
			return match.hasMatch() && setSpin(airmass, match.captured(1));
		},
		[airmass]() { airmass->setValue(2); }
	);

	// @ha
	QDoubleSpinBox *ha_from = doubleSpin(-12, 12, 0.5, 2, -2);
	QDoubleSpinBox *ha_to = doubleSpin(-12, 12, 0.5, 2, 2);
	QLabel *ha_dots = new QLabel("..");
	addField("ha", "Hour angle", "Allowed hour angle in hours; when 'from' is greater than 'to' the range wraps across 12h", { ha_from, ha_dots, ha_to },
		[ha_from, ha_to]() { return number(ha_from->value()) + " .. " + number(ha_to->value()); },
		[ha_from, ha_to](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^" NUMBER "\\s*\\.\\.\\s*" NUMBER "$").match(value.trimmed());
			return match.hasMatch() && setSpin(ha_from, match.captured(1)) && setSpin(ha_to, match.captured(2));
		},
		[ha_from, ha_to]() { ha_from->setValue(-2); ha_to->setValue(2); }
	);

	// @moon
	QDoubleSpinBox *moon = doubleSpin(0, 180, 1, 1, 40);
	moon->setPrefix("> ");
	moon->setSuffix("°");
	QCheckBox *moon_has_width = new QCheckBox("width");
	moon_has_width->setToolTip("Lorentzian moon avoidance: the full distance applies at full moon, half of it 'width' days before and after");
	QDoubleSpinBox *moon_width = doubleSpin(0.5, 30, 0.5, 1, 7);
	moon_width->setSuffix(" d");
	connect(moon_has_width, &QCheckBox::toggled, this, [this]() { updateEnabled(); });
	addField("moon", "Moon distance", "Minimum distance from the Moon, ignored while the Moon is below the horizon", { moon, moon_has_width, moon_width },
		[moon, moon_has_width, moon_width]() {
			QString text = "> " + number(moon->value());
			if (moon_has_width->isChecked()) text += " width " + number(moon_width->value());
			return text;
		},
		[moon, moon_has_width, moon_width](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^>=?\\s*" NUMBER "(?:\\s+width\\s+" NUMBER ")?$", QRegularExpression::CaseInsensitiveOption).match(value.trimmed());
			if (!match.hasMatch() || !setSpin(moon, match.captured(1))) return false;
			moon_has_width->setChecked(!match.captured(2).isEmpty());
			return match.captured(2).isEmpty() || setSpin(moon_width, match.captured(2));
		},
		[moon, moon_has_width, moon_width]() { moon->setValue(40); moon_has_width->setChecked(false); moon_width->setValue(7); }
	);
	m_fields.last().widgets.removeOne(moon_width);

	// @illumination
	QDoubleSpinBox *illumination = doubleSpin(0, 100, 5, 1, 50);
	illumination->setPrefix("< ");
	illumination->setSuffix(" %");
	addField("illumination", "Moon illumination", "Maximum Moon illumination, ignored while the Moon is below the horizon", { illumination },
		[illumination]() { return "< " + number(illumination->value()); },
		[illumination](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^<=?\\s*" NUMBER "$").match(value.trimmed());
			return match.hasMatch() && setSpin(illumination, match.captured(1));
		},
		[illumination]() { illumination->setValue(50); }
	);

	// @twilight
	QComboBox *twilight = combo({ "none", "civil", "nautical", "astronomical" });
	addField("twilight", "Darkness", "civil, nautical and astronomical: the Sun must be below -6°, -12° or -18°", { twilight },
		[twilight]() { return twilight->currentText(); },
		[twilight](const QString& value) { return setCombo(twilight, value); },
		[twilight]() { twilight->setCurrentText("astronomical"); }
	);

	// @window
	m_window_kind = combo({ "nightly", "absolute" });
	m_window_kind->setToolTip("nightly: HH:MM every night, never expires\nabsolute: YYYY-MM-DD HH:MM, the task expires when the end has passed");
	m_window_has_start = new QCheckBox("from");
	m_window_start = new QDateTimeEdit();
	m_window_start->setTimeSpec(Qt::UTC);
	m_window_has_end = new QCheckBox("to");
	m_window_end = new QDateTimeEdit();
	m_window_end->setTimeSpec(Qt::UTC);
	auto window_format = [this]() {
		QString format = m_window_kind->currentText() == "nightly" ? "HH:mm" : "yyyy-MM-dd HH:mm";
		m_window_start->setDisplayFormat(format);
		m_window_end->setDisplayFormat(format);
	};
	connect(m_window_kind, QOverload<int>::of(&QComboBox::currentIndexChanged), this, window_format);
	connect(m_window_has_start, &QCheckBox::toggled, this, [this]() { updateEnabled(); });
	connect(m_window_has_end, &QCheckBox::toggled, this, [this]() { updateEnabled(); });
	auto window_reset = [this, window_format]() {
		m_window_kind->setCurrentText("nightly");
		window_format();
		QDate today = QDateTime::currentDateTimeUtc().date();
		m_window_start->setDateTime(QDateTime(today, QTime(20, 0), Qt::UTC));
		m_window_end->setDateTime(QDateTime(today.addDays(1), QTime(4, 0), Qt::UTC));
		m_window_has_start->setChecked(true);
		m_window_has_end->setChecked(true);
	};
	window_reset();
	addField("window", "Time window", "Time window in UTC; the whole run must fit in it", { m_window_kind, nullptr, m_window_has_start, m_window_start, m_window_has_end, m_window_end },
		[this]() {
			QString format = m_window_kind->currentText() == "nightly" ? "HH:mm" : "yyyy-MM-dd HH:mm";
			QString start = m_window_has_start->isChecked() ? m_window_start->dateTime().toUTC().toString(format) : QString();
			QString end = m_window_has_end->isChecked() ? m_window_end->dateTime().toUTC().toString(format) : QString();
			return (start + " .. " + end).trimmed();
		},
		[this, window_format](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^(?:(\\d{4}-\\d{2}-\\d{2})\\s+)?(\\d{1,2}:\\d{2})?\\s*\\.\\.\\s*(?:(\\d{4}-\\d{2}-\\d{2})\\s+)?(\\d{1,2}:\\d{2})?$").match(value.trimmed());
			if (!match.hasMatch()) return false;
			bool has_start = !match.captured(2).isEmpty();
			bool has_end = !match.captured(4).isEmpty();
			bool start_date = !match.captured(1).isEmpty();
			bool end_date = !match.captured(3).isEmpty();
			if ((!has_start && start_date) || (!has_end && end_date) || (!has_start && !has_end)) return false;
			bool absolute = start_date || end_date;
			// both ends use the same form
			if ((has_start && start_date != absolute) || (has_end && end_date != absolute)) return false;
			m_window_kind->setCurrentText(absolute ? "absolute" : "nightly");
			window_format();
			QDate today = QDateTime::currentDateTimeUtc().date();
			auto parse_end = [&](const QString& date, const QString& clock, QDateTimeEdit *edit) {
				QTime time = QTime::fromString(clock.length() == 4 ? "0" + clock : clock, "HH:mm");
				QDate day = absolute ? QDate::fromString(date, "yyyy-MM-dd") : today;
				if (!time.isValid() || !day.isValid()) return false;
				edit->setDateTime(QDateTime(day, time, Qt::UTC));
				return true;
			};
			m_window_has_start->setChecked(has_start);
			m_window_has_end->setChecked(has_end);
			return (!has_start || parse_end(match.captured(1), match.captured(2), m_window_start)) &&
				(!has_end || parse_end(match.captured(3), match.captured(4), m_window_end));
		},
		window_reset
	);
	m_fields.last().widgets.removeOne(m_window_start);
	m_fields.last().widgets.removeOne(m_window_end);

	// @duration
	QSpinBox *duration = intSpin(1, 1440, 60);
	duration->setSuffix(" min");
	addField("duration", "Duration", "The expected run time; without it the scheduler estimates it from the sequence.\nIt is not a time limit.", { duration },
		[duration]() { return QString::number(duration->value()); },
		[duration](const QString& value) { return setSpin(duration, value.trimmed()); },
		[duration]() { duration->setValue(60); }
	);

	// @repeat
	QSpinBox *repeat = intSpin(1, 10000, 2);
	repeat->setSuffix(" runs");
	QCheckBox *repeat_has_every = new QCheckBox("every");
	QSpinBox *repeat_every = intSpin(0, 100000, 30);
	repeat_every->setSuffix(" min");
	connect(repeat_has_every, &QCheckBox::toggled, this, [this]() { updateEnabled(); });
	addField("repeat", "Repeat", "The number of successful runs, with an optional pause between them", { repeat, repeat_has_every, repeat_every },
		[repeat, repeat_has_every, repeat_every]() {
			QString text = QString::number(repeat->value());
			if (repeat_has_every->isChecked()) text += " every " + QString::number(repeat_every->value());
			return text;
		},
		[repeat, repeat_has_every, repeat_every](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^(\\d+)(?:\\s+every\\s+(\\d+))?$", QRegularExpression::CaseInsensitiveOption).match(value.trimmed());
			if (!match.hasMatch() || !setSpin(repeat, match.captured(1))) return false;
			repeat_has_every->setChecked(!match.captured(2).isEmpty());
			return match.captured(2).isEmpty() || setSpin(repeat_every, match.captured(2));
		},
		[repeat, repeat_has_every, repeat_every]() { repeat->setValue(2); repeat_has_every->setChecked(false); repeat_every->setValue(30); }
	);
	m_fields.last().widgets.removeOne(repeat_every);

	// @retry
	QSpinBox *retry = intSpin(0, 100, 1);
	retry->setSuffix(" times");
	QCheckBox *retry_has_after = new QCheckBox("after");
	QSpinBox *retry_after = intSpin(0, 100000, 10);
	retry_after->setSuffix(" min");
	connect(retry_has_after, &QCheckBox::toggled, this, [this]() { updateEnabled(); });
	addField("retry", "Retry", "Reruns after failures, shared by all runs of the task, with an optional delay", { retry, retry_has_after, retry_after },
		[retry, retry_has_after, retry_after]() {
			QString text = QString::number(retry->value());
			if (retry_has_after->isChecked()) text += " after " + QString::number(retry_after->value());
			return text;
		},
		[retry, retry_has_after, retry_after](const QString& value) {
			QRegularExpressionMatch match = QRegularExpression("^(\\d+)(?:\\s+after\\s+(\\d+))?$", QRegularExpression::CaseInsensitiveOption).match(value.trimmed());
			if (!match.hasMatch() || !setSpin(retry, match.captured(1))) return false;
			retry_has_after->setChecked(!match.captured(2).isEmpty());
			return match.captured(2).isEmpty() || setSpin(retry_after, match.captured(2));
		},
		[retry, retry_has_after, retry_after]() { retry->setValue(1); retry_has_after->setChecked(false); retry_after->setValue(10); }
	);
	m_fields.last().widgets.removeOne(retry_after);

	// @preempted
	QComboBox *preempted = combo({ "retry", "fail" });
	addField("preempted", "When preempted", "What happens to this task when an urgent task interrupts it", { preempted },
		[preempted]() { return preempted->currentText(); },
		[preempted](const QString& value) { return setCombo(preempted, value); },
		[preempted]() { preempted->setCurrentText("retry"); }
	);

	// @enabled
	QComboBox *enabled = combo({ "yes", "no" });
	addField("enabled", "Enabled", "A disabled task is kept but never selected", { enabled },
		[enabled]() { return enabled->currentText(); },
		[enabled](const QString& value) { return setCombo(enabled, value); },
		[enabled]() { enabled->setCurrentText("no"); }
	);

	// free text keys
	struct TextKey { const char *key; const char *label; const char *tooltip; int length; };
	static const TextKey text_keys[] = {
		{ "folder", "Folder", "Save the frames of this task in this subfolder, shared by tasks with the same folder; 'none' leaves the folder of the Imager Agent alone", 40 },
		{ "observer", "Observer", "Written as the FITS header OBSERVER", 56 },
		{ "pi", "PI", "The principal investigator, written as the FITS header PI_NAME", 56 },
		{ "project", "Project", "The project or proposal identifier, written as the FITS header PROPID", 56 },
	};
	for (const TextKey& text_key : text_keys) {
		QLineEdit *edit = new QLineEdit();
		edit->setMaxLength(text_key.length);
		addField(text_key.key, text_key.label, text_key.tooltip, { edit },
			[edit]() { return edit->text().trimmed(); },
			[edit](const QString& value) { edit->setText(value.trimmed()); return !value.trimmed().isEmpty(); },
			[edit]() { edit->clear(); }
		);
	}

	reset();
}

void SchedulerTaskForm::addField(const QString& key, const QString& label, const QString& tooltip, QList<QWidget*> widgets,
	std::function<QString()> format, std::function<bool(const QString&)> parse, std::function<void()> reset) {
	Field field;
	field.key = key;
	field.format = format;
	field.parse = parse;
	field.reset = reset;
	field.widgets = widgets;

	// a null widget starts a new line of the field
	QWidget *row = nullptr;
	QHBoxLayout *row_layout = nullptr;
	auto new_row = [&]() {
		if (row_layout) row_layout->addStretch();
		row = new QWidget();
		row_layout = new QHBoxLayout(row);
		row_layout->setContentsMargins(0, 0, 0, 0);
		row_layout->setSpacing(4);
		field.rows.append(row);
	};
	new_row();
	field.widgets.clear();
	for (QWidget *widget : widgets) {
		if (widget == nullptr) {
			new_row();
			continue;
		}
		field.widgets.append(widget);
		row_layout->addWidget(widget);
		connectChanged(widget);
	}
	row_layout->addStretch();

	if (key == "task") {
		QLabel *name_label = new QLabel(label + ":");
		name_label->setStyleSheet("font-weight: bold;");
		name_label->setToolTip(tooltip);
		m_layout->addWidget(name_label, m_row, 0);
	} else {
		field.enable = new QCheckBox(label);
		field.enable->setToolTip(QString("@%1: %2\nUnchecked: not set, the scheduler uses its default").arg(key, tooltip));
		connect(field.enable, &QCheckBox::toggled, this, [this]() {
			updateEnabled();
			if (!m_loading) emit changed();
		});
		m_layout->addWidget(field.enable, m_row, 0);
	}
	for (QWidget *line : field.rows) {
		line->setToolTip(QString("@%1: %2").arg(key, tooltip));
		m_layout->addWidget(line, m_row++, 1);
	}
	m_fields.append(field);
}

void SchedulerTaskForm::connectChanged(QWidget *widget) {
	auto notify = [this]() { if (!m_loading) emit changed(); };
	if (QLineEdit *edit = qobject_cast<QLineEdit*>(widget)) {
		connect(edit, &QLineEdit::textEdited, this, notify);
	} else if (QSpinBox *spin = qobject_cast<QSpinBox*>(widget)) {
		connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, notify);
	} else if (QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(widget)) {
		connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, notify);
	} else if (QComboBox *box = qobject_cast<QComboBox*>(widget)) {
		connect(box, QOverload<int>::of(&QComboBox::currentIndexChanged), this, notify);
	} else if (QCheckBox *check = qobject_cast<QCheckBox*>(widget)) {
		connect(check, &QCheckBox::toggled, this, notify);
	} else if (QDateTimeEdit *edit = qobject_cast<QDateTimeEdit*>(widget)) {
		connect(edit, &QDateTimeEdit::dateTimeChanged, this, notify);
	}
}

SchedulerTaskForm::Field *SchedulerTaskForm::field(const QString& key) {
	for (Field& field : m_fields) {
		if (field.key == key) return &field;
	}
	return nullptr;
}

const SchedulerTaskForm::Field *SchedulerTaskForm::field(const QString& key) const {
	for (const Field& field : m_fields) {
		if (field.key == key) return &field;
	}
	return nullptr;
}

void SchedulerTaskForm::updateEnabled() {
	for (Field& field : m_fields) {
		bool on = !m_readOnly && (field.enable == nullptr || field.enable->isChecked());
		if (field.enable) {
			field.enable->setEnabled(!m_readOnly);
		}
		for (QWidget *row : field.rows) {
			for (QWidget *child : row->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly)) {
				child->setEnabled(on);
			}
		}
	}
	// the dependent parts of composite values
	m_priority_level->setEnabled(m_priority_level->isEnabled() && m_priority_class->currentText() != "urgent");
	m_window_start->setEnabled(m_window_start->isEnabled() && m_window_has_start->isChecked());
	m_window_end->setEnabled(m_window_end->isEnabled() && m_window_has_end->isChecked());
	for (QCheckBox *option : findChildren<QCheckBox*>()) {
		// "width", "every" and "after" switch the spin box that follows them in their row
		QWidget *row = option->parentWidget();
		if (row == this || row == nullptr) continue;
		QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(row->layout());
		if (!layout || option == m_window_has_start || option == m_window_has_end) continue;
		int index = layout->indexOf(option);
		if (index >= 0 && index + 1 < layout->count()) {
			QWidget *next = layout->itemAt(index + 1)->widget();
			if (next) next->setEnabled(next->isEnabled() && option->isChecked());
		}
	}
}

void SchedulerTaskForm::reset() {
	m_loading = true;
	for (Field& field : m_fields) {
		field.reset();
		if (field.enable) field.enable->setChecked(false);
	}
	m_loaded.clear();
	m_order.clear();
	m_loading = false;
	updateEnabled();
}

bool SchedulerTaskForm::setHeader(const QList<QPair<QString, QString>>& header, QString *error) {
	reset();
	m_loading = true;
	QString problem;
	for (const auto& entry : header) {
		Field *f = field(entry.first);
		if (f == nullptr) {
			problem = QString("@%1 is not a known key").arg(entry.first);
			break;
		}
		if (!f->parse(entry.second)) {
			problem = QString("@%1 %2 can not be shown").arg(entry.first, entry.second);
			break;
		}
		if (f->enable) f->enable->setChecked(true);
		m_loaded[entry.first] = qMakePair(entry.second, f->format());
		m_order.append(entry.first);
	}
	m_loading = false;
	if (!problem.isEmpty()) {
		reset();
		if (error) *error = problem;
		return false;
	}
	updateEnabled();
	return true;
}

QList<QPair<QString, QString>> SchedulerTaskForm::header() const {
	QList<QPair<QString, QString>> header;
	QStringList order = m_order;
	for (const QString& key : SchedulerTask::knownKeys()) {
		if (!order.contains(key)) order.append(key);
	}
	for (const QString& key : order) {
		const Field *f = field(key);
		if (f == nullptr || (f->enable && !f->enable->isChecked())) continue;
		QString value = f->format();
		if (m_loaded.contains(key) && m_loaded[key].second == value) {
			value = m_loaded[key].first;
		}
		if (key != "task" && value.isEmpty()) continue;
		header.append(qMakePair(key, value));
	}
	return header;
}

QString SchedulerTaskForm::taskName() const {
	return m_name->text().trimmed();
}

void SchedulerTaskForm::setTaskName(const QString& name) {
	m_name->setText(name);
}

bool SchedulerTaskForm::hasTarget() const {
	const Field *f = field("target");
	return f && f->enable->isChecked();
}

void SchedulerTaskForm::setReadOnly(bool readOnly) {
	m_readOnly = readOnly;
	updateEnabled();
}
