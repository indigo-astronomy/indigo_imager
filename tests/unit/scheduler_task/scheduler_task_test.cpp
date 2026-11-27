// Unit test of the scheduler task editor of AIN.
//
// The example tasks of INDIGO_SCHEDULER_GUIDE.md are loaded into the editor: a
// task the visual editor can show must come out with the same header and the
// same steps, and a task it can not show must stay complete as text.

#include <QApplication>
#include <QAbstractSpinBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDateTime>
#include <QComboBox>
#include <QTabWidget>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <cstdio>

#include "SchedulerTask.h"
#include "SchedulerTaskForm.h"
#include "SchedulerTaskEditor.h"
#include "SchedulerSkyChart.h"
#include "IndigoSequence.h"
#include "IndigoSequenceItem.h"
#include "IndigoSequenceParser.h"
#include "QLineEditSG.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDoubleSpinBox>

static int failures = 0;
static int checks = 0;

#define CHECK(condition, ...) do { \
	checks++; \
	if (!(condition)) { \
		failures++; \
		fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #condition); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	} \
} while (0)

static QByteArray u(const QString& s) { return s.toUtf8(); }

/* the same steps and the same parameter values */
static QString sameBody(const QString& a, const QString& b) {
	IndigoSequenceParser parser;
	return IndigoSequenceParser::compareCalls(parser.parse(a), parser.parse(b));
}

static QStringList guideTasks() {
	QFile file(SCHEDULER_GUIDE);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		return QStringList();
	}
	QString guide = QTextStream(&file).readAll();
	QStringList tasks;
	QRegularExpression block("```(?:js|javascript)\\n(.*?)```", QRegularExpression::DotMatchesEverythingOption);
	auto it = block.globalMatch(guide);
	while (it.hasNext()) {
		QString code = it.next().captured(1);
		if (code.trimmed().startsWith("// @task")) {
			tasks.append(code);
		}
	}
	return tasks;
}

static void testSplit() {
	SchedulerTask task = SchedulerTask::fromText(
		"\xEF\xBB\xBF// @task   M31 LRGB\n"
		"//\n"
		"// @Priority high 80\n"
		"// my notes\n"
		"// @target 1 2\n"
		"var sequence = new Sequence(scheduler_task.name);\n");
	CHECK(task.name() == "M31 LRGB", "name '%s'", u(task.name()).constData());
	CHECK(task.value("priority") == "high 80", "keys are lower case");
	CHECK(!task.has("target"), "the header ends at a plain comment");
	CHECK(task.body().startsWith("// my notes"), "body '%s'", u(task.body()).constData());

	SchedulerTask built;
	built.set("task", "A");
	built.set("duration", "5");
	built.setBody("var sequence = new Sequence(scheduler_task.name);\nsequence.start();");
	QString text = built.toText();
	CHECK(text == "// @task     A\n// @duration 5\n\nvar sequence = new Sequence(scheduler_task.name);\nsequence.start();\n", "toText '%s'", u(text).constData());
	SchedulerTask again = SchedulerTask::fromText(text);
	CHECK(again.header() == built.header() && again.body() == built.body() + "\n", "round trip");
}

static void testLabels() {
	QStringList columns = scheduler_label_columns("2026-11-20 20:44-21:10 | M31 LRGB | run 1 | task | PREEMPTED | by SN | 2026abc");
	CHECK(columns.size() == 6, "six columns");
	CHECK(columns[1] == "M31 LRGB" && columns[4] == "PREEMPTED", "columns");
	CHECK(columns[5] == "by SN | 2026abc", "the reason keeps its separators: '%s'", u(columns[5]).constData());
	columns = scheduler_label_columns("Park | HOOK | hook shutdown | - | - | -");
	CHECK(columns[3].isEmpty() && columns[5].isEmpty(), "'-' is an empty column");
	columns = scheduler_label_columns("Park | HOOK | hook shutdown | - | - | - | -", 7);
	CHECK(columns.size() == 7 && columns[5].isEmpty() && columns[6].isEmpty(), "seven columns of a task: '%s'", u(columns.join("|")).constData());
	columns = scheduler_label_columns("South | UNREACHABLE | normal 50 | run 0/1 | retries left 0 | 0h01 estimated | never rises above the horizon", 7);
	CHECK(columns[5] == "0h01 estimated" && columns[6] == "never rises above the horizon", "the reason of a task is its own column");
	columns = scheduler_label_columns("Old | QUEUED | normal 50 | run 0/1 | retries left 0 | 0h01 estimated", 7);
	CHECK(columns.size() == 7 && columns[6].isEmpty(), "an older agent without the reason");
}

static void testForm() {
	SchedulerTaskForm form;
	QList<QPair<QString, QString>> header = {
		{ "task", "SS Cyg" }, { "priority", "high 20" }, { "target", "21:42:42.8 +43:35:10" },
		{ "altitude", ">= 30" }, { "airmass", "< 2.5" }, { "ha", "-2 .. 2" }, { "moon", "> 40 width 7" },
		{ "illumination", "< 60" }, { "twilight", "nautical" }, { "window", "20:00 .. 04:00" },
		{ "duration", "5" }, { "repeat", "10 every 30" }, { "retry", "3 after 5" }, { "preempted", "fail" },
		{ "enabled", "no" }, { "folder", "SS Cyg" }, { "observer", "Jane Doe" }, { "pi", "Prof. Smith" }, { "project", "SURVEY-7" }
	};
	QString error;
	CHECK(form.setHeader(header, &error), "all keys: %s", u(error).constData());
	CHECK(form.header() == header, "unchanged values are written as loaded");

	QList<QPair<QString, QString>> absolute = { { "task", "A" }, { "window", "2026-11-20 21:00 .." } };
	CHECK(form.setHeader(absolute, &error) && form.header() == absolute, "absolute window from a time on: %s", u(error).constData());
	QList<QPair<QString, QString>> until = { { "task", "A" }, { "window", ".. 2026-11-21 02:30" } };
	CHECK(form.setHeader(until, &error) && form.header() == until, "absolute window until a time: %s", u(error).constData());

	CHECK(!form.setHeader({ { "task", "A" }, { "bogus", "1" } }, &error), "unknown key");
	CHECK(!form.setHeader({ { "task", "A" }, { "window", "2026-11-20 21:00 .. 03:00" } }, &error), "mixed window forms");
	CHECK(!form.setHeader({ { "task", "A" }, { "altitude", "30" } }, &error), "a missing operator");
	CHECK(!form.setHeader({ { "task", "A" }, { "priority", "normal 50.5" } }, &error), "a level the form can not show");
	CHECK(form.header() == (QList<QPair<QString, QString>>{ { "task", "" } }), "a rejected header resets the form");
}

static void testGuideTasks() {
	QStringList tasks = guideTasks();
	CHECK(tasks.size() >= 10, "the guide has its example tasks (%d found in %s)", int(tasks.size()), SCHEDULER_GUIDE);
	int visual = 0;
	for (const QString& text : tasks) {
		SchedulerTaskEditor editor;
		editor.loadTask(text, "x");
		SchedulerTask original = SchedulerTask::fromText(text);
		SchedulerTask shown = SchedulerTask::fromText(editor.taskText());
		if (editor.isVisualMode()) {
			visual++;
			CHECK(shown.header() == original.header(), "header of '%s':\n%s", u(original.name()).constData(), u(editor.taskText()).constData());
			// a header-only example gets an empty sequence
			QString difference = original.body().trimmed().isEmpty() ? QString() : sameBody(original.body(), shown.body());
			CHECK(difference.isEmpty(), "body of '%s': %s", u(original.name()).constData(), u(difference).constData());
			CHECK(shown.body().contains("new Sequence(scheduler_task.name)"), "the sequence is named after the task");
			CHECK(!editor.isModified(), "a loaded task is not modified");
		} else {
			CHECK(editor.taskText() == text, "'%s' is kept as text unchanged", u(original.name()).constData());
		}
	}
	CHECK(visual >= tasks.size() - 2, "most guide tasks are shown visually (%d of %d)", visual, int(tasks.size()));
	fprintf(stderr, "guide tasks: %d, visual: %d\n", int(tasks.size()), visual);
}

static void testFallback() {
	const char *with_code =
		"// @task A\n\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"if (scheduler_task.run > 1) {\n\tsequence.focus(5);\n}\n"
		"sequence.capture_batch(1, 10);\n"
		"sequence.start();\n";
	SchedulerTaskEditor editor;
	editor.loadTask(with_code, "A");
	CHECK(!editor.isVisualMode(), "JavaScript besides steps is edited as text");
	CHECK(editor.taskText() == with_code, "and kept unchanged");

	const char *with_comment =
		"// @task A\n\n"
		"// focus first\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"sequence.focus(5);\n"
		"sequence.start();\n";
	editor.loadTask(with_comment, "A");
	CHECK(!editor.isVisualMode(), "comments in the body are edited as text");

	const char *bad_header = "// @task A\n// @window 21:00 .. 2026-01-01 03:00\n\nvar sequence = new Sequence(scheduler_task.name);\nsequence.start();\n";
	editor.loadTask(bad_header, "A");
	CHECK(!editor.isVisualMode(), "a header the form can not show is edited as text");
}

static void testBuild() {
	SchedulerTaskEditor editor;
	SchedulerTaskForm *form = editor.findChild<SchedulerTaskForm*>();
	IndigoSequence *body = editor.findChild<IndigoSequence*>();
	CHECK(form && body && body->isTaskMode(), "the editor has a form and a task body");
	if (!form || !body) return;

	QString error;
	CHECK(form->setHeader({ { "task", "M31" }, { "target", "00:42:44.3 +41:16:09" } }, &error), "%s", u(error).constData());
	IndigoSequenceItem *go = new IndigoSequenceItem("precise_goto", body);
	body->addItem(go);
	CHECK(go->isTargetBound(), "a new target step uses the task target");
	IndigoSequenceItem *guide = new IndigoSequenceItem("start_guiding", body);
	guide->setParameter(0, 0);
	body->addItem(guide);
	IndigoSequenceItem *batch = new IndigoSequenceItem("capture_batch", body);
	batch->setParameter(0, 20);
	batch->setParameter(1, 300);
	body->addItem(batch);

	QString text = editor.taskText();
	SchedulerTask task = SchedulerTask::fromText(text);
	CHECK(task.name() == "M31", "name");
	CHECK(task.body().contains("sequence.precise_goto(1, scheduler_task.ra, scheduler_task.dec);"), "body:\n%s", u(task.body()).constData());
	CHECK(task.body().contains("sequence.start_guiding();"), "an optional exposure at its minimum is left out:\n%s", u(task.body()).constData());
	CHECK(task.body().contains("sequence.capture_batch(20, 300);"), "body:\n%s", u(task.body()).constData());
	CHECK(editor.isModified(), "a built task is modified");

	// the text shows the same task in the visual editor again
	SchedulerTaskEditor again;
	again.loadTask(text, QString());
	CHECK(again.isVisualMode() && again.taskText() == text, "round trip:\n%s\n---\n%s", u(text).constData(), u(again.taskText()).constData());

	// bound coordinates follow the item when it is unbound and bound again
	go->setTargetBound(false);
	go->setParameter(1, "00:42:44.3");
	CHECK(qAbs(go->getParameter(1).toDouble() - 0.712306) < 1e-4, "sexagesimal RA: %f", go->getParameter(1).toDouble());
}

static QCheckBox *checkBox(QWidget *parent, const QString& text) {
	for (QCheckBox *box : parent->findChildren<QCheckBox*>()) {
		if (box->text() == text) return box;
	}
	return nullptr;
}

/* the target of the Scheduling page and its goto in the steps */
static void testTargetGoto() {
	const char *with_slew =
		"// @task  M31\n"
		"// @target 00:42:44.30 +41:16:09.00\n\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"sequence.wait(1);\n"
		"sequence.slew(\"1:00:00\", \"20:00:00\");\n"
		"sequence.capture_batch(2, 10);\n"
		"sequence.slew(\"2:00:00\", \"30:00:00\");\n"
		"sequence.start();\n";
	SchedulerTaskEditor editor;
	editor.loadTask(with_slew, "M31");
	SchedulerTaskForm *form = editor.findChild<SchedulerTaskForm*>();
	QCheckBox *precise = form ? checkBox(form, "precise") : nullptr;
	CHECK(editor.isVisualMode() && precise, "the target has a precise goto option");
	if (!precise) return;
	CHECK(!precise->isChecked(), "a slew is not a precise goto");

	QDoubleSpinBox *exposure = nullptr;
	for (QDoubleSpinBox *spin : form->findChildren<QDoubleSpinBox*>()) {
		if (spin->suffix() == " s") exposure = spin;
	}
	CHECK(exposure, "the precise goto has an exposure");
	if (!exposure) return;
	exposure->setValue(5);
	precise->click();
	QString body = SchedulerTask::fromText(editor.taskText()).body();
	CHECK(body.contains("sequence.wait(1);\nsequence.precise_goto(5, scheduler_task.ra, scheduler_task.dec);\nsequence.capture_batch(2, 10);"),
		"the first slew becomes a precise goto to the target:\n%s", u(body).constData());
	CHECK(body.contains("sequence.slew(\"2:00:00\", \"30:00:00\");") || body.contains("sequence.slew(2, 30);"), "the second goto is kept:\n%s", u(body).constData());

	precise->click();
	body = SchedulerTask::fromText(editor.taskText()).body();
	CHECK(body.contains("sequence.wait(1);\nsequence.slew(scheduler_task.ra, scheduler_task.dec);"), "unchecked it is a slew:\n%s", u(body).constData());

	// a task without a goto gets one as the first step when the target is set
	const char *without_goto =
		"// @task A\n\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"sequence.capture_batch(2, 10);\n"
		"sequence.start();\n";
	editor.loadTask(without_goto, "A");
	QLineEditSG *ra = nullptr, *dec = nullptr;
	for (QLineEditSG *edit : form->findChildren<QLineEditSG*>()) {
		if (edit->sgMode() == QLineEditSG::RA) ra = edit; else dec = edit;
	}
	CHECK(ra && dec, "the target has RA and Dec");
	if (!ra || !dec) return;
	checkBox(form, "Target")->click();
	CHECK(!SchedulerTask::fromText(editor.taskText()).body().contains("slew"), "no goto before the target has coordinates");
	ra->setText("10:00:00");
	emit ra->textEdited(ra->text());
	emit ra->editingFinished();
	dec->setText("+20:00:00");
	emit dec->textEdited(dec->text());
	emit dec->editingFinished();
	body = SchedulerTask::fromText(editor.taskText()).body();
	CHECK(body.contains("Sequence(scheduler_task.name);\nsequence.slew(scheduler_task.ra, scheduler_task.dec);\nsequence.capture_batch(2, 10);"),
		"a slew to the target is the first step:\n%s", u(body).constData());

	// a loaded precise goto is shown
	const char *with_precise =
		"// @task   B\n"
		"// @target 10:00:00.00 +20:00:00.00\n\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"sequence.precise_goto(3, scheduler_task.ra, scheduler_task.dec);\n"
		"sequence.start();\n";
	editor.loadTask(with_precise, "B");
	CHECK(precise->isChecked() && exposure->value() == 3, "the precise goto of the steps is shown");
	CHECK(editor.taskText() == with_precise, "loading does not change the steps:\n%s", u(editor.taskText()).constData());
}

static QComboBox *priorityClass(QWidget *form) {
	for (QComboBox *box : form->findChildren<QComboBox*>()) {
		if (box->findText("hook") >= 0 && box->findText("urgent") >= 0) return box;
	}
	return nullptr;
}

/* @priority hook: a class without a level and without the scheduling keys */
static void testHook() {
	const char *hook_task =
		"// @task     Park\n"
		"// @priority hook\n"
		"// @duration 2\n\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"sequence.park();\n"
		"sequence.start();\n";
	SchedulerTaskEditor editor;
	editor.loadTask(hook_task, "Park");
	SchedulerTaskForm *form = editor.findChild<SchedulerTaskForm*>();
	QComboBox *priority = form ? priorityClass(form) : nullptr;
	CHECK(editor.isVisualMode() && priority && priority->currentText() == "hook" && form->isHook(), "a task with @priority hook is shown as a hook");
	if (!priority) return;
	CHECK(editor.taskText() == hook_task, "a hook task is written as loaded:\n%s", u(editor.taskText()).constData());
	CHECK(!editor.isModified(), "and is not modified");
	CHECK(!checkBox(form, "Repeat")->isEnabled() && !checkBox(form, "Time window")->isEnabled() && !checkBox(form, "Enabled")->isEnabled(), "the scheduling keys are disabled");
	CHECK(checkBox(form, "Target")->isEnabled() && checkBox(form, "Duration")->isEnabled(), "the target and the duration stay editable");

	priority->setCurrentText("high");
	SchedulerTask task = SchedulerTask::fromText(editor.taskText());
	CHECK(task.value("priority") == "high 50" && task.value("duration") == "2", "another class has a level:\n%s", u(editor.taskText()).constData());
	CHECK(checkBox(form, "Repeat")->isEnabled(), "and the scheduling keys");
	priority->setCurrentText("hook");
	CHECK(editor.taskText() == hook_task, "hook again, the task is as loaded:\n%s", u(editor.taskText()).constData());

	// the scheduling keys of a task made a hook are not written, and come back with another class
	const char *repeated =
		"// @task     Flats\n"
		"// @priority normal 40\n"
		"// @repeat   3\n\n"
		"var sequence = new Sequence(scheduler_task.name);\n"
		"sequence.start();\n";
	editor.loadTask(repeated, "Flats");
	priority->setCurrentText("hook");
	task = SchedulerTask::fromText(editor.taskText());
	CHECK(task.value("priority") == "hook" && !task.has("repeat"), "a hook has no @repeat:\n%s", u(editor.taskText()).constData());
	priority->setCurrentText("normal");
	CHECK(editor.taskText() == repeated, "another class gets it back:\n%s", u(editor.taskText()).constData());

	// and also after the task was shown as text
	editor.loadTask(repeated, "Flats");
	QTabWidget *pages = editor.findChild<QTabWidget*>("SchedulerEditorPages");
	priority->setCurrentText("hook");
	pages->setCurrentIndex(pages->count() - 1);
	CHECK(!editor.isVisualMode() && !SchedulerTask::fromText(editor.taskText()).has("repeat"), "the text of a hook has no @repeat:\n%s", u(editor.taskText()).constData());
	pages->setCurrentIndex(0);
	CHECK(editor.isVisualMode() && priority->currentText() == "hook", "back on the forms it is a hook");
	priority->setCurrentText("normal");
	CHECK(editor.taskText() == repeated, "another class after the text gets the keys and the level back:\n%s", u(editor.taskText()).constData());

	// a task loaded next does not get them
	editor.loadTask(hook_task, "Park");
	priority->setCurrentText("normal");
	CHECK(!SchedulerTask::fromText(editor.taskText()).has("repeat"), "another task does not get them:\n%s", u(editor.taskText()).constData());

	SchedulerTaskForm plain;
	QString error;
	CHECK(!plain.setHeader({ { "task", "A" }, { "priority", "hook 50" } }, &error), "@priority hook takes no level");
	CHECK(!plain.setHeader({ { "task", "A" }, { "priority", "hook" }, { "repeat", "2" } }, &error) && error.contains("@repeat"), "a hook with @repeat is refused: %s", u(error).constData());
	CHECK(!plain.setHeader({ { "task", "A" }, { "hook", "" } }, &error), "@hook is not a key any more");
}

/* the positions of the chart, against values of the precise routines of INDIGO */
static void testSky() {
	const qint64 t = QDateTime(QDate(2026, 11, 18), QTime(18, 0), Qt::UTC).toSecsSinceEpoch();
	double sun = SchedulerSky::sunAltitude(t, 42.7, 23.3);
	CHECK(sun < -30 && sun > -45, "the Sun at 18 UTC in Sofia in November is well below the horizon: %.1f", sun);
	double noon = SchedulerSky::sunAltitude(QDateTime(QDate(2026, 3, 20), QTime(12, 7), Qt::UTC).toSecsSinceEpoch(), 0, 0);
	CHECK(noon > 88, "the Sun at noon on the equator at the equinox is near the zenith: %.1f", noon);
	double ra, dec, parallax;
	SchedulerSky::moon(t, &ra, &dec, &parallax);
	CHECK(parallax > 0.89 && parallax < 1.02 && dec > -30 && dec < 30, "the Moon has its parallax and declination: %.2f %.1f", parallax, dec);
	// M31 transits near the zenith of Sofia
	double best = -90;
	for (qint64 s = t - 12 * 3600; s < t + 12 * 3600; s += 300) best = qMax(best, SchedulerSky::targetAltitude(0.7122, 41.269, s, 42.7, 23.3));
	CHECK(best > 87 && best < 89, "M31 culminates at 90 - |42.7 - 41.3| degrees: %.1f", best);

	QPair<qint64, qint64> night = SchedulerSky::nightWindow(QDateTime(QDate(2026, 11, 18), QTime(12, 0), Qt::UTC).toSecsSinceEpoch(), 42.7, 23.3);
	QDateTime start = QDateTime::fromSecsSinceEpoch(night.first, Qt::UTC), end = QDateTime::fromSecsSinceEpoch(night.second, Qt::UTC);
	CHECK(start.time() == QTime(14, 0) && end.date() == QDate(2026, 11, 19) && end.time() == QTime(7, 0),
		"the night in Sofia, sunset 15:00 and sunrise 05:15 UTC, is shown from an hour before to an hour after: %s - %s", u(start.toString(Qt::ISODate)).constData(), u(end.toString(Qt::ISODate)).constData());
	qint64 polar_now = QDateTime(QDate(2026, 12, 21), QTime(12, 0), Qt::UTC).toSecsSinceEpoch();
	QPair<qint64, qint64> polar = SchedulerSky::nightWindow(polar_now, 80, 15);
	CHECK(polar.second - polar.first == 24 * 3600 && polar.first <= polar_now, "a night without end shows a day");
}

static bool busyStep(QWidget *step) {
	for (QWidget *child : step->findChildren<QWidget*>()) {
		if (child->property("stepState").toString() == "busy") return true;
	}
	return false;
}

/* only the task the scheduler runs is read only, any other can be edited and sent meanwhile */
static void testRunning() {
	const char *a = "// @task A\n\nvar sequence = new Sequence(scheduler_task.name);\nsequence.wait(1);\nsequence.start();\n";
	const char *b = "// @task B\n\nvar sequence = new Sequence(scheduler_task.name);\nsequence.wait(2);\nsequence.start();\n";
	SchedulerTaskEditor editor;
	QPushButton *apply = nullptr;
	for (QPushButton *button : editor.findChildren<QPushButton*>()) if (button->text() == "Apply") apply = button;
	editor.loadTask(a, "A");
	editor.setRunningTask("A");
	CHECK(editor.isRunning() && apply && !apply->isEnabled(), "the running task is read only");
	if (!apply) return;
	// its text can be seen but not edited, and the steps come back read only with the progress
	QTabWidget *pages = editor.findChild<QTabWidget*>("SchedulerEditorPages");
	QPlainTextEdit *text = editor.findChild<QPlainTextEdit*>("SchedulerEditorText");
	bool all_tabs = true;
	for (int i = 0; i < pages->count(); i++) all_tabs = all_tabs && pages->isTabEnabled(i);
	CHECK(all_tabs, "all the pages of the running task can be shown");
	editor.showStepStates({ 2 }, -1, 0);
	pages->setCurrentWidget(text);
	CHECK(!editor.isVisualMode() && text->isReadOnly(), "its text is shown read only");
	pages->setCurrentIndex(1);
	IndigoSequence *steps = editor.findChild<IndigoSequence*>();
	IndigoSequenceItem *step = steps ? steps->getItemAt(0) : nullptr;
	CHECK(editor.isVisualMode() && editor.isRunning() && step && step->findChild<QAbstractSpinBox*>() && !step->findChild<QAbstractSpinBox*>()->isEnabled() && busyStep(step),
		"its steps are shown again read only with their state");

	editor.newTask();
	CHECK(!editor.isRunning() && apply->isEnabled(), "a new task can be edited and sent while A runs");
	editor.loadTask(b, "B");
	CHECK(!editor.isRunning() && apply->isEnabled(), "another stored task can be edited and sent while A runs");
	editor.loadTask(a, QString());
	CHECK(!editor.isRunning() && apply->isEnabled(), "a task from a file can be edited and sent while A runs");
	editor.setResult(true, QString());
	CHECK(editor.isRunning() && !apply->isEnabled(), "a file that is sent as the running task is read only");
	editor.loadTask(b, "B");
	editor.setRunningTask("B");
	CHECK(editor.isRunning(), "the next running task is read only");
	editor.setRunningTask(QString());
	CHECK(!editor.isRunning() && apply->isEnabled(), "and editable again when the run ends");
}

int main(int argc, char *argv[]) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication app(argc, argv);
	testSplit();
	testLabels();
	testForm();
	testGuideTasks();
	testFallback();
	testBuild();
	testTargetGoto();
	testHook();
	testSky();
	testRunning();
	fprintf(stderr, "%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
