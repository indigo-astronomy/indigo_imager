// Test helper, preloaded into ain_imager with LD_PRELOAD. It drives the real widgets from a
// command file, so the tests need no test hooks in the application.
//
//   AIN_CMD	commands to run, one per line, appended by the test
//   AIN_OUT	everything the commands report

#include <QtWidgets>

static QString cmd_path;
static QString out_path;
static qint64 cmd_pos = 0;
static QString next_file;
static QMessageBox::StandardButton next_answer = QMessageBox::No;

static void out(const QString &line) {
	QFile file(out_path);
	if (file.open(QIODevice::Append | QIODevice::Text)) {
		QTextStream(&file) << line << "\n";
	}
}

static QString bare(QString text) {
	return text.remove('&');
}

// File dialogs can not be answered headless, the code behind them still runs.
QString QFileDialog::getOpenFileName(QWidget *, const QString &, const QString &, const QString &, QString *, QFileDialog::Options) {
	return next_file;
}

// Questions are answered with what the last "answer" command set, No if none, and reported.
QMessageBox::StandardButton QMessageBox::question(QWidget *, const QString &, const QString &text, QMessageBox::StandardButtons, QMessageBox::StandardButton) {
	out("QUESTION " + QString(text).replace('\n', ' '));
	return next_answer;
}

static QAction *find_action(const QString &text) {
	for (QWidget *widget : QApplication::allWidgets()) {
		for (QAction *action : widget->actions()) {
			if (bare(action->text()) == text) return action;
		}
	}
	return nullptr;
}

static QString combo_tag(QComboBox *combo) {
	for (int i = 0; i < combo->count(); i++) {
		QString item = combo->itemText(i);
		if (item.startsWith("Astrometry Agent") || item.startsWith("Platesolver Agent")) return "SOLVER_AGENT";
		if (item == "Current filter") return "FILTER";
		if (item.contains("Upload")) return "SOURCE";
	}
	return QString();
}

static QComboBox *find_combo(const QString &tag) {
	for (QWidget *widget : QApplication::allWidgets()) {
		QComboBox *combo = qobject_cast<QComboBox *>(widget);
		if (combo && combo_tag(combo) == tag) return combo;
	}
	return nullptr;
}

static QPushButton *find_button(const QString &text) {
	for (QWidget *widget : QApplication::allWidgets()) {
		QPushButton *button = qobject_cast<QPushButton *>(widget);
		if (button && bare(button->text()) == text) return button;
	}
	return nullptr;
}

// the log of ain_imager is a QTextEdit, the one of ain_platesolver a QPlainTextEdit; read-only text in a dialog,
// like Image Info of ain_platesolver, is not the log
static bool find_log(QString *text) {
	for (QWidget *widget : QApplication::allWidgets()) {
		QTextEdit *edit = qobject_cast<QTextEdit *>(widget);
		if (edit && edit->isReadOnly() && qobject_cast<QMainWindow *>(edit->window())) {
			*text = edit->toPlainText();
			return true;
		}
	}
	for (QWidget *widget : QApplication::allWidgets()) {
		QPlainTextEdit *edit = qobject_cast<QPlainTextEdit *>(widget);
		if (edit && edit->isReadOnly() && qobject_cast<QMainWindow *>(edit->window())) {
			*text = edit->toPlainText();
			return true;
		}
	}
	return false;
}

static void dump() {
	out("DUMP");
	for (const char *name : {"Astrometry Agent", "Platesolver Agent"}) {
		QAction *action = find_action(name);
		out(QString("MENU %1 checked=%2").arg(name).arg(action && action->isChecked() ? 1 : 0));
	}
	for (QWidget *widget : QApplication::allWidgets()) {
		QComboBox *combo = qobject_cast<QComboBox *>(widget);
		if (!combo) continue;
		QString tag = combo_tag(combo);
		if (tag.isEmpty()) continue;
		QStringList items;
		for (int i = 0; i < combo->count(); i++) items << combo->itemText(i);
		out(QString("COMBO %1 current='%2' items=[%3]").arg(tag, combo->currentText(), items.join(" | ")));
	}
}

// ---- the Scheduler tab, found by the object names of its widgets

static bool in_scheduler(QWidget *widget) {
	for (QWidget *w = widget; w; w = w->parentWidget()) {
		if (w->objectName() == "SchedulerView" || w->objectName() == "SchedulerControl") return true;
	}
	return false;
}

static QWidget *named(const QString &name) {
	for (QWidget *widget : QApplication::allWidgets()) {
		if (widget->objectName() == name) return widget;
	}
	return nullptr;
}

static QString plain(const QString &html) {
	// an image of a label is an object replacement character in its text
	return QTextDocumentFragment::fromHtml(html).toPlainText().remove(QChar(0xFFFC)).simplified();
}

// the states of the steps of a sequence editor, top to bottom, as their status icons show them
static QString step_states(QWidget *editor) {
	QList<QPair<int, QString>> steps;
	for (QToolButton *button : editor->findChildren<QToolButton *>()) {
		QVariant state = button->property("stepState");
		if (state.isValid()) steps.append(qMakePair(button->mapTo(editor, QPoint(0, 0)).y(), state.toString()));
	}
	// a page that was never shown has no layout: its steps are not in order
	std::stable_sort(steps.begin(), steps.end(), [](const QPair<int, QString> &a, const QPair<int, QString> &b) { return a.first < b.first; });
	QStringList states;
	for (const auto &step : steps) states << step.second;
	return states.join(" ");
}

// SEQ lines: the state of the Sequence tab
static void seq_dump() {
	out("SEQ_DUMP");
	QLabel *status = qobject_cast<QLabel *>(named("SequenceStatus"));
	out(QString("SEQ SequenceStatus %1").arg(status ? plain(status->text()) : QString("?")));
	QProgressBar *progress = qobject_cast<QProgressBar *>(named("SequenceProgress"));
	out(QString("SEQ SequenceProgress %1").arg(progress ? progress->text() : QString("?")));
	QWidget *editor = named("SequenceEditor");
	out(QString("SEQ SequenceSteps %1").arg(editor ? step_states(editor) : QString("?")));
	out("SEQ_END");
}

// SCHED lines: the state, the lists and the editor
static void sched_dump() {
	out("SCHED_DUMP");
	for (const char *name : { "SchedulerState", "SchedulerCurrent", "SchedulerReason", "SchedulerEditorTitle", "SchedulerEditorStatus", "SchedulerTaskStatus" }) {
		QLabel *label = qobject_cast<QLabel *>(named(name));
		out(QString("SCHED %1 %2").arg(name, label ? plain(label->text()) : QString("?")));
	}
	QLabel *sequence = qobject_cast<QLabel *>(named("SchedulerSequence"));
	out(QString("SCHED SchedulerSequence %1").arg(sequence ? plain(sequence->text()) : QString("?")));
	for (const char *name : { "SchedulerExposure", "SchedulerBatch", "SchedulerProgress" }) {
		QProgressBar *progress = qobject_cast<QProgressBar *>(named(name));
		out(QString("SCHED %1 %2").arg(name, progress ? progress->text() : QString("?")));
	}
	QWidget *chart = named("SchedulerSkyChart");
	QString chart_text;
	if (chart) QMetaObject::invokeMethod(chart, "describe", Qt::DirectConnection, Q_RETURN_ARG(QString, chart_text));
	out(QString("SCHED SchedulerChart %1").arg(chart ? chart_text : QString("?")));
	QTabWidget *pages = qobject_cast<QTabWidget *>(named("SchedulerEditorPages"));
	QWidget *editor = named("SchedulerTaskEditor");
	if (editor) out("SCHED EditorSteps " + step_states(editor));
	if (pages) {
		QStringList tabs;
		for (int i = 0; i < pages->count(); i++) tabs << QString("%1:%2").arg(pages->tabText(i)).arg(pages->isTabEnabled(i) ? "on" : "off");
		out("SCHED EditorTabs " + tabs.join(" "));
	}
	QPlainTextEdit *task_text = qobject_cast<QPlainTextEdit *>(named("SchedulerEditorText"));
	if (task_text) out(QString("SCHED EditorText %1").arg(task_text->isReadOnly() ? "read only" : "editable"));
	// the choices of the Settings page by their item names
	for (const char *name : { "SchedulerNight", "SchedulerStrategy", "SchedulerSafetyMissing" }) {
		QComboBox *combo = qobject_cast<QComboBox *>(named(name));
		if (combo && !combo->isHidden()) out(QString("SCHED %1 %2 %3").arg(name, combo->currentData().toString(), combo->isEnabled() ? "on" : "off"));
	}
	// the settings of the agent, a hidden one is not in it
	QList<QDoubleSpinBox *> settings;
	for (QWidget *widget : QApplication::allWidgets()) {
		QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox *>(widget);
		if (spin && spin->objectName().startsWith("SchedulerSetting_") && !spin->isHidden()) settings.append(spin);
	}
	std::sort(settings.begin(), settings.end(), [](QDoubleSpinBox *a, QDoubleSpinBox *b) { return a->objectName() < b->objectName(); });
	for (QDoubleSpinBox *spin : settings) {
		out(QString("SCHED Setting %1 %2 %3").arg(spin->objectName().mid(17), spin->text(), spin->isEnabled() ? "on" : "off"));
	}
	out(QString("SCHED EditorMode %1").arg(pages ? (pages->currentWidget() == named("SchedulerEditorText") ? "text" : "visual") : "?"));
	for (const char *name : { "SchedulerTasks", "SchedulerPlan", "SchedulerHistory" }) {
		QTreeWidget *tree = qobject_cast<QTreeWidget *>(named(name));
		for (int i = 0; tree && i < tree->topLevelItemCount(); i++) {
			QStringList columns;
			for (int c = 0; c < tree->columnCount(); c++) columns << tree->topLevelItem(i)->text(c);
			out(QString("SCHED %1 %2").arg(name, columns.join(" | ")));
		}
	}
	out("SCHED_END");
}

static QWidget *open_popup() {
	// the offscreen platform may have no active popup: a visible menu is the popup
	if (QWidget *popup = QApplication::activePopupWidget()) return popup;
	for (QWidget *widget : QApplication::topLevelWidgets()) {
		if (qobject_cast<QMenu *>(widget) && widget->isVisible()) return widget;
	}
	return nullptr;
}

static void run(const QString &line) {
	QString cmd = line.section(' ', 0, 0);
	QString arg = line.section(' ', 1);
	if (cmd == "click") {
		QAction *action = find_action(arg);
		if (action) action->trigger();
		else out("ERROR no action " + arg);
	} else if (cmd == "tooltip") {
		QAction *action = find_action(arg);
		out("TOOLTIP " + (action ? action->toolTip() : QString()));
	} else if (cmd == "button") {
		QPushButton *button = find_button(arg);
		if (button) button->click();
		else out("ERROR no button " + arg);
	} else if (cmd == "upload") {
		next_file = arg;
		QPushButton *button = find_button("Solve");
		if (button) button->click();
		else out("ERROR no Solve button");
	} else if (cmd == "enabled") {
		QPushButton *button = find_button(arg);
		if (button) out(QString("ENABLED %1 %2").arg(arg).arg(button->isEnabled() ? 1 : 0));
		else out("ERROR no button " + arg);
	} else if (cmd == "open") {
		next_file = arg;
		QAction *action = find_action("Open Image...");
		if (action) action->trigger();
		else out("ERROR no Open Image action");
	} else if (cmd == "loadseq") {
		next_file = arg;
		// the one of the Sequence tab, the task editor of the scheduler has one too
		QWidget *editor = named("SequenceEditor");
		for (QToolButton *button : editor ? editor->findChildren<QToolButton *>() : QList<QToolButton *>()) {
			if (button->toolTip() == "Load sequence from file") {
				button->click();
				return;
			}
		}
		out("ERROR no load sequence button");
	} else if (cmd == "source") {
		QComboBox *combo = find_combo("SOURCE");
		int index = combo ? combo->findText(arg) : -1;
		if (index >= 0) combo->setCurrentIndex(index);
		else out("ERROR no source " + arg);
	} else if (cmd == "tab") {
		// the tabs of the scheduler first: "Settings" is also a tab of the guider
		QTabWidget *scheduler_tabs = qobject_cast<QTabWidget *>(named("SchedulerTabs"));
		for (int i = 0; scheduler_tabs && i < scheduler_tabs->count(); i++) {
			if (scheduler_tabs->tabText(i) == arg) {
				scheduler_tabs->setCurrentIndex(i);
				return;
			}
		}
		for (QWidget *widget : QApplication::allWidgets()) {
			QTabWidget *tabs = qobject_cast<QTabWidget *>(widget);
			if (!tabs) continue;
			for (int i = 0; i < tabs->count(); i++) {
				if (bare(tabs->tabText(i)) == arg) {
					tabs->setCurrentIndex(i);
					return;
				}
			}
		}
		// a tab bar of its own, like the pages of the scheduler task editor
		for (QWidget *widget : QApplication::allWidgets()) {
			QTabBar *bar = qobject_cast<QTabBar *>(widget);
			if (!bar || qobject_cast<QTabWidget *>(bar->parentWidget())) continue;
			for (int i = 0; i < bar->count(); i++) {
				if (bare(bar->tabText(i)) == arg) {
					bar->setCurrentIndex(i);
					return;
				}
			}
		}
		out("ERROR no tab " + arg);
	} else if (cmd == "dump") {
		dump();
	} else if (cmd == "seq_dump") {
		seq_dump();
	} else if (cmd == "sched_dump") {
		sched_dump();
	} else if (cmd == "sched_click" || cmd == "sched_press") {
		// a push or tool button of the Scheduler tab by its text or tooltip; sched_press
		// clicks it from the event loop, for a button that opens a menu: the menu loop
		// can not run the poll timer it was clicked from
		for (QWidget *widget : QApplication::allWidgets()) {
			QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
			if (button && in_scheduler(button) && (bare(button->text()) == arg || button->toolTip() == arg)) {
				if (!button->isEnabled()) out("DISABLED " + arg);
				else if (cmd == "sched_press") QTimer::singleShot(0, button, &QAbstractButton::click);
				else button->click();
				return;
			}
		}
		out("ERROR no scheduler button " + arg);
	} else if (cmd == "sched_page") {
		// a page of the task editor, as the user picks its tab
		QTabWidget *pages = qobject_cast<QTabWidget *>(named("SchedulerEditorPages"));
		for (int i = 0; pages && i < pages->count(); i++) {
			if (pages->tabText(i) == arg) {
				pages->setCurrentIndex(i);
				return;
			}
		}
		out("ERROR no editor page " + arg);
	} else if (cmd == "sched_select" || cmd == "sched_edit") {
		QTreeWidget *tree = qobject_cast<QTreeWidget *>(named("SchedulerTasks"));
		for (int i = 0; tree && i < tree->topLevelItemCount(); i++) {
			if (tree->topLevelItem(i)->text(0) == arg) {
				tree->setCurrentItem(tree->topLevelItem(i));
				if (cmd == "sched_edit") emit tree->itemActivated(tree->topLevelItem(i), 0);
				return;
			}
		}
		out("ERROR no scheduler task " + arg);
	} else if (cmd == "sched_hook") {
		// sched_hook ITEM task: the hook choice of the Settings page, as the user picks it
		QComboBox *combo = qobject_cast<QComboBox *>(named("SchedulerHook_" + arg.section(' ', 0, 0)));
		if (!combo) {
			out("ERROR no hook " + arg);
			return;
		}
		int index = combo->findText(arg.section(' ', 1));
		if (index < 0) {
			out("ERROR no hook choice " + arg);
			return;
		}
		combo->setCurrentIndex(index);
		emit combo->activated(index);
	} else if (cmd == "sched_setting") {
		// sched_setting ITEM value: a setting of the Settings page, as the user types it
		QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox *>(named("SchedulerSetting_" + arg.section(' ', 0, 0)));
		if (!spin || spin->isHidden()) {
			out("ERROR no setting " + arg);
			return;
		}
		spin->setValue(arg.section(' ', 1).toDouble());
	} else if (cmd == "sched_choose") {
		// sched_choose NAME ITEM: the choice of a combo box by its object name and item name, as the user picks it
		QComboBox *combo = qobject_cast<QComboBox *>(named(arg.section(' ', 0, 0)));
		int index = combo ? combo->findData(arg.section(' ', 1)) : -1;
		if (index < 0) {
			out("ERROR no choice " + arg);
			return;
		}
		combo->setCurrentIndex(index);
		emit combo->activated(index);
	} else if (cmd == "sched_combo") {
		// sched_combo FROM TO: the choice of a combo box of the scheduler showing FROM, as the user picks it
		QString from = arg.section(' ', 0, 0), to = arg.section(' ', 1);
		for (QWidget *widget : QApplication::allWidgets()) {
			QComboBox *combo = qobject_cast<QComboBox *>(widget);
			if (!combo || !in_scheduler(combo) || !combo->isVisible() || combo->currentText() != from) continue;
			int index = combo->findText(to);
			if (index < 0) continue;
			combo->setCurrentIndex(index);
			emit combo->activated(index);
			return;
		}
		out("ERROR no combo " + arg);
	} else if (cmd == "sched_open") {
		next_file = arg;
		for (QWidget *widget : QApplication::allWidgets()) {
			QToolButton *button = qobject_cast<QToolButton *>(widget);
			if (button && in_scheduler(button) && button->toolTip() == "Open a task file") {
				button->click();
				return;
			}
		}
		out("ERROR no open task button");
	} else if (cmd == "sched_text") {
		// the text of the editor in text mode, on one line
		QPlainTextEdit *text = qobject_cast<QPlainTextEdit *>(named("SchedulerEditorText"));
		out("SCHED_TEXT " + (text ? text->toPlainText().replace('\n', "\\n") : QString()));
	} else if (cmd == "log") {
		QString text;
		QFile file(arg);
		if (find_log(&text) && file.open(QIODevice::WriteOnly | QIODevice::Text)) QTextStream(&file) << text;
	} else if (cmd == "shot") {
		for (QWidget *widget : QApplication::topLevelWidgets()) {
			if (qobject_cast<QMainWindow *>(widget)) widget->grab().save(arg);
		}
	} else if (cmd == "popup_shot") {
		// the open popup, and the frame width of the frames on it
		QWidget *popup = open_popup();
		if (!popup) {
			out("ERROR no popup");
			return;
		}
		popup->grab().save(arg);
		// the frame of the widget the popup shows, not of the frames inside it
		int frame = -1;
		for (QWidgetAction *action : popup->findChildren<QWidgetAction *>()) {
			if (QFrame *shown = qobject_cast<QFrame *>(action->defaultWidget())) frame = shown->frameWidth();
		}
		out(QString("POPUP frame %1").arg(frame));
	} else if (cmd == "popup_close") {
		if (QWidget *popup = open_popup()) popup->close();
	} else if (cmd == "answer") {
		next_answer = arg == "Yes" ? QMessageBox::Yes : QMessageBox::No;
	} else if (cmd == "sync") {
		out("SYNC " + arg);
	}
}

static void poll() {
	QFile file(cmd_path);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
	file.seek(cmd_pos);
	QStringList lines;
	while (!file.atEnd()) lines << QString::fromUtf8(file.readLine()).trimmed();
	cmd_pos = file.pos();
	file.close();
	for (const QString &line : lines) {
		if (!line.isEmpty()) run(line);
	}
}

static void start() {
	cmd_path = qEnvironmentVariable("AIN_CMD");
	out_path = qEnvironmentVariable("AIN_OUT");
	if (cmd_path.isEmpty()) return;
	QTimer *timer = new QTimer();
	QObject::connect(timer, &QTimer::timeout, poll);
	timer->start(100);
}

Q_COREAPP_STARTUP_FUNCTION(start)
