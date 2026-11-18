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

#include "SchedulerTaskEditor.h"
#include "SchedulerTaskForm.h"
#include "SchedulerTask.h"
#include "IndigoSequence.h"
#include "IndigoSequenceItem.h"
#include "SequenceItemModel.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>

static QString s_last_dir;

SchedulerTaskEditor::SchedulerTaskEditor(QWidget *parent) : QWidget(parent) {
	setObjectName("SchedulerTaskEditor");
	QVBoxLayout *layout = new QVBoxLayout(this);

	QHBoxLayout *toolbar = new QHBoxLayout();
	toolbar->setContentsMargins(1, 1, 1, 1);
	m_title = new QLabel();
	m_title->setObjectName("SchedulerEditorTitle");
	m_title->setStyleSheet("QLabel { font-weight: bold; }");
	toolbar->addWidget(m_title, 1);

	m_new_button = new QToolButton();
	m_new_button->setText("New");
	m_new_button->setToolTip("Start a new task");
	connect(m_new_button, &QToolButton::clicked, this, &SchedulerTaskEditor::onNew);
	toolbar->addWidget(m_new_button);

	m_open_button = new QToolButton();
	m_open_button->setIcon(QIcon(":resource/folder.png"));
	m_open_button->setToolTip("Open a task file");
	connect(m_open_button, &QToolButton::clicked, this, &SchedulerTaskEditor::onOpenFile);
	toolbar->addWidget(m_open_button);

	m_save_button = new QToolButton();
	m_save_button->setIcon(QIcon(":resource/save.png"));
	m_save_button->setToolTip("Save the task to a file");
	connect(m_save_button, &QToolButton::clicked, this, &SchedulerTaskEditor::onSaveFile);
	toolbar->addWidget(m_save_button);

	toolbar->addSpacing(12);

	m_apply_button = new QPushButton("Apply");
	m_apply_button->setToolTip("Store the task in the scheduler: the stored task it was opened from, or the one with its name, is updated\nand keeps its state and counters; otherwise it is added as a new task");
	connect(m_apply_button, &QPushButton::clicked, this, [this]() {
		emit applyTask(m_server_name, taskText());
	});
	toolbar->addWidget(m_apply_button);

	m_urgent_button = new QPushButton("Submit urgent");
	m_urgent_button->setToolTip("Run this task once as urgent: it interrupts the running task as soon as its own constraints hold.\nIt is not stored and is removed after its run.");
	connect(m_urgent_button, &QPushButton::clicked, this, [this]() {
		if (QMessageBox::question(this, "Submit urgent task",
			"The urgent task interrupts the running task as soon as its constraints hold.\n\nSubmit it?") == QMessageBox::Yes) {
			emit submitUrgent(taskText());
		}
	});
	toolbar->addWidget(m_urgent_button);
	layout->addLayout(toolbar);

	m_status = new QLabel();
	m_status->setObjectName("SchedulerEditorStatus");
	m_status->setWordWrap(true);
	m_status->setTextFormat(Qt::RichText);
	m_status->setVisible(false);
	layout->addWidget(m_status);

	// Steps, Properties and Extra edit the task in forms, Text edits all of it as JavaScript;
	// the pages of the side that is not edited are filled from the other one when shown
	m_pages = new QTabWidget();
	m_pages->setObjectName("SchedulerEditorPages");
	// the pages fill the tab pane, as the sequence editor fills its frame in the Sequence tab
	m_pages->setStyleSheet("QTabWidget#SchedulerEditorPages > QStackedWidget { padding: 0px; }");
	layout->addWidget(m_pages, 1);

	m_form = new SchedulerTaskForm();
	QScrollArea *form_scroll = new QScrollArea();
	form_scroll->setWidget(m_form);
	form_scroll->setWidgetResizable(true);
	form_scroll->setFrameShape(QFrame::NoFrame);
	m_form_page = form_scroll;
	QScrollArea *observer_scroll = new QScrollArea();
	observer_scroll->setWidget(m_form->observerPage());
	observer_scroll->setWidgetResizable(true);
	observer_scroll->setFrameShape(QFrame::NoFrame);
	m_observer_page = observer_scroll;
	m_body = new IndigoSequence();
	m_body->setTaskMode(true);
	connect(m_form, &SchedulerTaskForm::targetChanged, this, &SchedulerTaskEditor::applyTargetToSteps);
	connect(m_pages, &QTabWidget::currentChanged, this, &SchedulerTaskEditor::onPageChanged);

	m_text = new QPlainTextEdit();
	m_text->setObjectName("SchedulerEditorText");
	m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_text->setTabStopDistance(4 * m_text->fontMetrics().horizontalAdvance(' '));

	m_pages->addTab(m_body, "Steps");
	m_pages->setTabToolTip(0, "The sequence the task runs");
	m_pages->addTab(m_form_page, "Properties");
	m_pages->setTabToolTip(1, "The header of the task: when and how the scheduler runs it");
	m_pages->addTab(m_observer_page, "Extra");
	m_pages->setTabToolTip(2, "The observer, PI and project written to the FITS headers of the frames");
	m_pages->addTab(m_text, "Text");
	m_pages->setTabToolTip(3, "The whole task as JavaScript with its // @key header.\nA task with code other than steps can only be edited as text.");

	newTask();
}

void SchedulerTaskEditor::showPage(QWidget *page) {
	m_switching = true;
	m_pages->setCurrentWidget(page);
	m_switching = false;
}

void SchedulerTaskEditor::onPageChanged() {
	if (m_switching) {
		return;
	}
	QWidget *page = m_pages->currentWidget();
	if (page == m_text) {
		if (!m_text_mode) {
			m_text->setPlainText(taskText());
			m_text_mode = true;
		}
	} else if (m_text_mode) {
		QString error;
		if (!showVisual(m_text->toPlainText(), &error, page)) {
			showPage(m_text);
			setStatus(QString("The task can only be edited as text, the forms can not show it: %1.").arg(error), StatusAlert);
			return;
		}
		setStatus(QString());
	} else if (page == m_form_page) {
		// the steps may have been edited: the form shows the goto they have now
		showGotoOfSteps();
	}
	updateButtons();
}

bool SchedulerTaskEditor::isVisualMode() const {
	return !m_text_mode;
}

QString SchedulerTaskEditor::taskText() const {
	if (!isVisualMode()) {
		return m_text->toPlainText();
	}
	SchedulerTask task;
	task.setHeader(m_form->header());
	task.setBody(m_body->makeScriptFromView());
	return task.toText();
}

bool SchedulerTaskEditor::isModified() const {
	return taskText() != m_saved_text;
}

bool SchedulerTaskEditor::showVisual(const QString& text, QString *error, QWidget *page) {
	SchedulerTask task = SchedulerTask::fromText(text);
	// the sequence editor drops comments, notes in the body would be lost
	static const QRegularExpression comment(R"(^\s*//(?!\*))", QRegularExpression::MultilineOption);
	if (comment.match(task.body()).hasMatch()) {
		if (error) *error = "the body has comments";
		return false;
	}
	if (!m_form->setHeader(task.header(), error)) {
		return false;
	}
	if (task.body().trimmed().isEmpty()) {
		m_body->clear();
	} else if (!m_body->loadScriptExact(task.body(), error)) {
		m_form->reset();
		return false;
	}
	showGotoOfSteps();
	m_text_mode = false;
	if (page) {
		showPage(page);
	} else if (m_pages->currentWidget() == m_text) {
		showPage(m_body);
	}
	return true;
}

// the first goto of the steps that runs, a slew or a precise goto, in a repeat as well
static FunctionCall *first_goto(QVector<FunctionCall>& calls) {
	for (FunctionCall& call : calls) {
		if (call.omitted) {
			continue;
		}
		if (call.functionName == SC_SLEW || call.functionName == SC_PRECISE_GOTO) {
			return &call;
		}
		if (FunctionCall *nested = first_goto(call.nestedCalls)) {
			return nested;
		}
	}
	return nullptr;
}

void SchedulerTaskEditor::showGotoOfSteps() {
	QVector<FunctionCall> calls = m_body->functionCallsFromView();
	FunctionCall *call = first_goto(calls);
	bool precise = call && call->functionName == SC_PRECISE_GOTO && !call->parameters.isEmpty();
	m_form->setGoto(precise, precise ? call->parameters.first().toDouble() : m_form->preciseExposure());
}

void SchedulerTaskEditor::applyTargetToSteps() {
	// the first goto goes to the task target, a new one is the first step
	QVector<FunctionCall> calls = m_body->functionCallsFromView();
	FunctionCall *call = first_goto(calls);
	FunctionCall target_goto;
	target_goto.omitted = false;
	target_goto.objectName = "sequence";
	if (m_form->preciseGoto()) {
		target_goto.functionName = SC_PRECISE_GOTO;
		target_goto.parameters = QStringList() << QString::number(m_form->preciseExposure(), 'g', 10) << TASK_TARGET_RA << TASK_TARGET_DEC;
	} else {
		target_goto.functionName = SC_SLEW;
		target_goto.parameters = QStringList() << TASK_TARGET_RA << TASK_TARGET_DEC;
	}
	if (call) {
		if (call->functionName == target_goto.functionName && call->parameters == target_goto.parameters) {
			return;
		}
		call->functionName = target_goto.functionName;
		call->parameters = target_goto.parameters;
	} else {
		// after the constructor of the sequence
		calls.insert(calls.isEmpty() ? 0 : 1, target_goto);
	}
	m_body->viewFromFunctionCalls(calls);
	if (call) {
		setStatus("The first goto of the steps now goes to the target.");
	} else {
		setStatus(QString("A %1 to the target is added as the first step.").arg(m_form->preciseGoto() ? "precise goto" : "slew"));
	}
	updateButtons();
}

void SchedulerTaskEditor::showText(const QString& text) {
	m_text->setPlainText(text);
	m_text_mode = true;
	showPage(m_text);
}

void SchedulerTaskEditor::loadTask(const QString& text, const QString& serverName) {
	m_loading = true;
	// another task opens on its steps, the same task shown again keeps its page
	bool other = serverName.isEmpty() || serverName != m_server_name;
	m_server_name = serverName;
	setStatus(QString());
	QString error;
	if (!showVisual(text, &error)) {
		showText(text);
		setStatus(QString("Shown as text, the visual editor can not show it: %1.").arg(error));
	} else if (other) {
		showPage(m_body);
	}
	m_saved_text = taskText();
	m_loading = false;
	updateTitle();
	updateButtons();
}

void SchedulerTaskEditor::newTask() {
	m_server_name.clear();
	m_form->reset();
	m_form->setTaskName("New task");
	m_body->clear();
	m_form->setGoto(false, 1);
	m_text_mode = false;
	showPage(m_body);
	setStatus(QString());
	m_saved_text = taskText();
	updateTitle();
	updateButtons();
}

bool SchedulerTaskEditor::confirmDiscard() {
	if (!isModified()) {
		return true;
	}
	// the name the task has in the editor, it may have been renamed
	QString name = SchedulerTask::fromText(taskText()).name().trimmed();
	if (name.isEmpty()) name = m_server_name;
	QString task = name.isEmpty() ? QString("The task being edited") : QString("The task '%1' is being edited and").arg(name);
	return QMessageBox::question(this, "Task modified",
		QString("%1 has changes that are not sent to the scheduler or saved.\n\nAre you sure you want to discard them?").arg(task)) == QMessageBox::Yes;
}

void SchedulerTaskEditor::onNew() {
	if (confirmDiscard()) {
		newTask();
	}
}

void SchedulerTaskEditor::onOpenFile() {
	if (!confirmDiscard()) {
		return;
	}
	QString file_name = QFileDialog::getOpenFileName(this, "Open Task", s_last_dir.isEmpty() ? QDir::homePath() : s_last_dir,
		"Task files (*.task *.js);;All files (*)");
	if (file_name.isEmpty()) {
		return;
	}
	QFile file(file_name);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::critical(this, "Open Task", QString("Can not open %1: %2").arg(file_name, file.errorString()));
		return;
	}
	s_last_dir = QFileInfo(file_name).absolutePath();
	QTextStream in(&file);
	loadTask(in.readAll(), QString());
	m_title->setText(QString("Task file: %1").arg(QFileInfo(file_name).fileName()));
}

void SchedulerTaskEditor::onSaveFile() {
	QString text = taskText();
	QString name = SchedulerTask::fromText(text).name();
	name.replace(QRegularExpression("[^A-Za-z0-9._ +-]"), "_");
	if (name.isEmpty()) {
		name = "untitled";
	}
	QString dir = s_last_dir.isEmpty() ? QDir::homePath() : s_last_dir;
	QString file_name = QFileDialog::getSaveFileName(this, "Save Task", QDir(dir).filePath(name + ".task"),
		"Task files (*.task);;All files (*)");
	if (file_name.isEmpty()) {
		return;
	}
	QFile file(file_name);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, "Save Task", QString("Can not save %1: %2").arg(file_name, file.errorString()));
		return;
	}
	s_last_dir = QFileInfo(file_name).absolutePath();
	QTextStream out(&file);
	out << text;
	if (m_server_name.isEmpty()) {
		m_saved_text = text;
	}
	setStatus(QString("Saved to %1").arg(file_name));
}

void SchedulerTaskEditor::setResult(bool ok, const QString& message) {
	if (ok) {
		QString name = SchedulerTask::fromText(taskText()).name();
		if (!name.isEmpty()) {
			m_server_name = name;
		}
		m_saved_text = taskText();
		setStatus(message.isEmpty() ? "Accepted by the scheduler" : message, StatusOk);
	} else {
		setStatus(message.isEmpty() ? "Rejected by the scheduler" : message, StatusAlert);
	}
	updateTitle();
	updateButtons();
}

void SchedulerTaskEditor::setRunning(bool running) {
	if (m_running == running) {
		return;
	}
	m_running = running;
	if (running && isVisualMode()) {
		// the steps show the progress of the run
		showPage(m_body);
	}
	m_form->setReadOnly(running);
	emit m_body->enable(!running);
	m_text->setReadOnly(running);
	if (!running) {
		m_body->setIdle();
	}
	updateTitle();
	updateButtons();
}

void SchedulerTaskEditor::showStepStates(const QVector<int>& states, int loopStep, int loopIteration) {
	if (!m_running || !isVisualMode() || states.size() != m_body->executedItemCount()) {
		return;
	}
	for (int i = 0; i < states.size(); i++) {
		IndigoSequenceItem *item = m_body->getItemAt(m_body->getItemIndexByExecutedStep(i));
		if (!item) continue;
		switch (states[i]) {
			case 1: item->setOk(); break;      // INDIGO_OK_STATE
			case 2: item->setBusy(); m_body->scrollToItem(m_body->getItemIndexByExecutedStep(i)); break;
			case 3: item->setAlert(); break;
			default: item->setIdle(); break;
		}
		item->setIteration(i == loopStep ? loopIteration + 1 : 0);
	}
}

void SchedulerTaskEditor::updateTitle() {
	if (m_server_name.isEmpty()) {
		m_title->setText("New task (not in the scheduler)");
	} else if (m_running) {
		m_title->setText(QString("Task '%1' - running, read only").arg(m_server_name));
	} else {
		m_title->setText(QString("Task '%1'").arg(m_server_name));
	}
}

void SchedulerTaskEditor::updateButtons() {
	m_apply_button->setEnabled(!m_running);
	m_urgent_button->setEnabled(!m_running);
	// a running task stays on the side it is shown on
	for (int i = 0; i < m_pages->count(); i++) {
		bool text_page = m_pages->widget(i) == m_text;
		m_pages->setTabEnabled(i, !m_running || text_page == m_text_mode);
	}
	m_new_button->setEnabled(true);
	m_open_button->setEnabled(true);
}

void SchedulerTaskEditor::setStatus(const QString& text, StatusKind kind) {
	// the result of a request has the LED of its state, as the rest of the scheduler
	QString led;
	if (kind == StatusOk) {
		led = "<img src=\":resource/led-green.png\"> ";
	} else if (kind == StatusAlert) {
		led = "<img src=\":resource/led-red.png\"> ";
	}
	m_status->setText(led + text.toHtmlEscaped());
	m_status->setVisible(!text.isEmpty());
}
