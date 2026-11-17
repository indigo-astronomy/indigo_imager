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

#include "SchedulerWidget.h"
#include "SchedulerTaskEditor.h"
#include "SchedulerTask.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSplitter>
#include <QTabWidget>
#include <QTextStream>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QScrollBar>
#include <cstring>
#include <functional>

#define SCHEDULER_AGENT_PREFIX     "Scheduler Agent"
#define SCRIPTING_AGENT_PREFIX     "Scripting Agent"

#define P_ADD_TASK         "AGENT_SCHEDULER_ADD_TASK"
#define P_TASK_PREFIX      "AGENT_SCHEDULER_TASK_"
#define P_DELETE_TASK      "AGENT_SCHEDULER_DELETE_TASK"
#define P_URGENT_TASK      "AGENT_SCHEDULER_URGENT_TASK"
#define P_TASK_STATE       "AGENT_SCHEDULER_TASK_STATE"
#define P_TASK_NAME        "AGENT_SCHEDULER_TASK_NAME"
#define P_TASK_COMMAND     "AGENT_SCHEDULER_TASK_COMMAND"
#define P_CONTROL          "AGENT_SCHEDULER_CONTROL"
#define P_STATE            "AGENT_SCHEDULER_STATE"
#define P_CURRENT          "AGENT_SCHEDULER_CURRENT"
#define P_SETTINGS         "AGENT_SCHEDULER_SETTINGS"
#define P_HOOKS            "AGENT_SCHEDULER_HOOKS"
#define P_NIGHT            "AGENT_SCHEDULER_NIGHT"
#define P_DATA             "AGENT_SCHEDULER_DATA"
#define P_DATA_DIR         "AGENT_SCHEDULER_DATA_DIR"
#define P_HISTORY          "AGENT_SCHEDULER_HISTORY"
#define P_CLEAR_HISTORY    "AGENT_SCHEDULER_CLEAR_HISTORY"
#define P_PLAN             "AGENT_SCHEDULER_PLAN"

static const char *hook_items[][2] = {
	{ "STARTUP", "Startup" },
	{ "SHUTDOWN", "Shutdown" },
	{ "IDLE", "Idle" },
	{ "NIGHT_START", "Night start" },
	{ "NIGHT_END", "Night end" },
	{ "RESUME", "Resume" },
};

/* AGENT_SCHEDULER_TASK_<n>, a stored task, not AGENT_SCHEDULER_TASK_STATE and the like */
static bool isTaskProperty(const QString& name) {
	if (!name.startsWith(P_TASK_PREFIX)) return false;
	QString suffix = name.mid(strlen(P_TASK_PREFIX));
	bool ok;
	suffix.toInt(&ok);
	return ok;
}

static QIcon led(int state) {
	switch (state) {
		case SCHEDULER_OK: return QIcon(":resource/led-green.png");
		case SCHEDULER_BUSY: return QIcon(":resource/led-orange.png");
		case SCHEDULER_ALERT: return QIcon(":resource/led-red.png");
		default: return QIcon(":resource/led-grey.png");
	}
}

static QString ledHtml(int state) {
	switch (state) {
		case SCHEDULER_OK: return "<img src=\":resource/led-green.png\">";
		case SCHEDULER_BUSY: return "<img src=\":resource/led-orange.png\">";
		case SCHEDULER_ALERT: return "<img src=\":resource/led-red.png\">";
		default: return "<img src=\":resource/led-grey.png\">";
	}
}

static QTreeWidget *makeList(const QStringList& columns) {
	QTreeWidget *list = new QTreeWidget();
	list->setColumnCount(columns.size());
	list->setHeaderLabels(columns);
	list->setRootIsDecorated(false);
	list->setUniformRowHeights(true);
	list->setAlternatingRowColors(true);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
	list->header()->setStretchLastSection(true);
	list->header()->setSectionResizeMode(QHeaderView::Interactive);
	// the agent redefines a list when a label changes, the selection the user made is kept by name
	QObject::connect(list, &QTreeWidget::currentItemChanged, list, [list](QTreeWidgetItem *current) {
		if (current && !list->property("rebuilding").toBool()) {
			list->setProperty("selectedName", current->data(0, Qt::UserRole));
		}
	});
	return list;
}

/* rebuilds a list from the items of a list property, keeping the selected and the visible row */
static void fillList(QTreeWidget *list, const SchedulerProperty *property, int nameColumn, std::function<QStringList(const QStringList&)> columns) {
	QString selected = list->property("selectedName").toString();
	int scroll = list->verticalScrollBar() ? list->verticalScrollBar()->value() : 0;
	list->setProperty("rebuilding", true);
	list->clear();
	if (property == nullptr) {
		list->setProperty("rebuilding", false);
		return;
	}
	QTreeWidgetItem *current = nullptr;
	for (const SchedulerItem& item : property->items) {
		QStringList label = scheduler_label_columns(item.label);
		QTreeWidgetItem *row = new QTreeWidgetItem(columns(label));
		QString name = label.value(nameColumn);
		row->setData(0, Qt::UserRole, name);
		row->setData(0, Qt::UserRole + 1, item.name);
		row->setIcon(0, led(item.light));
		row->setToolTip(row->columnCount() - 1, row->text(row->columnCount() - 1));
		list->addTopLevelItem(row);
		if (!selected.isEmpty() && name == selected && current == nullptr) {
			current = row;
		}
	}
	if (current) {
		list->setCurrentItem(current);
	}
	if (list->verticalScrollBar()) {
		list->verticalScrollBar()->setValue(scroll);
	}
	list->setProperty("rebuilding", false);
}

SchedulerWidget::SchedulerWidget(QWidget *parent) : QWidget(parent) {
	setObjectName("SchedulerView");
	buildControlPanel();

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	// the editor has a page of its own, the header form and the steps need the room
	m_editor = new SchedulerTaskEditor();
	m_tabs = new QTabWidget();
	m_tabs->addTab(buildTasksPage(), "Tasks");
	m_tabs->addTab(m_editor, "Task editor");
	m_tabs->addTab(buildPlanPage(), "Night plan");
	m_tabs->addTab(buildHistoryPage(), "History");
	m_tabs->addTab(buildSettingsPage(), "Settings");
	layout->addWidget(m_tabs);

	connect(m_editor, &SchedulerTaskEditor::uploadTask, this, [this](const QString& text) {
		sendTask(P_ADD_TASK, text);
	});
	connect(m_editor, &SchedulerTaskEditor::applyTask, this, [this](const QString& name, const QString& text) {
		QString property = taskProperty(name);
		if (property.isEmpty()) {
			m_editor->setResult(false, QString("Task '%1' is not in the scheduler, add it instead").arg(name));
			return;
		}
		sendTask(property, text);
	});
	connect(m_editor, &SchedulerTaskEditor::submitUrgent, this, [this](const QString& text) {
		sendTask(P_URGENT_TASK, text);
	});

	clearAgent();
}

bool SchedulerWidget::isSchedulerDevice(const QString& device) {
	return device.startsWith(SCHEDULER_AGENT_PREFIX);
}

QString SchedulerWidget::selectedAgent() const {
	return m_agent_select->currentData().toString();
}

QString SchedulerWidget::scriptingAgent() const {
	QString agent = selectedAgent();
	if (agent.isEmpty()) {
		return QString();
	}
	// the scheduler uses the Scripting Agent of its own server, see SCHEDULER.md
	return SCRIPTING_AGENT_PREFIX + agent.mid(strlen(SCHEDULER_AGENT_PREFIX));
}

bool SchedulerWidget::wantsDevice(const QString& device) const {
	return isSchedulerDevice(device) || (!selectedAgent().isEmpty() && device == scriptingAgent());
}

bool SchedulerWidget::isRunning() const {
	auto it = m_properties.find(P_STATE);
	return it != m_properties.end() && it->state == SCHEDULER_BUSY;
}

QString SchedulerWidget::runningTask() const {
	return m_properties.value(P_CURRENT).text("CURRENT");
}

// MARK: Control panel

void SchedulerWidget::buildControlPanel() {
	m_control_panel = new QWidget();
	m_control_panel->setObjectName("SchedulerControl");
	QGridLayout *layout = new QGridLayout(m_control_panel);
	layout->setAlignment(Qt::AlignTop);
	int row = 0;

	m_agent_select = new QComboBox();
	m_agent_select->setToolTip("The Scheduler Agent; it runs its tasks in the Scripting Agent of its own server");
	layout->addWidget(m_agent_select, row++, 0, 1, 3);
	connect(m_agent_select, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SchedulerWidget::onAgentSelected);

	m_start_button = new QPushButton("Start");
	m_start_button->setIcon(QIcon(":resource/play.png"));
	m_start_button->setToolTip("Run the startup hook, then schedule the tasks");
	m_stop_button = new QPushButton("Stop");
	m_stop_button->setIcon(QIcon(":resource/pause.png"));
	m_stop_button->setToolTip("Let the running task finish, then run the shutdown hook and stop");
	m_abort_button = new QPushButton("Abort");
	m_abort_button->setIcon(QIcon(":resource/stop.png"));
	m_abort_button->setToolTip("Abort the running task at once, run the shutdown hook and stop");
	for (QPushButton *button : { m_start_button, m_stop_button, m_abort_button }) {
		button->setStyleSheet("min-width: 30px");
	}
	layout->addWidget(m_start_button, row, 0);
	layout->addWidget(m_stop_button, row, 1);
	layout->addWidget(m_abort_button, row++, 2);
	connect(m_start_button, &QPushButton::clicked, this, [this]() {
		send({ SchedulerRequest::switches1(selectedAgent(), P_CONTROL, "START") });
	});
	connect(m_stop_button, &QPushButton::clicked, this, [this]() {
		send({ SchedulerRequest::switches1(selectedAgent(), P_CONTROL, "STOP") });
	});
	connect(m_abort_button, &QPushButton::clicked, this, [this]() {
		if (QMessageBox::question(m_control_panel, "Abort scheduler", "Abort the running task and stop the scheduler?") == QMessageBox::Yes) {
			send({ SchedulerRequest::switches1(selectedAgent(), P_CONTROL, "ABORT") });
		}
	});

	layout->addItem(new QSpacerItem(1, 8), row++, 0);

	QLabel *title = new QLabel("Scheduler:");
	title->setStyleSheet("font-weight: bold;");
	layout->addWidget(title, row, 0);
	m_state_label = new QLabel();
	m_state_label->setObjectName("SchedulerState");
	layout->addWidget(m_state_label, row++, 1, 1, 2);

	m_current_label = new QLabel();
	m_current_label->setObjectName("SchedulerCurrent");
	m_current_label->setWordWrap(true);
	m_current_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(m_current_label, row++, 0, 1, 3);

	m_reason_label = new QLabel();
	m_reason_label->setObjectName("SchedulerReason");
	m_reason_label->setWordWrap(true);
	m_reason_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(m_reason_label, row++, 0, 1, 3);

	m_next_label = new QLabel();
	m_next_label->setWordWrap(true);
	m_next_label->setToolTip("The next run of the night plan, times in UTC");
	layout->addWidget(m_next_label, row++, 0, 1, 3);

	m_counters_label = new QLabel();
	m_counters_label->setWordWrap(true);
	layout->addWidget(m_counters_label, row++, 0, 1, 3);

	layout->addItem(new QSpacerItem(1, 8), row++, 0);

	title = new QLabel("Running task:");
	title->setStyleSheet("font-weight: bold;");
	layout->addWidget(title, row++, 0, 1, 3);
	m_sequence_progress = new QProgressBar();
	m_sequence_progress->setRange(0, 100);
	layout->addWidget(m_sequence_progress, row++, 0, 1, 3);
	m_sequence_label = new QLabel();
	m_sequence_label->setWordWrap(true);
	layout->addWidget(m_sequence_label, row++, 0, 1, 3);

	layout->addItem(new QSpacerItem(1, 8), row++, 0);
	QLabel *note = new QLabel("Times are UTC.");
	layout->addWidget(note, row++, 0, 1, 3);
}

// MARK: Pages

QWidget *SchedulerWidget::buildTasksPage() {
	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(page);
	layout->setContentsMargins(2, 2, 2, 2);

	m_tasks = makeList({ "Task", "State", "Priority", "Runs", "Retries", "Duration" });
	m_tasks->setObjectName("SchedulerTasks");
	m_tasks->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tasks->setToolTip("The tasks in list order; the list order only breaks ties of priority and sky score.\nDouble click a task to edit it.");
	connect(m_tasks, &QTreeWidget::itemActivated, this, &SchedulerWidget::onTaskActivated);
	connect(m_tasks, &QTreeWidget::customContextMenuRequested, this, &SchedulerWidget::onTasksContextMenu);
	layout->addWidget(m_tasks);

	QHBoxLayout *buttons = new QHBoxLayout();
	auto button = [&](const QString& text, const QString& tooltip, std::function<void()> action) {
		QToolButton *b = new QToolButton();
		b->setText(text);
		b->setToolTip(tooltip);
		connect(b, &QToolButton::clicked, this, action);
		buttons->addWidget(b);
		return b;
	};
	button("New", "Write a new task in the editor", [this]() {
		if (m_editor->confirmDiscard()) {
			m_editor->newTask();
			updateEditorRunning();
		}
		m_tabs->setCurrentWidget(m_editor);
	});
	button("Edit", "Open the selected task in the editor", [this]() { editTask(selectedTask()); });
	button("Reset", "Queue the task again with its run and retry counters cleared", [this]() { taskCommand(selectedTask(), "RESET"); });
	button("Enable", "Allow the task to be selected", [this]() { taskCommand(selectedTask(), "ENABLE"); });
	button("Disable", "Keep the task but never select it", [this]() { taskCommand(selectedTask(), "DISABLE"); });
	QToolButton *up = button("", "Move up; the order only breaks ties", [this]() { taskCommand(selectedTask(), "MOVE_UP"); });
	up->setIcon(QIcon(":resource/arrow-up.png"));
	QToolButton *down = button("", "Move down; the order only breaks ties", [this]() { taskCommand(selectedTask(), "MOVE_DOWN"); });
	down->setIcon(QIcon(":resource/arrow-down.png"));
	button("Delete", "Delete the task; a running task is aborted first", [this]() { deleteTask(selectedTask()); });
	buttons->addSpacing(12);
	button("Export...", "Save all stored tasks as .task files to a folder", [this]() { onExportTasks(); });
	button("Import...", "Add tasks from .task files", [this]() { onImportTasks(); });
	buttons->addStretch();
	button("Reset all", "Reset every task; the running task is left alone", [this]() {
		if (QMessageBox::question(this, "Reset all tasks", "Reset the run and retry counters of all tasks?") == QMessageBox::Yes) {
			send({ SchedulerRequest::switches1(selectedAgent(), P_TASK_COMMAND, "RESET_ALL") });
		}
	});
	button("Delete all", "Delete every task except the hook tasks; only while the scheduler is stopped", [this]() {
		if (QMessageBox::question(this, "Delete all tasks", "Delete all tasks except the tasks used as hooks?\n\nThis can not be undone; export the tasks first to keep them.") == QMessageBox::Yes) {
			send({ SchedulerRequest::switches1(selectedAgent(), P_TASK_COMMAND, "DELETE_ALL") });
		}
	});
	layout->addLayout(buttons);
	return page;
}

QWidget *SchedulerWidget::buildPlanPage() {
	m_plan = makeList({ "When (UTC)", "Task", "Run", "Priority", "State", "Reason" });
	m_plan->setObjectName("SchedulerPlan");
	m_plan->setToolTip("The forecast of the rest of the night: it assumes every run succeeds and takes its expected duration.\nDouble click a run to edit its task.");
	connect(m_plan, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
		editTask(item->data(0, Qt::UserRole).toString());
	});
	return m_plan;
}

QWidget *SchedulerWidget::buildHistoryPage() {
	QWidget *page = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout(page);
	layout->setContentsMargins(2, 2, 2, 2);
	m_history = makeList({ "When (UTC)", "Task", "Run", "Kind", "Result", "Reason" });
	m_history->setObjectName("SchedulerHistory");
	m_history->setToolTip("The finished runs, newest first");
	layout->addWidget(m_history);
	QHBoxLayout *buttons = new QHBoxLayout();
	buttons->addStretch();
	QToolButton *clear = new QToolButton();
	clear->setText("Clear history");
	clear->setToolTip("Remove all entries and reset the urgent task counters");
	connect(clear, &QToolButton::clicked, this, [this]() {
		if (QMessageBox::question(this, "Clear history", "Remove all entries of the run history?") == QMessageBox::Yes) {
			send({ SchedulerRequest::switches1(selectedAgent(), P_CLEAR_HISTORY, "CLEAR") });
		}
	});
	buttons->addWidget(clear);
	layout->addLayout(buttons);
	return page;
}

QWidget *SchedulerWidget::buildSettingsPage() {
	QWidget *content = new QWidget();
	QHBoxLayout *columns = new QHBoxLayout(content);
	auto edited = [this]() {
		m_settings_edited = true;
		m_settings_apply->setEnabled(true);
	};

	QVBoxLayout *left = new QVBoxLayout();
	QGroupBox *night_box = new QGroupBox("Night");
	QFormLayout *night_layout = new QFormLayout(night_box);
	m_night = new QComboBox();
	m_night->addItem("Civil (Sun below -6°)", "CIVIL");
	m_night->addItem("Nautical (Sun below -12°)", "NAUTICAL");
	m_night->addItem("Astronomical (Sun below -18°)", "ASTRONOMICAL");
	m_night->setToolTip("How dark it must be for the night of the night hooks; each task keeps its own @twilight");
	connect(m_night, QOverload<int>::of(&QComboBox::activated), this, edited);
	night_layout->addRow("Night limit:", m_night);
	left->addWidget(night_box);

	QGroupBox *hooks_box = new QGroupBox("Hooks");
	hooks_box->setToolTip("Stored tasks run at special moments; a hook task is never scheduled as an ordinary task");
	QFormLayout *hooks_layout = new QFormLayout(hooks_box);
	for (const auto& hook : hook_items) {
		QComboBox *box = new QComboBox();
		box->setEditable(true);
		box->setInsertPolicy(QComboBox::NoInsert);
		box->setMinimumContentsLength(16);
		connect(box, QOverload<int>::of(&QComboBox::activated), this, edited);
		connect(box->lineEdit(), &QLineEdit::textEdited, this, edited);
		box->setObjectName(QString("SchedulerHook_") + hook[0]);
		m_hooks[hook[0]] = box;
		hooks_layout->addRow(QString(hook[1]) + ":", box);
	}
	left->addWidget(hooks_box);

	QGroupBox *data_box = new QGroupBox("Observation data");
	QFormLayout *data_layout = new QFormLayout(data_box);
	for (const char *item : { "OBJECT", "FITS", "FOLDER" }) {
		QCheckBox *check = new QCheckBox();
		connect(check, &QCheckBox::clicked, this, edited);
		m_data[item] = check;
		data_layout->addRow(check);
	}
	m_data["OBJECT"]->setText("Task name as object name");
	m_data["FITS"]->setText("Task FITS keywords (SCHTASK, SCHRUN, SCHNIGHT...)");
	m_data["FOLDER"]->setText("Folder per task");
	m_data_dir = new QLineEdit();
	m_data_dir->setPlaceholderText("folder of the Imager Agent");
	m_data_dir->setToolTip("Parent folder of the task folders on the server; empty for the folder the Imager Agent had when the scheduler was started");
	connect(m_data_dir, &QLineEdit::textEdited, this, edited);
	data_layout->addRow("Base folder:", m_data_dir);
	left->addWidget(data_box);
	left->addStretch();
	columns->addLayout(left);

	QVBoxLayout *right = new QVBoxLayout();
	QGroupBox *settings_box = new QGroupBox("Scheduler settings");
	m_settings_layout = new QFormLayout(settings_box);
	right->addWidget(settings_box);

	QHBoxLayout *buttons = new QHBoxLayout();
	buttons->addStretch();
	QPushButton *revert = new QPushButton("Revert");
	revert->setToolTip("Show the settings of the scheduler again");
	connect(revert, &QPushButton::clicked, this, [this]() {
		m_settings_edited = false;
		m_settings_apply->setEnabled(false);
		updateSettings();
	});
	buttons->addWidget(revert);
	m_settings_apply = new QPushButton("Apply");
	m_settings_apply->setEnabled(false);
	connect(m_settings_apply, &QPushButton::clicked, this, [this]() {
		QString agent = selectedAgent();
		QList<SchedulerRequest> requests;
		requests.append(SchedulerRequest::switches1(agent, P_NIGHT, m_night->currentData().toString()));

		QStringList items, values;
		for (const auto& hook : hook_items) {
			items.append(hook[0]);
			values.append(m_hooks[hook[0]]->currentText().trimmed());
		}
		requests.append(SchedulerRequest::text(agent, P_HOOKS, items, values));

		SchedulerRequest data;
		data.type = SchedulerRequest::Switch;
		data.device = agent;
		data.property = P_DATA;
		for (auto it = m_data.begin(); it != m_data.end(); ++it) {
			data.items.append(it.key());
			data.switches.append(it.value()->isChecked());
		}
		requests.append(data);
		requests.append(SchedulerRequest::text(agent, P_DATA_DIR, "BASE", m_data_dir->text().trimmed()));

		if (!m_settings.isEmpty()) {
			SchedulerRequest settings;
			settings.type = SchedulerRequest::Number;
			settings.device = agent;
			settings.property = P_SETTINGS;
			for (auto it = m_settings.begin(); it != m_settings.end(); ++it) {
				settings.items.append(it.key());
				settings.numbers.append(it.value()->value());
			}
			requests.append(settings);
		}
		send(requests);
		m_settings_edited = false;
		m_settings_apply->setEnabled(false);
	});
	buttons->addWidget(m_settings_apply);
	right->addLayout(buttons);
	right->addStretch();
	columns->addLayout(right);

	QScrollArea *scroll = new QScrollArea();
	scroll->setWidget(content);
	scroll->setWidgetResizable(true);
	return scroll;
}

// MARK: Properties

void SchedulerWidget::onAgentSelected(int index) {
	Q_UNUSED(index);
	clearAgent();
	emit agentSelected(selectedAgent());
}

void SchedulerWidget::clearAgent() {
	m_properties.clear();
	m_sequence_state = SchedulerProperty();
	m_step_state = SchedulerProperty();
	m_loop_state = SchedulerProperty();
	m_pending_property.clear();
	m_messages.clear();
	updateState();
	updateTasks();
	updatePlan();
	updateHistory();
	m_settings_edited = false;
	updateSettings();
	handleScripting(SchedulerProperty());
}

void SchedulerWidget::noteMessage(const QString& device, const QString& property, const QString& message) {
	if (device != selectedAgent() || message.isEmpty()) {
		return;
	}
	if (!m_result_property.isEmpty() && property == m_result_property) {
		m_result_property.clear();
		m_editor->setResult(m_result_ok, message);
		return;
	}
	m_messages[property] = message;
}

void SchedulerWidget::defineProperty(const SchedulerProperty& property) {
	if (isSchedulerDevice(property.device) && m_agent_select->findData(property.device) < 0) {
		m_agent_select->addItem(property.device, property.device);
	}
	if (property.device == selectedAgent()) {
		handleProperty(property);
	} else if (property.device == scriptingAgent()) {
		handleScripting(property);
	}
}

void SchedulerWidget::updateProperty(const SchedulerProperty& property) {
	if (property.device == selectedAgent()) {
		handleProperty(property);
	} else if (property.device == scriptingAgent()) {
		handleScripting(property);
	}
}

void SchedulerWidget::deleteProperty(const QString& device, const QString& name) {
	if (device == selectedAgent()) {
		if (name.isEmpty()) {
			// the agent is gone, or the host clears it
			m_properties.clear();
		} else {
			m_properties.remove(name);
		}
		// a list property is deleted to be defined again with new labels, it is shown again then
		if (name.isEmpty()) {
			updateState();
			updateTasks();
			updatePlan();
			updateHistory();
		}
	} else if (device == scriptingAgent() && name.isEmpty()) {
		handleScripting(SchedulerProperty());
	}
	if (name.isEmpty() && isSchedulerDevice(device)) {
		int index = m_agent_select->findData(device);
		if (index >= 0) {
			m_agent_select->removeItem(index);
		}
	}
}

void SchedulerWidget::handleProperty(const SchedulerProperty& property) {
	m_properties[property.name] = property;
	const QString& name = property.name;
	if (name == P_STATE || name == P_CURRENT) {
		updateState();
		updateEditorRunning();
	} else if (name == P_TASK_STATE) {
		updateTasks();
	} else if (name == P_PLAN) {
		updatePlan();
		updateState();
	} else if (name == P_HISTORY) {
		updateHistory();
	} else if (name == P_SETTINGS || name == P_HOOKS || name == P_NIGHT || name == P_DATA || name == P_DATA_DIR) {
		updateSettings();
	} else if (isTaskProperty(name)) {
		// a stored task changed elsewhere, show it again unless it has local changes
		if (m_editor->serverName() == property.label && !m_editor->isModified() && name != m_pending_property) {
			QString text = property.text("TASK");
			if (text != m_editor->taskText()) {
				m_editor->loadTask(text, property.label);
				updateEditorRunning();
			}
		}
	}
	checkPendingResult(property);
}

void SchedulerWidget::checkPendingResult(const SchedulerProperty& property) {
	if (m_pending_property.isEmpty() || property.name != m_pending_property) {
		return;
	}
	if (property.state == SCHEDULER_OK || property.state == SCHEDULER_ALERT) {
		m_pending_property.clear();
		QString message = m_messages.take(property.name);
		m_result_ok = property.state == SCHEDULER_OK;
		m_result_property = message.isEmpty() ? property.name : QString();
		m_editor->setResult(m_result_ok, message);
		updateEditorRunning();
	}
}

void SchedulerWidget::handleScripting(const SchedulerProperty& property) {
	if (property.name == "SEQUENCE_STATE") {
		m_sequence_state = property;
	} else if (property.name == "SEQUENCE_STEP_STATE") {
		m_step_state = property;
	} else if (property.name == "LOOP_0") {
		m_loop_state = property;
	} else if (!property.name.isEmpty()) {
		return;
	}

	bool scheduler_task = !m_properties.value(P_CURRENT).text("CURRENT").isEmpty();
	if (m_sequence_state.name.isEmpty() || !scheduler_task) {
		m_sequence_progress->setValue(0);
		m_sequence_progress->setFormat("Idle");
		m_sequence_label->clear();
	} else {
		double progress = m_sequence_state.number("PROGRESS");
		double total = m_sequence_state.number("PROGRESS_TOTAL");
		int percent = total > 0 ? int(progress / total * 100 + 0.5) : 0;
		m_sequence_progress->setValue(percent);
		if (m_sequence_state.state == SCHEDULER_ALERT) {
			m_sequence_progress->setFormat("Failed (%p%)");
		} else {
			m_sequence_progress->setFormat("%p% complete");
		}
		double exposure = m_sequence_state.number("EXPOSURE");
		double exposure_total = m_sequence_state.number("EXPOSURE_TOTAL");
		double remaining = qMax(0.0, exposure_total - exposure);
		m_sequence_label->setText(QString("Step %1, exposure %2 of %3 remaining")
			.arg(int(m_sequence_state.number("STEP")) + 1)
			.arg(QString("%1:%2").arg(int(remaining / 3600)).arg(int(remaining / 60) % 60, 2, 10, QChar('0')))
			.arg(QString("%1:%2").arg(int(exposure_total / 3600)).arg(int(exposure_total / 60) % 60, 2, 10, QChar('0'))));
	}

	if (!m_step_state.name.isEmpty() && m_editor->isRunning()) {
		QVector<int> states;
		for (const SchedulerItem& item : m_step_state.items) {
			states.append(item.light);
		}
		int loop_step = m_loop_state.name.isEmpty() ? -1 : int(m_loop_state.number("STEP"));
		int loop_count = m_loop_state.name.isEmpty() ? 0 : int(m_loop_state.number("COUNT"));
		m_editor->showStepStates(states, loop_step, loop_count);
	}
}

// MARK: Views

void SchedulerWidget::updateState() {
	bool connected = !selectedAgent().isEmpty() && m_properties.contains(P_STATE);
	const SchedulerProperty state = m_properties.value(P_STATE);
	const SchedulerProperty current = m_properties.value(P_CURRENT);

	QString text;
	if (!connected) {
		text = "No scheduler agent";
	} else switch (state.state) {
		case SCHEDULER_BUSY: text = "Running"; break;
		case SCHEDULER_OK: text = "Stopped"; break;
		case SCHEDULER_ALERT: text = "Stopped, see the log"; break;
		default: text = "Not started"; break;
	}
	m_state_label->setText(ledHtml(connected ? state.state : SCHEDULER_IDLE) + " " + text);

	QString running = current.text("CURRENT");
	QString reason = current.text("REASON");
	m_current_label->setText(running.isEmpty() ? QString("Running: <i>none</i>") : QString("Running: <b>%1</b>").arg(running.toHtmlEscaped()));
	m_current_label->setVisible(connected);
	m_reason_label->setText(QString("Waiting: %1").arg(reason.toHtmlEscaped()));
	// the reason is why nothing runs now, it is stale while the scheduler is stopped
	m_reason_label->setVisible(connected && state.state == SCHEDULER_BUSY && running.isEmpty() && !reason.isEmpty());

	QString next;
	const SchedulerProperty plan = m_properties.value(P_PLAN);
	for (const SchedulerItem& item : plan.items) {
		QStringList columns = scheduler_label_columns(item.label);
		if (columns[4] == "PLANNED") {
			next = QString("Next: <b>%1</b> %2").arg(columns[1].toHtmlEscaped(), columns[0].toHtmlEscaped());
			break;
		}
	}
	m_next_label->setText(next);
	m_next_label->setVisible(connected && !next.isEmpty());

	m_counters_label->setText(QString("Done: %1 &nbsp; Failed: %2 &nbsp; Tasks: %3<br>Urgent done: %4 &nbsp; failed: %5")
		.arg(int(state.number("DONE"))).arg(int(state.number("FAILED"))).arg(int(state.number("TOTAL")))
		.arg(int(state.number("URGENT_DONE"))).arg(int(state.number("URGENT_FAILED"))));
	m_counters_label->setVisible(connected);

	bool busy = state.state == SCHEDULER_BUSY;
	m_start_button->setEnabled(connected && !busy);
	m_stop_button->setEnabled(connected && busy);
	m_abort_button->setEnabled(connected);
	setEnabled(true);
	for (int i = 0; i < m_tabs->count(); i++) {
		if (m_tabs->widget(i) != m_editor) {
			m_tabs->widget(i)->setEnabled(connected);
		}
	}
}

void SchedulerWidget::updateTasks() {
	const SchedulerProperty *property = m_properties.contains(P_TASK_STATE) ? &m_properties[P_TASK_STATE] : nullptr;
	fillList(m_tasks, property, 0, [](const QStringList& c) {
		return QStringList{ c[0], c[1], c[2], c[3], c[4], c[5] };
	});
	m_tasks->resizeColumnToContents(0);

	// the hook choices are the stored task names
	QStringList names = storedTaskNames();
	for (QComboBox *box : m_hooks) {
		QString text = box->currentText();
		box->blockSignals(true);
		box->clear();
		box->addItem("");
		box->addItems(names);
		box->setCurrentText(text);
		box->blockSignals(false);
	}
}

void SchedulerWidget::updatePlan() {
	const SchedulerProperty *property = m_properties.contains(P_PLAN) ? &m_properties[P_PLAN] : nullptr;
	fillList(m_plan, property, 1, [](const QStringList& c) {
		return QStringList{ c[0], c[1], c[2], c[3], c[4], c[5] };
	});
	m_plan->resizeColumnToContents(0);
	m_plan->resizeColumnToContents(1);
}

void SchedulerWidget::updateHistory() {
	const SchedulerProperty *property = m_properties.contains(P_HISTORY) ? &m_properties[P_HISTORY] : nullptr;
	fillList(m_history, property, 1, [](const QStringList& c) {
		return QStringList{ c[0], c[1], c[2], c[3], c[4], c[5] };
	});
	m_history->resizeColumnToContents(0);
	m_history->resizeColumnToContents(1);
}

void SchedulerWidget::updateSettings() {
	if (m_settings_edited) {
		return;
	}
	const SchedulerProperty night = m_properties.value(P_NIGHT);
	for (const SchedulerItem& item : night.items) {
		if (item.on) {
			int index = m_night->findData(item.name);
			if (index >= 0) m_night->setCurrentIndex(index);
		}
	}

	const SchedulerProperty hooks = m_properties.value(P_HOOKS);
	for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
		it.value()->setCurrentText(hooks.text(it.key()));
	}

	const SchedulerProperty data = m_properties.value(P_DATA);
	for (auto it = m_data.begin(); it != m_data.end(); ++it) {
		it.value()->setChecked(data.on(it.key()));
	}
	m_data_dir->setText(m_properties.value(P_DATA_DIR).text("BASE"));

	const SchedulerProperty settings = m_properties.value(P_SETTINGS);
	if (settings.items.size() != m_settings.size()) {
		while (m_settings_layout->rowCount() > 0) {
			m_settings_layout->removeRow(0);
		}
		m_settings.clear();
		for (const SchedulerItem& item : settings.items) {
			QDoubleSpinBox *spin = new QDoubleSpinBox();
			spin->setRange(item.min, item.max);
			spin->setSingleStep(item.step > 0 ? item.step : 1);
			spin->setDecimals(item.step > 0 && item.step < 1 ? 1 : 0);
			spin->setToolTip(item.name);
			connect(spin, &QDoubleSpinBox::editingFinished, this, [this]() {
				m_settings_edited = true;
				m_settings_apply->setEnabled(true);
			});
			m_settings[item.name] = spin;
			m_settings_layout->addRow(item.label + ":", spin);
		}
	}
	for (const SchedulerItem& item : settings.items) {
		if (m_settings.contains(item.name)) {
			m_settings[item.name]->blockSignals(true);
			m_settings[item.name]->setValue(item.number);
			m_settings[item.name]->blockSignals(false);
		}
	}
	m_settings_apply->setEnabled(false);
}

void SchedulerWidget::updateEditorRunning() {
	QString running = m_properties.value(P_CURRENT).text("CURRENT");
	m_editor->setRunning(!running.isEmpty() && running == m_editor->serverName());
}

// MARK: Tasks

QStringList SchedulerWidget::storedTaskNames() const {
	QStringList names;
	for (const SchedulerProperty& property : m_properties) {
		if (isTaskProperty(property.name)) {
			names.append(property.label);
		}
	}
	names.sort(Qt::CaseInsensitive);
	return names;
}

QString SchedulerWidget::taskProperty(const QString& taskName) const {
	for (const SchedulerProperty& property : m_properties) {
		if (isTaskProperty(property.name) && property.label.compare(taskName, Qt::CaseInsensitive) == 0) {
			return property.name;
		}
	}
	return QString();
}

QString SchedulerWidget::taskText(const QString& taskName) const {
	QString property = taskProperty(taskName);
	return property.isEmpty() ? QString() : m_properties.value(property).text("TASK");
}

QString SchedulerWidget::selectedTask() const {
	QTreeWidgetItem *item = m_tasks->currentItem();
	return item ? item->data(0, Qt::UserRole).toString() : m_tasks->property("selectedName").toString();
}

void SchedulerWidget::onTaskActivated(QTreeWidgetItem *item, int column) {
	Q_UNUSED(column);
	editTask(item->data(0, Qt::UserRole).toString());
}

void SchedulerWidget::editTask(const QString& name) {
	if (name.isEmpty()) {
		return;
	}
	if (taskProperty(name).isEmpty()) {
		QMessageBox::information(this, "Edit task", QString("'%1' is a one-off urgent task, it is not stored and can not be edited.").arg(name));
		return;
	}
	if (m_editor->serverName() == name && !m_editor->isModified()) {
		m_tabs->setCurrentWidget(m_editor);
		return;
	}
	if (!m_editor->confirmDiscard()) {
		return;
	}
	m_tabs->setCurrentWidget(m_editor);
	m_editor->loadTask(taskText(name), m_properties.value(taskProperty(name)).label);
	updateEditorRunning();
	handleScripting(SchedulerProperty());
}

void SchedulerWidget::taskCommand(const QString& name, const QString& command) {
	if (name.isEmpty()) {
		return;
	}
	QString agent = selectedAgent();
	// the name and the command are sent in this order, see SCHEDULER.md
	send({
		SchedulerRequest::text(agent, P_TASK_NAME, "NAME", name),
		SchedulerRequest::switches1(agent, P_TASK_COMMAND, command)
	});
}

void SchedulerWidget::deleteTask(const QString& name) {
	if (name.isEmpty()) {
		return;
	}
	QString question = QString("Delete the task '%1'?").arg(name);
	if (name == m_properties.value(P_CURRENT).text("CURRENT")) {
		question += "\n\nThe task is running, it will be aborted.";
	}
	if (QMessageBox::question(this, "Delete task", question) != QMessageBox::Yes) {
		return;
	}
	send({ SchedulerRequest::text(selectedAgent(), P_DELETE_TASK, "NAME", name) });
}

void SchedulerWidget::onTasksContextMenu(const QPoint& pos) {
	QTreeWidgetItem *item = m_tasks->itemAt(pos);
	if (!item) {
		return;
	}
	m_tasks->setCurrentItem(item);
	QString name = item->data(0, Qt::UserRole).toString();
	bool stored = !taskProperty(name).isEmpty();

	QMenu menu(this);
	menu.addAction("Edit", this, [this, name]() { editTask(name); })->setEnabled(stored);
	menu.addAction("Save to file...", this, [this, name]() { saveTaskToFile(name); })->setEnabled(stored);
	menu.addSeparator();
	menu.addAction("Reset", this, [this, name]() { taskCommand(name, "RESET"); });
	menu.addAction("Enable", this, [this, name]() { taskCommand(name, "ENABLE"); });
	menu.addAction("Disable", this, [this, name]() { taskCommand(name, "DISABLE"); });
	menu.addAction("Move up", this, [this, name]() { taskCommand(name, "MOVE_UP"); });
	menu.addAction("Move down", this, [this, name]() { taskCommand(name, "MOVE_DOWN"); });
	menu.addSeparator();
	QMenu *hooks = menu.addMenu("Use as hook");
	hooks->setEnabled(stored);
	const SchedulerProperty hook_property = m_properties.value(P_HOOKS);
	for (const auto& hook : hook_items) {
		QString item_name = hook[0];
		bool is_hook = hook_property.text(item_name) == name;
		QAction *action = hooks->addAction(hook[1], this, [this, name, item_name, is_hook]() {
			send({ SchedulerRequest::text(selectedAgent(), P_HOOKS, item_name, is_hook ? QString() : name) });
		});
		action->setCheckable(true);
		action->setChecked(is_hook);
	}
	menu.addSeparator();
	menu.addAction("Delete", this, [this, name]() { deleteTask(name); });
	menu.exec(m_tasks->viewport()->mapToGlobal(pos));
}

static QString fileNameFor(const QString& taskName) {
	QString name = taskName;
	name.replace(QRegularExpression("[^A-Za-z0-9._ +-]"), "_");
	return name.isEmpty() ? QString("untitled") : name;
}

void SchedulerWidget::saveTaskToFile(const QString& name) {
	QString text = taskText(name);
	QString file_name = QFileDialog::getSaveFileName(this, "Save Task", QDir(QDir::homePath()).filePath(fileNameFor(name) + ".task"),
		"Task files (*.task);;All files (*)");
	if (file_name.isEmpty()) {
		return;
	}
	QFile file(file_name);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, "Save Task", QString("Can not save %1: %2").arg(file_name, file.errorString()));
		return;
	}
	QTextStream(&file) << text;
}

void SchedulerWidget::onExportTasks() {
	QStringList names = storedTaskNames();
	if (names.isEmpty()) {
		QMessageBox::information(this, "Export tasks", "There are no stored tasks.");
		return;
	}
	QString dir = QFileDialog::getExistingDirectory(this, "Export tasks to folder", QDir::homePath());
	if (dir.isEmpty()) {
		return;
	}
	QStringList existing;
	for (const QString& name : names) {
		if (QFileInfo::exists(QDir(dir).filePath(fileNameFor(name) + ".task"))) {
			existing.append(fileNameFor(name) + ".task");
		}
	}
	if (!existing.isEmpty() && QMessageBox::question(this, "Export tasks",
		QString("These files exist and will be replaced:\n\n%1\n\nContinue?").arg(existing.join("\n"))) != QMessageBox::Yes) {
		return;
	}
	QStringList failed;
	for (const QString& name : names) {
		QFile file(QDir(dir).filePath(fileNameFor(name) + ".task"));
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
			failed.append(file.fileName());
			continue;
		}
		QTextStream(&file) << taskText(name);
	}
	if (!failed.isEmpty()) {
		QMessageBox::critical(this, "Export tasks", QString("Can not write:\n\n%1").arg(failed.join("\n")));
	}
}

void SchedulerWidget::onImportTasks() {
	QStringList files = QFileDialog::getOpenFileNames(this, "Import tasks", QDir::homePath(), "Task files (*.task *.js);;All files (*)");
	if (files.isEmpty()) {
		return;
	}
	QList<SchedulerRequest> requests;
	for (const QString& file_name : files) {
		QFile file(file_name);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QMessageBox::critical(this, "Import tasks", QString("Can not open %1: %2").arg(file_name, file.errorString()));
			return;
		}
		requests.append(SchedulerRequest::text(selectedAgent(), P_ADD_TASK, "TASK", QTextStream(&file).readAll()));
	}
	// the scheduler answers every upload with a message in the log
	send(requests);
}

void SchedulerWidget::sendTask(const QString& property, const QString& text) {
	if (selectedAgent().isEmpty()) {
		m_editor->setResult(false, "No scheduler agent");
		return;
	}
	m_pending_property = property;
	m_result_property.clear();
	m_messages.remove(property);
	send({ SchedulerRequest::text(selectedAgent(), property, "TASK", text) });
}

void SchedulerWidget::send(const QList<SchedulerRequest>& requests) {
	if (selectedAgent().isEmpty()) {
		return;
	}
	emit sendRequests(requests);
}
