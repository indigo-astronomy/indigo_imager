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
#include <QSplitter>
#include <QStackedWidget>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>

static QString s_last_dir;

SchedulerTaskEditor::SchedulerTaskEditor(QWidget *parent) : QWidget(parent) {
	setObjectName("SchedulerTaskEditor");
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(2);

	QHBoxLayout *toolbar = new QHBoxLayout();
	toolbar->setContentsMargins(4, 2, 4, 2);
	m_title = new QLabel();
	m_title->setObjectName("SchedulerEditorTitle");
	m_title->setStyleSheet("font-weight: bold;");
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

	m_mode_button = new QToolButton();
	m_mode_button->setToolTip("Switch between the visual editor and the task text");
	connect(m_mode_button, &QToolButton::clicked, this, &SchedulerTaskEditor::onToggleMode);
	toolbar->addWidget(m_mode_button);

	toolbar->addSpacing(12);

	m_apply_button = new QPushButton("Apply");
	m_apply_button->setToolTip("Replace the stored task on the server with this text; its state and counters are kept");
	connect(m_apply_button, &QPushButton::clicked, this, [this]() {
		emit applyTask(m_server_name, taskText());
	});
	toolbar->addWidget(m_apply_button);

	m_upload_button = new QPushButton("Add to scheduler");
	m_upload_button->setToolTip("Store this task in the scheduler as a new task");
	connect(m_upload_button, &QPushButton::clicked, this, [this]() {
		emit uploadTask(taskText());
	});
	toolbar->addWidget(m_upload_button);

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
	m_status->setContentsMargins(6, 0, 6, 0);
	m_status->setVisible(false);
	layout->addWidget(m_status);

	m_pages = new QStackedWidget();
	m_pages->setObjectName("SchedulerEditorPages");
	layout->addWidget(m_pages, 1);

	m_visual = new QSplitter(Qt::Horizontal);
	m_form = new SchedulerTaskForm();
	QScrollArea *form_scroll = new QScrollArea();
	form_scroll->setWidget(m_form);
	form_scroll->setWidgetResizable(true);
	form_scroll->setMinimumWidth(150);
	m_visual->addWidget(form_scroll);
	m_body = new IndigoSequence();
	m_body->setTaskMode(true);
	m_visual->addWidget(m_body);
	m_visual->setStretchFactor(0, 0);
	m_visual->setStretchFactor(1, 1);
	m_pages->addWidget(m_visual);

	m_text = new QPlainTextEdit();
	m_text->setObjectName("SchedulerEditorText");
	m_text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	m_text->setLineWrapMode(QPlainTextEdit::NoWrap);
	m_text->setTabStopDistance(4 * m_text->fontMetrics().horizontalAdvance(' '));
	m_pages->addWidget(m_text);

	newTask();
}

void SchedulerTaskEditor::showEvent(QShowEvent *event) {
	QWidget::showEvent(event);
	if (!m_split_done && m_visual->width() > 0) {
		// the whole form if it leaves the steps at least as much room, else less than half
		m_split_done = true;
		int total = m_visual->width();
		int form = m_form->sizeHint().width() + 24;
		if (form > total / 2) form = total * 45 / 100;
		m_visual->setSizes({ form, total - form });
	}
}

bool SchedulerTaskEditor::isVisualMode() const {
	return m_pages->currentWidget() == m_visual;
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

bool SchedulerTaskEditor::showVisual(const QString& text, QString *error) {
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
	m_pages->setCurrentWidget(m_visual);
	m_mode_button->setText("Text");
	return true;
}

void SchedulerTaskEditor::showText(const QString& text) {
	m_text->setPlainText(text);
	m_pages->setCurrentWidget(m_text);
	m_mode_button->setText("Visual");
}

void SchedulerTaskEditor::loadTask(const QString& text, const QString& serverName) {
	m_loading = true;
	m_server_name = serverName;
	setStatus(QString());
	QString error;
	if (!showVisual(text, &error)) {
		showText(text);
		setStatus(QString("Shown as text, the visual editor can not show it: %1.").arg(error));
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
	m_pages->setCurrentWidget(m_visual);
	m_mode_button->setText("Text");
	setStatus(QString());
	m_saved_text = taskText();
	updateTitle();
	updateButtons();
}

bool SchedulerTaskEditor::confirmDiscard() {
	if (!isModified()) {
		return true;
	}
	return QMessageBox::question(this, "Task modified",
		"The task in the editor has changes that are not sent to the scheduler or saved.\n\nDiscard them?") == QMessageBox::Yes;
}

void SchedulerTaskEditor::onNew() {
	if (confirmDiscard()) {
		newTask();
	}
}

void SchedulerTaskEditor::onToggleMode() {
	if (isVisualMode()) {
		showText(taskText());
		return;
	}
	QString text = m_text->toPlainText();
	QString error;
	if (!showVisual(text, &error)) {
		showText(text);
		QMessageBox::warning(this, "Visual editor",
			QString("The visual editor can not show this task without losing a part of it:\n\n%1\n\nKeep editing it as text.").arg(error));
		return;
	}
	setStatus(QString());
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
		setStatus(message.isEmpty() ? "Accepted by the scheduler" : message, "#40a040");
	} else {
		setStatus(message.isEmpty() ? "Rejected by the scheduler" : message, "#e04040");
	}
	updateTitle();
	updateButtons();
}

void SchedulerTaskEditor::setRunning(bool running) {
	if (m_running == running) {
		return;
	}
	m_running = running;
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
	m_apply_button->setVisible(!m_server_name.isEmpty());
	m_apply_button->setText(QString("Apply to '%1'").arg(m_server_name));
	m_apply_button->setEnabled(!m_running);
	m_upload_button->setEnabled(!m_running);
	m_urgent_button->setEnabled(!m_running);
	m_mode_button->setEnabled(!m_running);
	m_new_button->setEnabled(true);
	m_open_button->setEnabled(true);
}

void SchedulerTaskEditor::setStatus(const QString& text, const QString& color) {
	m_status->setText(text);
	m_status->setStyleSheet(color.isEmpty() ? QString() : QString("color: %1;").arg(color));
	m_status->setVisible(!text.isEmpty());
}
