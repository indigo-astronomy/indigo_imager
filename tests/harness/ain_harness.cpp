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
		for (QWidget *widget : QApplication::allWidgets()) {
			QToolButton *button = qobject_cast<QToolButton *>(widget);
			if (button && button->toolTip() == "Load sequence from file") {
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
		out("ERROR no tab " + arg);
	} else if (cmd == "dump") {
		dump();
	} else if (cmd == "log") {
		QString text;
		QFile file(arg);
		if (find_log(&text) && file.open(QIODevice::WriteOnly | QIODevice::Text)) QTextStream(&file) << text;
	} else if (cmd == "shot") {
		for (QWidget *widget : QApplication::topLevelWidgets()) {
			if (qobject_cast<QMainWindow *>(widget)) widget->grab().save(arg);
		}
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
