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

#ifndef __SCHEDULERTASKFORM_H
#define __SCHEDULERTASKFORM_H

#include <QWidget>
#include <QMap>
#include <QList>
#include <QPair>
#include <functional>

class QCheckBox;
class QGridLayout;
class QLineEdit;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QDateTimeEdit;
class QLabel;
class QLineEditSG;

/* The header of a scheduler task as a form. Every key except @task has a check
   box: an unchecked key is not written and the agent uses its default. A value
   that is shown but not changed is written back as it was loaded. */
class SchedulerTaskForm : public QWidget {
	Q_OBJECT
public:
	explicit SchedulerTaskForm(QWidget *parent = nullptr);

	/* Shows the header. Returns false, with the reason in error, when a key is
	   unknown or a value can not be shown; the form is then reset. */
	bool setHeader(const QList<QPair<QString, QString>>& header, QString *error);
	QList<QPair<QString, QString>> header() const;
	void reset();

	QString taskName() const;
	void setTaskName(const QString& name);
	bool hasTarget() const;
	/* @hook: the task runs only as a hook */
	bool isHook() const;
	/* the first goto of the steps: a precise goto with the exposure, or a slew */
	bool preciseGoto() const;
	double preciseExposure() const;
	void setGoto(bool precise, double exposure);

	void setReadOnly(bool readOnly);
	/* the page with @observer, @pi and @project, shown apart from the form */
	QWidget *observerPage() const { return m_observer_page; }

signals:
	void changed();
	/* the target or the kind of its goto was changed by the user, and the target is set */
	void targetChanged();

private:
	struct Field {
		QString key;
		QCheckBox *enable = nullptr;
		QList<QWidget*> widgets;
		QList<QWidget*> rows;
		std::function<QString()> format;
		std::function<bool(const QString&)> parse;
		std::function<void()> reset;
	};

	void addField(const QString& key, const QString& label, const QString& tooltip, QList<QWidget*> widgets,
		std::function<QString()> format, std::function<bool(const QString&)> parse, std::function<void()> reset);
	void updateEnabled();
	void emitTargetChanged();
	void connectChanged(QWidget *widget);
	Field *field(const QString& key);
	const Field *field(const QString& key) const;

	QGridLayout *m_layout;
	QGridLayout *m_main_layout = nullptr;
	int m_main_row = 0;
	QWidget *m_observer_page = nullptr;
	QList<Field> m_fields;
	int m_row = 0;
	bool m_readOnly = false;
	bool m_loading = false;
	bool m_target_edited = false;

	// the loaded value of a key and how the form formats it, to write unchanged values back as loaded
	QMap<QString, QPair<QString, QString>> m_loaded;
	// the keys of the loaded header in their order, the other keys follow in the order of the form
	QStringList m_order;

	QLineEdit *m_name;
	QCheckBox *m_hook;
	QComboBox *m_priority_class;
	QSpinBox *m_priority_level;
	QLineEditSG *m_target_ra;
	QLineEditSG *m_target_dec;
	QCheckBox *m_precise_goto;
	QDoubleSpinBox *m_precise_exposure;
	QComboBox *m_window_kind;
	QDateTimeEdit *m_window_start;
	QDateTimeEdit *m_window_end;
	QCheckBox *m_window_has_start;
	QCheckBox *m_window_has_end;
};

#endif // __SCHEDULERTASKFORM_H
