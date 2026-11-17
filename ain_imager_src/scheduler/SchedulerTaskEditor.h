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

class QLabel;
class QToolButton;
class QPushButton;
class QStackedWidget;
class QPlainTextEdit;
class QSplitter;
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

	/* The stored task is running: the editor shows its progress and does not
	   allow edits until the run ends. */
	void setRunning(bool running);
	bool isRunning() const { return m_running; }
	void showStepStates(const QVector<int>& states, int loopStep, int loopIteration);

	/* Asks whether to drop unsent changes; true when there are none or the user
	   agrees. */
	bool confirmDiscard();

	bool isVisualMode() const;

signals:
	void uploadTask(const QString& text);
	void applyTask(const QString& serverName, const QString& text);
	void submitUrgent(const QString& text);

protected:
	void showEvent(QShowEvent *event) override;

private slots:
	void onToggleMode();
	void onOpenFile();
	void onSaveFile();
	void onNew();

private:
	bool showVisual(const QString& text, QString *error);
	void showText(const QString& text);
	void updateTitle();
	void updateButtons();
	void setStatus(const QString& text, const QString& color = QString());

	QLabel *m_title;
	QLabel *m_status;
	QToolButton *m_mode_button;
	QPushButton *m_upload_button;
	QPushButton *m_apply_button;
	QPushButton *m_urgent_button;
	QToolButton *m_new_button;
	QToolButton *m_open_button;
	QToolButton *m_save_button;
	QStackedWidget *m_pages;
	QSplitter *m_visual;
	SchedulerTaskForm *m_form;
	IndigoSequence *m_body;
	QPlainTextEdit *m_text;

	QString m_server_name;
	// the task as it was loaded, sent or saved, to tell whether it has been changed since
	QString m_saved_text;
	bool m_running = false;
	bool m_loading = false;
	bool m_split_done = false;
};

#endif // __SCHEDULERTASKEDITOR_H
