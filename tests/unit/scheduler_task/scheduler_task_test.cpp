// Unit test of the scheduler task editor of AIN.
//
// The example tasks of INDIGO_SCHEDULER_GUIDE.md are loaded into the editor: a
// task the visual editor can show must come out with the same header and the
// same steps, and a task it can not show must stay complete as text.

#include <QApplication>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>
#include <cstdio>

#include "SchedulerTask.h"
#include "SchedulerTaskForm.h"
#include "SchedulerTaskEditor.h"
#include "IndigoSequence.h"
#include "IndigoSequenceItem.h"
#include "IndigoSequenceParser.h"

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

int main(int argc, char *argv[]) {
	qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication app(argc, argv);
	testSplit();
	testLabels();
	testForm();
	testGuideTasks();
	testFallback();
	testBuild();
	fprintf(stderr, "%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
