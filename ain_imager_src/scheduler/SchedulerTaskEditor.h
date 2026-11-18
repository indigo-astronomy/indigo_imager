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

#ifndef __SCHEDULERTASKEDITOR_H
#define __SCHEDULERTASKEDITOR_H

#include <QWidget>
#include <QVector>

class QComboBox;
class QLabel;
class QToolButton;
class QPushButton;
class QTabWidget;
class QPlainTextEdit;
class IndigoSequence;
class SchedulerTaskForm;

/* Edits one scheduler task: the header in a form and the body in the sequence
   editor. A task the editor can not show completely, for example with a body
   that has JavaScript besides sequence steps, is edited as text instead, so
   nothing of it is lost. */
class SchedulerTaskEditor : public QWidget {
	Q_OBJECT
public:
	explicit SchedulerTaskEditor(QWidget *parent = nullptr);

	/* Shows a task. serverName is the name of the stored task it comes from, or
	   empty for a task that is not on the server. */
	void loadTask(const QString& text, const QString& serverName);
	void newTask();
	QString taskText() const;

	QString serverName() const { return m_server_name; }
	bool isModified() const;

	/* The result of an upload or an edit sent from this editor. */
	void setResult(bool ok, const QString& message);

	/* The task the scheduler runs, empty when none. While the editor shows it, the
	   editor shows its progress and does not allow edits until the run ends; any
	   other task, new or stored, can be edited and sent meanwhile. */
	void setRunningTask(const QString& name);
	bool isRunning() const { return m_running; }
	void showStepStates(const QVector<int>& states, int loopStep, int loopIteration);

	/* Asks whether to drop unsent changes; true when there are none or the user
	   agrees. */
	bool confirmDiscard();

	bool isVisualMode() const;

signals:
	/* stores the task: updates the stored task serverName or the one with its name, or adds it */
	void applyTask(const QString& serverName, const QString& text);
	void submitUrgent(const QString& text);

private slots:
	void onPageChanged();
	void onOpenFile();
	void onSaveFile();
	void onNew();

private:
	/* shows the text in the forms, on the page given or the one shown */
	/* keepHookKeys: the text was made from the forms, the keys a hook does not write come back */
	bool showVisual(const QString& text, QString *error, QWidget *page = nullptr, bool keepHookKeys = false);
	/* the form shows the kind of the first goto of the steps */
	void showGotoOfSteps();
	/* the first goto of the steps goes to the task target, added when there is none */
	void applyTargetToSteps();
	void showText(const QString& text);
	void updateTitle();
	void updateButtons();
	enum StatusKind { StatusInfo, StatusOk, StatusAlert };
	void setStatus(const QString& text, StatusKind kind = StatusInfo);
	void showPage(QWidget *page);

	QLabel *m_title;
	QLabel *m_status;
	QPushButton *m_apply_button;
	QPushButton *m_urgent_button;
	QToolButton *m_new_button;
	QToolButton *m_open_button;
	QToolButton *m_save_button;
	QTabWidget *m_pages;
	// the visual editor shows either the header form or the steps, each in the full width
	QWidget *m_form_page;
	QWidget *m_observer_page;
	SchedulerTaskForm *m_form;
	IndigoSequence *m_body;
	QPlainTextEdit *m_text;
	// the text page holds the task, the forms are filled from it when shown
	bool m_text_mode = false;
	bool m_switching = false;

	QString m_server_name;
	// the task as it was loaded, sent or saved, to tell whether it has been changed since
	QString m_saved_text;
	bool m_running = false;
	QString m_running_task;
	QVector<int> m_step_states;
	int m_loop_step = -1;
	int m_loop_iteration = 0;
	void setRunning(bool running);
	void updateRunning();
	bool m_loading = false;
};

#endif // __SCHEDULERTASKEDITOR_H
