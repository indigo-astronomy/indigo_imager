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
#include "SchedulerTaskForm.h"
#include "SchedulerSkyChart.h"
#include "SexagesimalConverter.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QDateTime>
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
#include <QPainter>
#include <QStyledItemDelegate>
#include <cstring>
#include <functional>

#define SCHEDULER_AGENT_PREFIX     "Scheduler Agent"
#define SCRIPTING_AGENT_PREFIX     "Scripting Agent"
#define IMAGER_AGENT_PREFIX        "Imager Agent"
#define MOUNT_AGENT_PREFIX         "Mount Agent"

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
#define P_STRATEGY         "AGENT_SCHEDULER_STRATEGY"
#define P_DATA             "AGENT_SCHEDULER_DATA"
#define P_DATA_DIR         "AGENT_SCHEDULER_DATA_DIR"
#define P_HISTORY          "AGENT_SCHEDULER_HISTORY"
#define P_CLEAR_HISTORY    "AGENT_SCHEDULER_CLEAR_HISTORY"
#define P_PLAN             "AGENT_SCHEDULER_PLAN"
#define P_SAFETY_MISSING   "AGENT_SCHEDULER_SAFETY_MISSING"

static const char *hook_items[][2] = {
	{ "STARTUP", "Startup" },
	{ "SHUTDOWN", "Shutdown" },
	{ "NIGHT_START", "Night start" },
	{ "NIGHT_END", "Night end" },
	{ "IDLE", "Idle" },
	{ "RESUME", "Resume" },
	{ "UNSAFE", "Unsafe" },
	{ "SAFE", "Safe" },
};

/* a hook the agent has, all of them before its hooks arrive; an older agent has fewer */
static bool hasHook(const SchedulerProperty& hooks, const char *name) {
	return hooks.items.isEmpty() || hooks.item(name) != nullptr;
}

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

/* a page of the tabs as the pages of the other tabs of Ain: a styled panel with the default layout margins */
static QFrame *makePage(QBoxLayout **layout) {
	QFrame *page = new QFrame();
	page->setFrameShape(QFrame::StyledPanel);
	page->setContentsMargins(0, 0, 0, 0);
	*layout = new QVBoxLayout(page);
	return page;
}

/* The choices of a switch property as add_items_to_combobox() makes them for the other tabs:
   the labels of the items with their names as the data, the item that is on selected. The
   items are added again only when they change, an open popup stays as it is. */
static void setComboItems(QComboBox *combo, const SchedulerProperty& property) {
	bool same = combo->count() == property.items.size();
	for (int i = 0; same && i < combo->count(); i++) {
		same = combo->itemData(i).toString() == property.items[i].name && combo->itemText(i) == property.items[i].label;
	}
	combo->blockSignals(true);
	if (!same) {
		combo->clear();
		for (const SchedulerItem& item : property.items) {
			combo->addItem(item.label, item.name);
		}
	}
	for (int i = 0; i < property.items.size(); i++) {
		if (property.items[i].on) combo->setCurrentIndex(i);
	}
	combo->blockSignals(false);
	combo->setEnabled(!property.items.isEmpty());
}

/* The rows of the task list, the night plan and the history drawn as cards like the properties of
   the INDIGO Control Panel: a panel tinted by the state light with the light, the name in bold and
   the time or state on the right, the other columns below it and the reason on a line of its own.
   The columns stay in the model, hidden, so the lists keep their selection, their activation and
   their context menu. */
class SchedulerCardDelegate : public QStyledItemDelegate {
public:
	enum Kind { TaskCard, PlanCard, HistoryCard };

	SchedulerCardDelegate(Kind kind, QObject *parent) : QStyledItemDelegate(parent), m_kind(kind) {
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		QFontMetrics metrics(option.font);
		int lines = reason(index).isEmpty() ? 2 : 3;
		return QSize(200, 2 * MARGIN_Y + 2 * PADDING_Y + lines * metrics.height() + (lines - 1) * LINE_GAP);
	}

	void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
		painter->save();
		painter->setRenderHint(QPainter::Antialiasing, true);
		painter->setFont(option.font);
		// over the row background and the selection the view draws
		painter->fillRect(option.rect, option.palette.color(QPalette::Base));

		bool enabled = option.state & QStyle::State_Enabled;
		bool selected = option.state & QStyle::State_Selected;
		bool hover = option.state & QStyle::State_MouseOver;
		int light = index.data(Qt::UserRole + 2).toInt();
		QColor background = cardColor(light);
		QColor border = selected ? option.palette.color(QPalette::Highlight) : background.lighter(hover ? 190 : 140);

		QRectF card = QRectF(option.rect).adjusted(MARGIN_X + 0.5, MARGIN_Y + 0.5, -MARGIN_X - 0.5, -MARGIN_Y - 0.5);
		painter->setPen(QPen(border, selected ? 2 : 1));
		painter->setBrush(background);
		painter->drawRoundedRect(card, 3, 3);

		QFontMetrics metrics(option.font);
		QColor text = option.palette.color(enabled ? QPalette::Active : QPalette::Disabled, QPalette::Text);
		QColor dim = text;
		dim.setAlpha(150);
		QRect content = card.toRect().adjusted(PADDING_X, PADDING_Y, -PADDING_X, -PADDING_Y);
		int line = metrics.height() + LINE_GAP;

		// first line: the light, the name and on the right the time or the state
		QRect first(content.left(), content.top(), content.width(), metrics.height());
		QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
		int led_size = 10;
		icon.paint(painter, QRect(first.left(), first.center().y() - led_size / 2 + 1, led_size, led_size));
		first.setLeft(first.left() + led_size + 6);
		QString right = column(index, m_kind == TaskCard ? 1 : 0);
		if (m_kind != TaskCard && !right.isEmpty()) right += " UTC";
		int right_width = right.isEmpty() ? 0 : qMin(metrics.horizontalAdvance(right) + 2, first.width() / 2);
		painter->setPen(enabled ? stateTextColor(light, text) : text);
		painter->drawText(first, Qt::AlignRight | Qt::AlignVCenter, metrics.elidedText(right, Qt::ElideRight, right_width));
		QFont bold = option.font;
		bold.setBold(true);
		QFontMetrics bold_metrics(bold);
		painter->setFont(bold);
		painter->setPen(text);
		QRect name_rect = first.adjusted(0, 0, -(right_width + (right_width ? 12 : 0)), 0);
		painter->drawText(name_rect, Qt::AlignLeft | Qt::AlignVCenter, bold_metrics.elidedText(title(index), Qt::ElideRight, name_rect.width()));
		painter->setFont(option.font);

		// second line: the other columns, a label only where the value does not say what it is
		QRect second(content.left() + led_size + 6, content.top() + line, content.width() - led_size - 6, metrics.height());
		int x = second.left();
		bool first_part = true;
		for (const auto& part : details(index)) {
			if (part.second.isEmpty()) continue;
			QString separator = first_part ? QString() : QString("  ·  ");
			QString label = part.first.isEmpty() ? QString() : part.first + " ";
			int width = second.right() - x;
			if (width <= 0) break;
			painter->setPen(dim);
			QString head = metrics.elidedText(separator + label, Qt::ElideRight, width);
			painter->drawText(QRect(x, second.top(), width, second.height()), Qt::AlignLeft | Qt::AlignVCenter, head);
			x += metrics.horizontalAdvance(head);
			width = second.right() - x;
			if (width <= 0) break;
			painter->setPen(text);
			QString value = metrics.elidedText(part.second, Qt::ElideRight, width);
			painter->drawText(QRect(x, second.top(), width, second.height()), Qt::AlignLeft | Qt::AlignVCenter, value);
			x += metrics.horizontalAdvance(value);
			first_part = false;
		}

		// third line: the reason, the tooltip has all of it
		QString why = reason(index);
		if (!why.isEmpty()) {
			QRect third(second.left(), content.top() + 2 * line, second.width(), metrics.height());
			QFont italic = option.font;
			italic.setItalic(true);
			painter->setFont(italic);
			painter->setPen(dim);
			painter->drawText(third, Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(italic).elidedText(why, Qt::ElideRight, third.width()));
		}
		painter->restore();
	}

private:
	static const int MARGIN_X = 4;
	static const int MARGIN_Y = 2;
	static const int PADDING_X = 6;
	static const int PADDING_Y = 4;
	static const int LINE_GAP = 1;

	/* the colors of the property panels of the INDIGO Control Panel */
	static QColor cardColor(int light) {
		switch (light) {
			case SCHEDULER_OK: return QColor(0x20, 0x32, 0x20);
			case SCHEDULER_BUSY: return QColor(0x35, 0x35, 0x20);
			case SCHEDULER_ALERT: return QColor(0x35, 0x22, 0x22);
			default: return QColor(0x27, 0x27, 0x27);
		}
	}

	static QColor stateTextColor(int light, const QColor& text) {
		switch (light) {
			case SCHEDULER_OK: return QColor(0x90, 0xd0, 0x90);
			case SCHEDULER_BUSY: return QColor(0xe0, 0xd0, 0x70);
			case SCHEDULER_ALERT: return QColor(0xe8, 0x88, 0x88);
			default: return text;
		}
	}

	static QString column(const QModelIndex& index, int column) {
		return index.sibling(index.row(), column).data(Qt::DisplayRole).toString();
	}

	QString title(const QModelIndex& index) const {
		if (m_kind == TaskCard) return column(index, 0);
		// a plan line without a task, like the night hooks without a night, is named by its state
		QString name = column(index, 1);
		return name.isEmpty() ? column(index, 4) : name;
	}

	QList<QPair<QString, QString>> details(const QModelIndex& index) const {
		switch (m_kind) {
			case TaskCard:
				// "run 1/3", "retries left 2" and "1h30 estimate" say what they are
				return { { "Priority", column(index, 2) }, { "", column(index, 3) }, { "", column(index, 4) }, { "", column(index, 5) } };
			case PlanCard:
				return { { "", column(index, 2) }, { "Priority", column(index, 3) }, { "", column(index, 1).isEmpty() ? QString() : column(index, 4) } };
			default:
				return { { "", column(index, 2) }, { "", column(index, 3) }, { "", column(index, 4) } };
		}
	}

	QString reason(const QModelIndex& index) const {
		return m_kind == TaskCard ? QString() : column(index, 5);
	}

	Kind m_kind;
};

static QTreeWidget *makeList(const QStringList& columns, SchedulerCardDelegate::Kind kind) {
	QTreeWidget *list = new QTreeWidget();
	// the style takes the frame of a list in a tab away, Ain draws it as for the object list
	list->setStyleSheet("QTreeWidget {border: 1px solid #404040;}");
	list->setColumnCount(columns.size());
	list->setHeaderLabels(columns);
	// one card per row in the first column, the others are only data for the card and the tooltip
	list->setHeaderHidden(true);
	for (int i = 1; i < columns.size(); i++) {
		list->setColumnHidden(i, true);
	}
	list->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	list->setItemDelegate(new SchedulerCardDelegate(kind, list));
	list->setRootIsDecorated(false);
	list->setIndentation(0);
	list->setUniformRowHeights(false);
	list->setAlternatingRowColors(false);
	list->setMouseTracking(true);
	list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	list->setSelectionMode(QAbstractItemView::SingleSelection);
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
		row->setData(0, Qt::UserRole + 2, item.light);
		row->setIcon(0, led(item.light));
		// the card cuts long texts, the tooltip has every column with its title
		QStringList tooltip;
		for (int c = 0; c < row->columnCount(); c++) {
			if (!row->text(c).isEmpty()) {
				tooltip << QString("<b>%1:</b> %2").arg(list->headerItem()->text(c).toHtmlEscaped(), row->text(c).toHtmlEscaped());
			}
		}
		row->setToolTip(0, tooltip.join("<br>"));
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
	m_tabs->setObjectName("SchedulerTabs");
	m_tabs->addTab(buildTasksPage(), "Tasks");
	m_tabs->addTab(m_editor, "Task editor");
	m_tabs->addTab(buildPlanPage(), "Night plan");
	m_tabs->addTab(buildHistoryPage(), "History");
	m_tabs->addTab(buildHooksPage(), "Hooks");
	m_tabs->addTab(buildSettingsPage(), "Settings");
	layout->addWidget(m_tabs);

	connect(m_editor, &SchedulerTaskEditor::applyTask, this, [this](const QString& name, const QString& text) {
		// the stored task the editor shows, else the one with the name of the task, else a new one
		QString property = name.isEmpty() ? QString() : taskProperty(name);
		if (property.isEmpty()) {
			QString task_name = SchedulerTask::fromText(text).name().trimmed();
			if (!task_name.isEmpty()) property = taskProperty(task_name);
		}
		sendTask(property.isEmpty() ? QString(P_ADD_TASK) : property, text);
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

QString SchedulerWidget::imagerAgent() const {
	QString agent = selectedAgent();
	if (agent.isEmpty()) {
		return QString();
	}
	// the sequences of the tasks use the Imager Agent of the server by default
	return IMAGER_AGENT_PREFIX + agent.mid(strlen(SCHEDULER_AGENT_PREFIX));
}

QString SchedulerWidget::mountAgent() const {
	QString agent = selectedAgent();
	if (agent.isEmpty()) {
		return QString();
	}
	return MOUNT_AGENT_PREFIX + agent.mid(strlen(SCHEDULER_AGENT_PREFIX));
}

bool SchedulerWidget::wantsDevice(const QString& device) const {
	return isSchedulerDevice(device) || (!selectedAgent().isEmpty() && device == scriptingAgent());
}

bool SchedulerWidget::wantsProperty(const QString& device, const QString& name) const {
	if (wantsDevice(device)) {
		return true;
	}
	if (selectedAgent().isEmpty()) {
		return false;
	}
	if (device == mountAgent()) {
		return name.isEmpty() || name == "GEOGRAPHIC_COORDINATES";
	}
	return device == imagerAgent() &&
		(name.isEmpty() || name == "AGENT_START_PROCESS" || name == "AGENT_IMAGER_STATS" || name == "AGENT_IMAGER_BATCH");
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

	title = new QLabel("Task progress:");
	title->setStyleSheet("font-weight: bold;");
	layout->addWidget(title, row, 0);
	m_task_status = new QLabel();
	m_task_status->setObjectName("SchedulerTaskStatus");
	layout->addWidget(m_task_status, row++, 1, 1, 2);
	m_exposure_progress = new QProgressBar();
	m_exposure_progress->setObjectName("SchedulerExposure");
	layout->addWidget(m_exposure_progress, row++, 0, 1, 3);
	m_batch_progress = new QProgressBar();
	m_batch_progress->setObjectName("SchedulerBatch");
	layout->addWidget(m_batch_progress, row++, 0, 1, 3);
	m_sequence_progress = new QProgressBar();
	m_sequence_progress->setObjectName("SchedulerProgress");
	layout->addWidget(m_sequence_progress, row++, 0, 1, 3);
	m_sequence_label = new QLabel();
	m_sequence_label->setObjectName("SchedulerSequence");
	m_sequence_label->setWordWrap(true);
	layout->addWidget(m_sequence_label, row++, 0, 1, 3);
	resetProgress();

	layout->addItem(new QSpacerItem(1, 8), row++, 0);
	title = new QLabel("Tonight:");
	title->setStyleSheet("font-weight: bold;");
	layout->addWidget(title, row++, 0, 1, 3);
	m_chart = new SchedulerSkyChart();
	m_chart->setToolTip("The altitude of the targets of the tasks over the night");
	layout->addWidget(m_chart, row, 0, 1, 3);
	layout->setRowStretch(row++, 1);
	QLabel *note = new QLabel("Times are UTC. The thick parts are the runs of the night plan, the dashed curve is the Moon, the shading is the twilight.");
	note->setWordWrap(true);
	layout->addWidget(note, row++, 0, 1, 3);
}

// MARK: Pages

QWidget *SchedulerWidget::buildTasksPage() {
	QBoxLayout *layout;
	QFrame *page = makePage(&layout);

	m_tasks = makeList({ "Task", "State", "Priority", "Runs", "Retries", "Duration" }, SchedulerCardDelegate::TaskCard);
	m_tasks->setObjectName("SchedulerTasks");
	m_tasks->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tasks->setToolTip("The tasks in list order; the list order only breaks ties of priority and sky score.\nDouble click a task to edit it.");
	connect(m_tasks, &QTreeWidget::itemActivated, this, &SchedulerWidget::onTaskActivated);
	m_chart->setAccentSource(m_tasks);
	connect(m_tasks, &QTreeWidget::currentItemChanged, this, [this]() { if (!m_tasks->property("rebuilding").toBool()) updateChart(); });
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
	QToolButton *up = button("", "Move up; the order only breaks ties", [this]() { taskCommand(selectedTask(), "MOVE_UP"); });
	up->setIcon(QIcon(":resource/arrow-up.png"));
	QToolButton *down = button("", "Move down; the order only breaks ties", [this]() { taskCommand(selectedTask(), "MOVE_DOWN"); });
	down->setIcon(QIcon(":resource/arrow-down.png"));
	QToolButton *remove = button("", "Delete the task; a running task is aborted first", [this]() { deleteTask(selectedTask()); });
	remove->setIcon(QIcon(":resource/delete.png"));
	buttons->addSpacing(12);
	QToolButton *import = button("", "Import tasks from .task files", [this]() { onImportTasks(); });
	import->setIcon(QIcon(":resource/folder.png"));
	QToolButton *exporting = button("", "Export all stored tasks as .task files to a folder", [this]() { onExportTasks(); });
	exporting->setIcon(QIcon(":resource/save.png"));
	buttons->addStretch();
	button("Reset all", "Reset every task; the running task is left alone", [this]() {
		if (QMessageBox::question(this, "Reset all tasks", "Reset the run and retry counters of all tasks?") == QMessageBox::Yes) {
			send({ SchedulerRequest::switches1(selectedAgent(), P_TASK_COMMAND, "RESET_ALL") });
		}
	});
	QToolButton *remove_all = button("All", "Delete every task except the hook tasks; only while the scheduler is stopped", [this]() {
		if (QMessageBox::question(this, "Delete all tasks", "Delete all tasks except the tasks used as hooks?\n\nThis can not be undone; export the tasks first to keep them.") == QMessageBox::Yes) {
			send({ SchedulerRequest::switches1(selectedAgent(), P_TASK_COMMAND, "DELETE_ALL") });
		}
	});
	remove_all->setIcon(QIcon(":resource/delete.png"));
	remove_all->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	layout->addLayout(buttons);
	return page;
}

QWidget *SchedulerWidget::buildPlanPage() {
	m_plan = makeList({ "When (UTC)", "Task", "Run", "Priority", "State", "Reason" }, SchedulerCardDelegate::PlanCard);
	m_plan->setObjectName("SchedulerPlan");
	m_plan->setToolTip("The forecast of the rest of the night: it assumes every run succeeds and takes its expected duration.\nDouble click a run to edit its task.");
	connect(m_plan, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
		editTask(item->data(0, Qt::UserRole).toString());
	});
	QBoxLayout *layout;
	QFrame *page = makePage(&layout);
	layout->addWidget(m_plan);
	return page;
}

QWidget *SchedulerWidget::buildHistoryPage() {
	QBoxLayout *layout;
	QFrame *page = makePage(&layout);
	m_history = makeList({ "When (UTC)", "Task", "Run", "Kind", "Result", "Reason" }, SchedulerCardDelegate::HistoryCard);
	m_history->setObjectName("SchedulerHistory");
	m_history->setToolTip("The finished runs, newest first.\nDouble click a run to edit its task.");
	connect(m_history, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
		editTask(item->data(0, Qt::UserRole).toString());
	});
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

/* the items of AGENT_SCHEDULER_SETTINGS that have a place on the page */
static const char *setting_items[][3] = {
	{ "NIGHT_START_OFFSET", "Night start offset (min)", "Shift of the night start for the night hooks; negative is earlier" },
	{ "NIGHT_END_OFFSET", "Night end offset (min)", "Shift of the night end for the night hooks; negative is earlier" },
	{ "SETTING_WEIGHT", "Setting soonest weight", "How strongly tasks whose constraints end soon are preferred within a priority class:\nup to 100 × weight points, 0 lets the level decide" },
	{ "ALTITUDE_WEIGHT", "Altitude weight", "How strongly targets near their own culmination are preferred within a priority class:\nup to 100 × weight points, 0 lets the level decide" },
	{ "VISIBILITY_FLIP_PENALTY", "Best visibility: meridian flip penalty (min)", "Best visibility only: the minutes of full-quality imaging given up to avoid one meridian flip,\n0 switches the cost off; ignored while the flip is disabled" },
	{ "SAFE_DELAY", "Safe delay (min)", "How long the conditions must be safe without interruption\nbefore the unsafe hold ends or a waiting start begins" },
	{ "IDLE_HOOK_DELAY", "Idle hook delay (min)", "Time without an eligible task before the Idle hook runs" },
	{ "IDLE_CHECK_INTERVAL", "Check interval (s)", "How often the scheduler checks the constraints while it waits,\nand checks for urgent tasks while a task runs" },
	{ "START_TIMEOUT", "Start timeout (s)", "Time allowed for a started task's sequence to begin" },
	{ "STOP_TIMEOUT", "Stop timeout (s)", "Time allowed for an aborted sequence and its agents to stop" },
	{ "FRAME_OVERHEAD", "Estimate overhead per frame (s)", "Time added per frame in the duration estimate (download, saving, dithering)" },
	{ "OPERATION_OVERHEAD", "Estimate overhead per operation (s)", "Time added per slew, centering, focus or guiding start in the duration estimate" },
};

/* a column of the page as the settings of the other tabs of Ain: bold section titles,
   the labels on the left, the values in the last column */
static QGridLayout *makeSettingsColumn() {
	QGridLayout *layout = new QGridLayout();
	layout->setAlignment(Qt::AlignTop);
	layout->setColumnStretch(0, 3);
	layout->setColumnStretch(1, 1);
	layout->setColumnStretch(2, 1);
	layout->setColumnStretch(3, 1);
	return layout;
}

static QLabel *addSection(QGridLayout *layout, int *row, const QString& title) {
	if (*row > 0) {
		layout->addItem(new QSpacerItem(1, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), (*row)++, 0);
	}
	QLabel *label = new QLabel(title);
	label->setStyleSheet("QLabel { font-weight: bold; }");
	layout->addWidget(label, (*row)++, 0, 1, 4);
	return label;
}

/* a spin box for an item of AGENT_SCHEDULER_SETTINGS, with its label on the left */
void SchedulerWidget::addSetting(QGridLayout *grid, int *row, const char *name) {
	for (const auto& item : setting_items) {
		if (strcmp(item[0], name)) continue;
		QLabel *label = new QLabel(QString(item[1]) + ":");
		label->setToolTip(item[2]);
		grid->addWidget(label, *row, 0, 1, 2);
		QDoubleSpinBox *spin = new QDoubleSpinBox();
		spin->setObjectName(QString("SchedulerSetting_") + name);
		spin->setToolTip(item[2]);
		spin->setKeyboardTracking(false);
		spin->setEnabled(false);
		connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SchedulerWidget::sendSettings);
		grid->addWidget(spin, (*row)++, 2, 1, 2);
		m_settings[name] = spin;
		m_setting_labels[name] = label;
	}
}

/* a page with one column as the settings of the other tabs, it scrolls when it is not tall enough */
static QFrame *makeSettingsPage(QGridLayout *layout) {
	QBoxLayout *page_layout;
	QFrame *page = makePage(&page_layout);
	page_layout->setContentsMargins(0, 0, 0, 0);
	QWidget *content = new QWidget();
	content->setLayout(layout);
	QScrollArea *scroll = new QScrollArea();
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setWidget(content);
	scroll->setWidgetResizable(true);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	page_layout->addWidget(scroll);
	return page;
}

/* the hook tasks and when they run, like the rest of Ain every change is sent to the agent right away */
QWidget *SchedulerWidget::buildHooksPage() {
	QGridLayout *layout = makeSettingsColumn();
	int row = 0;

	addSection(layout, &row, "Event hooks:");
	for (const auto& hook : hook_items) {
		QComboBox *box = new QComboBox();
		box->setEditable(true);
		box->setInsertPolicy(QComboBox::NoInsert);
		box->setToolTip("The stored task that runs at this moment; a hook task is never scheduled as an ordinary task");
		connect(box, QOverload<int>::of(&QComboBox::activated), this, &SchedulerWidget::sendHooks);
		connect(box->lineEdit(), &QLineEdit::editingFinished, this, &SchedulerWidget::sendHooks);
		box->setObjectName(QString("SchedulerHook_") + hook[0]);
		m_hooks[hook[0]] = box;
		QLabel *label = new QLabel(QString(hook[1]) + ":");
		m_hook_labels[hook[0]] = label;
		layout->addWidget(label, row, 0, 1, 2);
		layout->addWidget(box, row++, 2, 1, 2);
	}

	addSection(layout, &row, "Hook timing:");
	m_night = new QComboBox();
	m_night->setObjectName("SchedulerNight");
	m_night->setToolTip("How dark it must be for the night of the night hooks; each task keeps its own @twilight");
	connect(m_night, QOverload<int>::of(&QComboBox::activated), this, [this]() {
		send({ SchedulerRequest::switches1(selectedAgent(), P_NIGHT, m_night->currentData().toString()) });
	});
	layout->addWidget(new QLabel("Night limit:"), row, 0, 1, 2);
	layout->addWidget(m_night, row++, 2, 1, 2);
	addSetting(layout, &row, "NIGHT_START_OFFSET");
	addSetting(layout, &row, "NIGHT_END_OFFSET");
	addSetting(layout, &row, "IDLE_HOOK_DELAY");
	addSetting(layout, &row, "SAFE_DELAY");
	return makeSettingsPage(layout);
}

QWidget *SchedulerWidget::buildSettingsPage() {
	// like the rest of Ain, every change is sent to the agent right away
	QGridLayout *layout = makeSettingsColumn();
	int row = 0;

	addSection(layout, &row, "Observation data:");
	for (const char *item : { "OBJECT", "FITS", "FOLDER" }) {
		QCheckBox *check = new QCheckBox();
		check->setObjectName(QString("SchedulerData_") + item);
		connect(check, &QCheckBox::clicked, this, [this]() {
			SchedulerRequest data;
			data.type = SchedulerRequest::Switch;
			data.device = selectedAgent();
			data.property = P_DATA;
			for (auto it = m_data.begin(); it != m_data.end(); ++it) {
				data.items.append(it.key());
				data.switches.append(it.value()->isChecked());
			}
			send({ data });
		});
		m_data[item] = check;
		layout->addWidget(check, row++, 0, 1, 4);
	}
	m_data["OBJECT"]->setText("Task name as object name");
	m_data["FITS"]->setText("Task FITS keywords (SCHTASK, SCHRUN, SCHNIGHT...)");
	m_data["FOLDER"]->setText("Folder per task");
	m_data_dir = new QLineEdit();
	m_data_dir->setObjectName("SchedulerDataDir");
	m_data_dir->setPlaceholderText("folder of the Imager Agent");
	m_data_dir->setToolTip("Parent folder of the task folders on the server; empty for the folder the Imager Agent had when the scheduler was started");
	connect(m_data_dir, &QLineEdit::editingFinished, this, [this]() {
		QString base = m_data_dir->text().trimmed();
		if (base != m_properties.value(P_DATA_DIR).text("BASE")) {
			send({ SchedulerRequest::text(selectedAgent(), P_DATA_DIR, "BASE", base) });
		}
	});
	// a path, it takes the width of the page after its label
	QHBoxLayout *data_dir_layout = new QHBoxLayout();
	data_dir_layout->addWidget(new QLabel("Base folder:"));
	data_dir_layout->addWidget(m_data_dir, 1);
	layout->addLayout(data_dir_layout, row++, 0, 1, 4);

	addSection(layout, &row, "Task selection:");
	m_strategy = new QComboBox();
	m_strategy->setObjectName("SchedulerStrategy");
	m_strategy->setToolTip(
		"Compact: the best task starts as soon as one can run, the night fills from dusk.\n"
		"Best visibility: the scheduler plans the whole night, never with fewer runs than compact,\n"
		"and places the runs where their targets stand highest; it may wait for a planned run.");
	connect(m_strategy, QOverload<int>::of(&QComboBox::activated), this, [this]() {
		send({ SchedulerRequest::switches1(selectedAgent(), P_STRATEGY, m_strategy->currentData().toString()) });
	});
	m_strategy_label = new QLabel("Strategy:");
	m_strategy_label->setToolTip(m_strategy->toolTip());
	layout->addWidget(m_strategy_label, row, 0, 1, 2);
	layout->addWidget(m_strategy, row++, 2, 1, 2);
	addSetting(layout, &row, "SETTING_WEIGHT");
	addSetting(layout, &row, "ALTITUDE_WEIGHT");
	addSetting(layout, &row, "VISIBILITY_FLIP_PENALTY");

	m_safety_title = addSection(layout, &row, "Safety:");
	m_safety_missing = new QComboBox();
	m_safety_missing->setObjectName("SchedulerSafetyMissing");
	m_safety_missing->setToolTip(
		"What no safety information means: no Auxiliary Agent, no AGENT_AUX_SAFETY or an Idle light.\n"
		"Treated as unsafe, the scheduler holds until a safe signal arrives.");
	connect(m_safety_missing, QOverload<int>::of(&QComboBox::activated), this, [this]() {
		send({ SchedulerRequest::switches1(selectedAgent(), P_SAFETY_MISSING, m_safety_missing->currentData().toString()) });
	});
	m_safety_missing_label = new QLabel("Missing safety information:");
	m_safety_missing_label->setToolTip(m_safety_missing->toolTip());
	layout->addWidget(m_safety_missing_label, row, 0, 1, 2);
	layout->addWidget(m_safety_missing, row++, 2, 1, 2);

	addSection(layout, &row, "Timings:");
	addSetting(layout, &row, "IDLE_CHECK_INTERVAL");
	addSetting(layout, &row, "START_TIMEOUT");
	addSetting(layout, &row, "STOP_TIMEOUT");

	addSection(layout, &row, "Task duration estimate:");
	addSetting(layout, &row, "FRAME_OVERHEAD");
	addSetting(layout, &row, "OPERATION_OVERHEAD");

	// the settings of a newer agent
	layout->addItem(new QSpacerItem(1, 10, QSizePolicy::Minimum, QSizePolicy::Fixed), row++, 0);
	m_other_title = new QLabel("Other:");
	m_other_title->setStyleSheet("QLabel { font-weight: bold; }");
	m_other_title->hide();
	layout->addWidget(m_other_title, row++, 0, 1, 4);
	m_other_layout = makeSettingsColumn();
	m_other_layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(m_other_layout, row++, 0, 1, 4);
	return makeSettingsPage(layout);
}

void SchedulerWidget::sendHooks() {
	const SchedulerProperty hooks = m_properties.value(P_HOOKS);
	QStringList items, values;
	bool changed = false;
	for (const auto& hook : hook_items) {
		if (!hasHook(hooks, hook[0])) continue;
		QString value = m_hooks[hook[0]]->currentText().trimmed();
		changed |= value != hooks.text(hook[0]);
		items.append(hook[0]);
		values.append(value);
	}
	if (changed) {
		send({ SchedulerRequest::text(selectedAgent(), P_HOOKS, items, values) });
	}
}

void SchedulerWidget::sendSettings() {
	// only the items of the agent, an older one does not have all of them
	const SchedulerProperty property = m_properties.value(P_SETTINGS);
	SchedulerRequest settings;
	settings.type = SchedulerRequest::Number;
	settings.device = selectedAgent();
	settings.property = P_SETTINGS;
	for (const SchedulerItem& item : property.items) {
		if (m_settings.contains(item.name)) {
			settings.items.append(item.name);
			settings.numbers.append(m_settings[item.name]->value());
		}
	}
	if (!settings.items.isEmpty()) {
		send({ settings });
	}
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
	m_imager_process = SchedulerProperty();
	m_imager_stats = SchedulerProperty();
	m_imager_batch = SchedulerProperty();
	m_site = SchedulerProperty();
	m_progress_current.clear();
	m_progress_shown = false;
	m_task_started = false;
	m_task_finished = false;
	m_exposure_running = false;
	resetProgress();
	m_pending_property.clear();
	m_messages.clear();
	updateState();
	updateTasks();
	updatePlan();
	updateHistory();
	updateSettings();
	handleScripting(SchedulerProperty());
	updateChart();
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
	} else if (property.device == imagerAgent()) {
		handleImager(property);
	} else if (property.device == mountAgent()) {
		handleMount(property);
	}
}

void SchedulerWidget::updateProperty(const SchedulerProperty& property) {
	if (property.device == selectedAgent()) {
		handleProperty(property);
	} else if (property.device == scriptingAgent()) {
		handleScripting(property);
	} else if (property.device == imagerAgent()) {
		handleImager(property);
	} else if (property.device == mountAgent()) {
		handleMount(property);
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
	} else if (device == imagerAgent() && name.isEmpty()) {
		m_imager_process = SchedulerProperty();
		m_imager_stats = SchedulerProperty();
		m_imager_batch = SchedulerProperty();
	} else if (device == mountAgent() && (name.isEmpty() || name == "GEOGRAPHIC_COORDINATES")) {
		m_site = SchedulerProperty();
		updateChart();
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
		updateTaskProgress();
		updateChart();
	} else if (name == P_TASK_STATE) {
		updateTasks();
		updateChart();
	} else if (name == P_PLAN) {
		updatePlan();
		updateState();
		updateChart();
	} else if (name == P_HISTORY) {
		updateHistory();
	} else if (name == P_SETTINGS || name == P_HOOKS || name == P_NIGHT || name == P_STRATEGY || name == P_SAFETY_MISSING || name == P_DATA || name == P_DATA_DIR) {
		updateSettings();
	} else if (isTaskProperty(name)) {
		updateChart();
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

	updateTaskProgress();

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

static QString hms(double seconds) {
	int s = int(qMax(0.0, seconds) + 0.5);
	return QString("%1:%2:%3").arg(s / 3600, 2, 10, QChar('0')).arg(s / 60 % 60, 2, 10, QChar('0')).arg(s % 60, 2, 10, QChar('0'));
}

static void setProgress(QProgressBar *bar, int max, int value, const QString& format) {
	bar->setRange(0, max);
	bar->setValue(value);
	bar->setFormat(format);
}

void SchedulerWidget::handleImager(const SchedulerProperty& property) {
	if (property.name == "AGENT_START_PROCESS") {
		m_imager_process = property;
	} else if (property.name == "AGENT_IMAGER_STATS") {
		m_imager_stats = property;
	} else if (property.name == "AGENT_IMAGER_BATCH") {
		m_imager_batch = property;
		return;
	} else {
		return;
	}
	updateExposureProgress();
}

void SchedulerWidget::handleMount(const SchedulerProperty& property) {
	if (property.name == "GEOGRAPHIC_COORDINATES") {
		m_site = property;
		updateChart();
	}
}

/* The chart shows the targets of the stored tasks, the runs of the night plan, and the
   task selected in the list and the running one over the others */
void SchedulerWidget::updateChart() {
	if (m_site.name.isEmpty() || m_site.item("LATITUDE") == nullptr) {
		m_chart->setSite(false);
	} else {
		double longitude = m_site.number("LONGITUDE");
		if (longitude > 180) longitude -= 360;
		m_chart->setSite(true, m_site.number("LATITUDE"), longitude);
	}

	QList<SchedulerSkyChart::Target> targets;
	for (const QString& name : storedTaskNames()) {
		SchedulerTask task = SchedulerTask::fromText(taskText(name));
		QStringList coordinates = task.value("target").split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
		if (coordinates.size() != 2) continue;
		bool ok_ra, ok_dec;
		SchedulerSkyChart::Target target;
		target.name = name;
		target.ra = SexagesimalConverter::stringToDouble(coordinates[0], &ok_ra);
		target.dec = SexagesimalConverter::stringToDouble(coordinates[1], &ok_dec);
		if (!ok_ra || !ok_dec) continue;
		QString altitude = task.value("altitude");
		altitude.remove(QRegularExpression("[<>=\\s]"));
		bool ok_altitude;
		double min_altitude = altitude.toDouble(&ok_altitude);
		if (ok_altitude) target.minAltitude = min_altitude;
		targets.append(target);
	}
	m_chart->setTargets(targets);

	// the plan has "yyyy-MM-dd HH:mm-HH:mm | name | ...", a run that ends before it starts ends the next day
	static const QRegularExpression when(R"(^(\d{4}-\d\d-\d\d) (\d\d:\d\d)-(\d\d:\d\d)$)");
	QList<SchedulerSkyChart::Run> runs;
	for (const SchedulerItem& item : m_properties.value(P_PLAN).items) {
		QStringList columns = scheduler_label_columns(item.label);
		QRegularExpressionMatch match = when.match(columns.value(0).trimmed());
		if (!match.hasMatch()) continue;
		QDateTime start = QDateTime::fromString(match.captured(1) + " " + match.captured(2), "yyyy-MM-dd HH:mm");
		QDateTime end = QDateTime::fromString(match.captured(1) + " " + match.captured(3), "yyyy-MM-dd HH:mm");
		start.setTimeSpec(Qt::UTC);
		end.setTimeSpec(Qt::UTC);
		if (end < start) end = end.addDays(1);
		SchedulerSkyChart::Run run;
		run.name = columns.value(1).trimmed();
		run.start = start.toSecsSinceEpoch();
		run.end = end.toSecsSinceEpoch();
		runs.append(run);
	}
	m_chart->setRuns(runs);

	QStringList highlighted;
	if (m_tasks->currentItem()) highlighted << m_tasks->currentItem()->data(0, Qt::UserRole).toString();
	if (!runningTask().isEmpty()) highlighted << runningTask();
	m_chart->setHighlighted(highlighted);
}

void SchedulerWidget::resetProgress() {
	m_task_status->setText(ledHtml(SCHEDULER_IDLE) + " Idle");
	setProgress(m_exposure_progress, 1, 0, "Exposure: Idle");
	setProgress(m_batch_progress, 1, 0, "Batch: Idle");
	setProgress(m_sequence_progress, 1, 0, "Task: Idle");
	m_sequence_label->setText("Total exposure: " + hms(0));
}

/* The progress of the task the scheduler runs, as the Sequence tab shows the sequence of Ain.
   The Scripting Agent runs other sequences too: its state is followed from the start of the
   sequence of a task to its end, and the end of the last task is shown until the next one. */
void SchedulerWidget::updateTaskProgress() {
	QString current = m_properties.value(P_CURRENT).text("CURRENT");
	if (current != m_progress_current) {
		if (!current.isEmpty()) {
			// a new task: the state of the last sequence is not its own until its sequence starts
			m_progress_shown = true;
			m_task_started = false;
			m_task_finished = false;
			m_exposure_running = false;
			resetProgress();
			m_task_status->setText(ledHtml(SCHEDULER_BUSY) + " Starting");
		} else if (!m_task_started) {
			// the task ended before its sequence started
			resetProgress();
		}
		m_progress_current = current;
	}
	if (!m_progress_shown || m_sequence_state.name.isEmpty()) {
		return;
	}
	if (!m_task_started) {
		if (current.isEmpty() || m_sequence_state.state != SCHEDULER_BUSY) {
			return;
		}
		m_task_started = true;
	}
	// the end of the sequence may come after the scheduler is done with the task
	if (m_task_finished || (current.isEmpty() && m_sequence_state.state == SCHEDULER_BUSY)) {
		return;
	}

	double progress = m_sequence_state.number("PROGRESS");
	double total = m_sequence_state.number("PROGRESS_TOTAL");
	int percent = total > 0 ? int(progress / total * 100 + 0.5) : 0;
	double exposure = m_sequence_state.number("EXPOSURE");
	double exposure_total = m_sequence_state.number("EXPOSURE_TOTAL");
	switch (m_sequence_state.state) {
		case SCHEDULER_BUSY:
			if (int(m_sequence_state.number("STEP")) == 0 && progress == 0) {
				setProgress(m_exposure_progress, 1, 0, "Exposure: Idle");
				setProgress(m_batch_progress, 1, 0, "Batch: Idle");
			}
			m_task_status->setText(ledHtml(SCHEDULER_BUSY) + " In progress");
			setProgress(m_sequence_progress, 100, percent, "Task: %v% complete");
			m_sequence_label->setText(QString("Total exposure: %1    (%2 remaining)").arg(hms(exposure_total)).arg(hms(exposure_total - exposure)));
			break;
		case SCHEDULER_OK:
			m_task_finished = true;
			m_task_status->setText(ledHtml(SCHEDULER_OK) + " Complete");
			setProgress(m_sequence_progress, 100, percent, "Task: %v% complete");
			m_sequence_label->setText("Total exposure: " + hms(exposure_total));
			break;
		case SCHEDULER_ALERT:
			m_task_finished = true;
			m_task_status->setText(ledHtml(SCHEDULER_ALERT) + " Failed");
			setProgress(m_sequence_progress, 100, percent, "Task: Failed (%v% complete)");
			m_sequence_label->setText("Total exposure: " + hms(exposure_total));
			break;
		default:
			break;
	}
}

/* The exposures of the Imager Agent while the sequence of a task runs */
void SchedulerWidget::updateExposureProgress() {
	if (!m_task_started || m_task_finished || m_imager_process.name.isEmpty()) {
		return;
	}
	const SchedulerItem *exposure = m_imager_process.item("EXPOSURE");
	int frames_complete = int(m_imager_stats.number("FRAME"));
	int frames_total = int(m_imager_stats.number("FRAMES"));
	if (m_imager_process.state == SCHEDULER_BUSY) {
		if (!exposure || !exposure->on) {
			return;
		}
		m_exposure_running = true;
		const SchedulerItem *batch_exposure = m_imager_batch.item("EXPOSURE");
		int exposure_time = batch_exposure ? int(batch_exposure->target) : 1;
		int elapsed = int(exposure_time - m_imager_stats.number("EXPOSURE"));
		setProgress(m_exposure_progress, exposure_time, qBound(0, elapsed, exposure_time), "Exposure: %v of %m seconds elapsed...");
		if (frames_total < 0) {
			setProgress(m_batch_progress, frames_complete, frames_complete - 1, "Batch: exposure %v complete...");
		} else {
			setProgress(m_batch_progress, frames_total, frames_complete - 1, "Batch: exposure %v of %m complete...");
		}
	} else if (m_exposure_running) {
		m_exposure_running = false;
		if (m_imager_process.state == SCHEDULER_OK) {
			setProgress(m_exposure_progress, 100, 100, "Exposure: Complete");
			setProgress(m_batch_progress, 100, 100, "Batch: Complete");
		} else {
			setProgress(m_exposure_progress, 1, 0, "Exposure: Aborted");
			if (frames_total < 0) {
				setProgress(m_batch_progress, frames_complete, frames_complete - 1, "Batch: exposure %v complete");
			} else {
				setProgress(m_batch_progress, frames_total, frames_complete - 1, "Batch: exposure %v of %m complete");
			}
		}
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
}

void SchedulerWidget::updateHistory() {
	const SchedulerProperty *property = m_properties.contains(P_HISTORY) ? &m_properties[P_HISTORY] : nullptr;
	fillList(m_history, property, 1, [](const QStringList& c) {
		return QStringList{ c[0], c[1], c[2], c[3], c[4], c[5] };
	});
}

void SchedulerWidget::updateSettings() {
	// a field being typed in keeps its text until it is finished
	setComboItems(m_night, m_properties.value(P_NIGHT));

	// an agent without strategies is always compact
	const SchedulerProperty strategy = m_properties.value(P_STRATEGY);
	bool has_strategy = !strategy.items.isEmpty() || m_properties.value(P_SETTINGS).items.isEmpty();
	m_strategy->setVisible(has_strategy);
	m_strategy_label->setVisible(has_strategy);
	setComboItems(m_strategy, strategy);

	// an agent without safety information never holds
	const SchedulerProperty safety_missing = m_properties.value(P_SAFETY_MISSING);
	bool has_safety = !safety_missing.items.isEmpty() || m_properties.value(P_SETTINGS).items.isEmpty();
	m_safety_missing->setVisible(has_safety);
	m_safety_missing_label->setVisible(has_safety);
	m_safety_title->setVisible(has_safety);
	setComboItems(m_safety_missing, safety_missing);

	const SchedulerProperty hooks = m_properties.value(P_HOOKS);
	for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
		bool shown = hasHook(hooks, it.key().toUtf8().constData());
		it.value()->setVisible(shown);
		m_hook_labels[it.key()]->setVisible(shown);
		if (!it.value()->lineEdit()->hasFocus()) it.value()->setCurrentText(hooks.text(it.key()));
	}

	const SchedulerProperty data = m_properties.value(P_DATA);
	for (auto it = m_data.begin(); it != m_data.end(); ++it) {
		it.value()->setChecked(data.on(it.key()));
	}
	if (!m_data_dir->hasFocus()) m_data_dir->setText(m_properties.value(P_DATA_DIR).text("BASE"));

	const SchedulerProperty settings = m_properties.value(P_SETTINGS);
	// the items this version does not know, under "Other"
	QStringList other;
	for (const SchedulerItem& item : settings.items) {
		if (!m_settings.contains(item.name) || m_other_settings.contains(item.name)) other.append(item.name);
	}
	if (other != m_other_settings) {
		for (const QString& name : m_other_settings) {
			delete m_settings.take(name);
			delete m_setting_labels.take(name);
		}
		m_other_settings = other;
		int row = 0;
		for (const QString& name : other) {
			const SchedulerItem *item = settings.item(name);
			QLabel *label = new QLabel(item->label + ":");
			m_other_layout->addWidget(label, row, 0, 1, 2);
			QDoubleSpinBox *spin = new QDoubleSpinBox();
			spin->setObjectName("SchedulerSetting_" + name);
			spin->setToolTip(name);
			spin->setKeyboardTracking(false);
			connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SchedulerWidget::sendSettings);
			m_other_layout->addWidget(spin, row++, 2, 1, 2);
			m_settings[name] = spin;
			m_setting_labels[name] = label;
		}
		m_other_title->setVisible(!other.isEmpty());
	}
	// the ranges come from the agent, an item an older agent does not have is hidden
	for (auto it = m_settings.begin(); it != m_settings.end(); ++it) {
		const SchedulerItem *item = settings.item(it.key());
		QDoubleSpinBox *spin = it.value();
		bool shown = item != nullptr || settings.items.isEmpty();
		spin->setVisible(shown);
		m_setting_labels[it.key()]->setVisible(shown);
		// the label of the item as the agent names it
		if (item != nullptr) m_setting_labels[it.key()]->setText(item->label + ":");
		spin->setEnabled(item != nullptr);
		if (item == nullptr || spin->hasFocus()) continue;
		spin->blockSignals(true);
		spin->setDecimals(item->step > 0 && item->step < 1 ? 1 : 0);
		spin->setRange(item->min, item->max);
		spin->setSingleStep(item->step > 0 ? item->step : 1);
		spin->setValue(item->number);
		spin->blockSignals(false);
	}
}

void SchedulerWidget::updateEditorRunning() {
	QString running = m_properties.value(P_CURRENT).text("CURRENT");
	m_editor->setRunningTask(running);
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
	menu.addAction("Move up", this, [this, name]() { taskCommand(name, "MOVE_UP"); });
	menu.addAction("Move down", this, [this, name]() { taskCommand(name, "MOVE_DOWN"); });
	menu.addSeparator();
	QMenu *hooks = menu.addMenu("Use as hook");
	hooks->setEnabled(stored);
	const SchedulerProperty hook_property = m_properties.value(P_HOOKS);
	for (const auto& hook : hook_items) {
		if (!hasHook(hook_property, hook[0])) continue;
		QString item_name = hook[0];
		bool is_hook = hook_property.text(item_name) == name;
		QAction *action = hooks->addAction(hook[1], this, [this, name, item_name, is_hook]() {
			QList<SchedulerRequest> requests;
			SchedulerTask task = SchedulerTask::fromText(taskText(name));
			if (!is_hook && task.value("priority").trimmed() != "hook") {
				// @priority hook first, so that a running scheduler does not pick the task as an ordinary one
				QStringList dropped;
				for (const auto& entry : task.header()) {
					if (SchedulerTaskForm::hookRejectedKeys().contains(entry.first)) dropped.append("@" + entry.first);
				}
				QString question = QString("Mark '%1' as a hook-only task with @priority hook?\n\n"
					"It is then never scheduled as an ordinary task, also when no hook names it.").arg(name);
				if (!dropped.isEmpty()) {
					question += QString("\n\nA hook task has no scheduling keys: %1 will be removed.").arg(dropped.join(", "));
				}
				QMessageBox::StandardButton answer = QMessageBox::question(this, "Use as hook", question,
					QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
				if (answer == QMessageBox::Cancel) {
					return;
				}
				if (answer == QMessageBox::Yes) {
					QList<QPair<QString, QString>> header;
					bool has_priority = false;
					for (const auto& entry : task.header()) {
						if (SchedulerTaskForm::hookRejectedKeys().contains(entry.first)) continue;
						if (entry.first == "priority") {
							header.append(qMakePair(entry.first, QString("hook")));
							has_priority = true;
						} else {
							header.append(entry);
						}
					}
					if (!has_priority) {
						int at = 0;
						while (at < header.size() && header[at].first != "task") at++;
						header.insert(at < header.size() ? at + 1 : 0, qMakePair(QString("priority"), QString("hook")));
					}
					task.setHeader(header);
					requests.append(SchedulerRequest::text(selectedAgent(), taskProperty(name), "TASK", task.toText()));
				}
			}
			requests.append(SchedulerRequest::text(selectedAgent(), P_HOOKS, item_name, is_hook ? QString() : name));
			send(requests);
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
