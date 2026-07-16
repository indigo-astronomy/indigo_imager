// Copyright (c) 2026
// All rights reserved.

#include "guidelogviewerwindow.h"

#include "guidelogstats.h"
#include "pecurvewindow.h"

#include <QAbstractItemView>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QBrush>
#include <QCheckBox>
#include <QEvent>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>
#include <QSet>
#include <QRegularExpression>

#include <cmath>
#include <algorithm>

#include <simpleplot.h>

namespace {

// Row order of the statistics table (column 0 = axis name).
enum StatsRow {
	StatsRowRa = 0,
	StatsRowDec,
	StatsRowTotal,
	StatsRowCount
};

const char *const kStatsRowNames[StatsRowCount] = {
	"RA",
	"Dec",
	"Total"
};

// Axis label colours, matching the RA/Dec plot conventions.
const QColor kStatsRowColors[StatsRowCount] = {
	QColor(255, 80, 80),    // RA  - red
	QColor(60, 170, 245),   // Dec - blue
	QColor(150, 150, 150)   // Total - gray
};

// Statistics value columns (column 0 holds the axis name).
enum StatsColumn {
	StatsColumnAxis = 0,
	StatsColumnRmseArc,
	StatsColumnPeakArc,
	StatsColumnRmsePx,
	StatsColumnPeakPx,
	StatsColumnCount
};

// Formats a value with 2 decimals and its unit, or "n/a" when the unit system
// is absent (e.g. "1.20\"" or "2.60 px").
QString formatValue(bool valid, double value, const QString &unit) {
	return valid ? (QString::number(value, 'f', 2) + unit) : QString("n/a");
}

// --- Over-/under-correction balance indicator -----------------------------
// The metric is the lag-1 autocorrelation of the residual (see CorrectionBalance):
// |v| <= 0.15 is well tuned, up to 0.4 slightly off, beyond that clearly off.
const double kBalanceOkThreshold = 0.15;
const double kBalanceWarnThreshold = 0.4;
const double kBalanceScale = 0.9; // bar spans [-0.9, +0.9]; larger values clamp

QColor balanceStatusColor(double v) {
	const double a = std::fabs(v);
	if (a <= kBalanceOkThreshold) return QColor(90, 200, 110);   // green
	if (a <= kBalanceWarnThreshold) return QColor(230, 180, 60); // amber
	return QColor(225, 90, 80);                                  // red
}

// Verdict such as "Balanced (ρ₁ +0.04)" / "Over-correcting (ρ₁ −0.42)".
QString balanceVerdictText(double v, bool valid) {
	if (!valid) {
		return QStringLiteral("n/a");
	}
	const double a = std::fabs(v);
	QString word;
	if (a <= kBalanceOkThreshold) {
		word = QStringLiteral("Balanced");
	} else if (v > 0.0) {
		word = (a <= kBalanceWarnThreshold) ? QStringLiteral("Slightly under")
		                                    : QStringLiteral("Under-correcting");
	} else {
		word = (a <= kBalanceWarnThreshold) ? QStringLiteral("Slightly over")
		                                    : QStringLiteral("Over-correcting");
	}
	const QString sign = (v >= 0.0) ? QStringLiteral("+") : QStringLiteral("−");
	return QString("%1 (ρ₁ %2%3)").arg(word, sign, QString::number(a, 'f', 2));
}

QColor colorForGuideColumn(const QString &header, int column) {
	const QString h = header.trimmed().toLower();

	// Keep RA in reds and DEC in blues, following ain_imager conventions.
	if (h == "ra dif") return QColor(255, 0, 0);
	if (h == "ra dif(\")") return QColor(230, 70, 70);
	if (h == "rmse ra") return QColor(200, 40, 40);
	if (h == "rmse ra(\")") return QColor(170, 20, 20);
	if (h == "ra correction") return QColor(255, 120, 120);

	if (h == "dec dif(\")") return QColor(3, 172, 240);
	if (h == "dec dif") return QColor(0, 150, 220);
	if (h == "rmse dec") return QColor(0, 125, 200);
	if (h == "rmse dec(\")") return QColor(70, 180, 245);
	if (h == "dec correction") return QColor(0, 100, 170);

	if (h == "x dif") return QColor(132, 232, 72);
	if (h == "y dif") return QColor(255, 193, 7);
	if (h == "dithering") return QColor(180, 180, 180);

	// Stable fallback for unknown numeric columns.
	static const QColor fallback[] = {
		QColor(255, 105, 180),
		QColor(170, 120, 255),
		QColor(241, 183, 1),
		QColor(90, 200, 170)
	};
	return fallback[column % (sizeof(fallback) / sizeof(fallback[0]))];
}

QString timeOnlyLabel(const QString &timestamp) {
	QString ts = timestamp.trimmed();
	if (ts.startsWith('"')) {
		ts.remove(0, 1);
	}
	if (ts.endsWith('"')) {
		ts.chop(1);
	}

	int tPos = ts.indexOf('T');
	if (tPos < 0) {
		tPos = ts.indexOf(' ');
	}

	QString time = (tPos >= 0 && tPos + 1 < ts.size()) ? ts.mid(tPos + 1) : ts;

	// Keep hh:mm:ss(.sss) and drop timezone/date leftovers if present.
	int zonePos = time.indexOf('Z');
	if (zonePos < 0) zonePos = time.indexOf('+');
	if (zonePos < 0) {
		int minusPos = time.indexOf('-', 1);
		if (minusPos > 0) zonePos = minusPos;
	}
	if (zonePos > 0) {
		time = time.left(zonePos);
	}

	return time.trimmed();
}

} // namespace

// A compact horizontal gauge: a track with a central "balanced" band and a marker
// whose position and colour encode the lag-1 autocorrelation. Left = under, right
// = over. Defined at file scope (matching the header's forward declaration); it has
// no signals/slots, so no Q_OBJECT / moc is needed.
class BalanceBar : public QWidget {
public:
	explicit BalanceBar(QWidget *parent = nullptr) : QWidget(parent) {
		setFixedSize(150, 20);
		setToolTip(QStringLiteral(
			"<b>Correction response</b> — how well the guide loop is tuned on this axis.<br><br>"
			"It is the <b>lag-1 autocorrelation of the residual error</b>: how each frame's "
			"error relates to the previous one. The marker shows where the loop sits:<br><br>"
			"• <b>Centre (green)</b> — errors are uncorrelated (white noise); each pulse cancels "
			"the error. Well tuned.<br>"
			"• <b>Left — over-correcting</b> — errors flip sign every frame; the loop overshoots "
			"and rings, often chasing seeing. Try lowering aggressiveness or raising the min-move.<br>"
			"• <b>Right — under-correcting</b> — errors persist in the same direction frame after "
			"frame. Aggressiveness too low (or max pulse too small); drift and periodic error "
			"leak into the residual. Try raising aggressiveness.<br><br>"
			"Colour: green = well tuned, amber = slightly off, red = clearly off. "
			"A little right/under bias is normal with short exposures (there is always some "
			"tracking lag) — judge by the colour and the marker's distance from centre."));
	}

	void setValue(double value, bool valid) {
		m_value = value;
		m_valid = valid;
		update();
	}

protected:
	void paintEvent(QPaintEvent *) override {
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing, true);

		const QRectF area = rect().adjusted(1, 1, -1, -1);
		const QRectF track(area.left(), area.center().y() - 4.0, area.width(), 8.0);
		const double clamp = kBalanceScale;
		auto toX = [&](double v) {
			const double c = std::max(-clamp, std::min(clamp, v));
			return track.left() + (0.5 + c / (2.0 * clamp)) * track.width();
		};

		p.setPen(Qt::NoPen);
		p.setBrush(QColor(70, 70, 74));
		p.drawRoundedRect(track, 4.0, 4.0);

		// Central "balanced" band.
		QRectF band(toX(-kBalanceOkThreshold), track.top(),
		            toX(kBalanceOkThreshold) - toX(-kBalanceOkThreshold), track.height());
		p.setBrush(QColor(55, 95, 60));
		p.drawRoundedRect(band, 4.0, 4.0);

		// Centre tick.
		p.setPen(QPen(QColor(140, 140, 145), 1.0));
		const double cx = toX(0.0);
		p.drawLine(QPointF(cx, track.top() - 1.0), QPointF(cx, track.bottom() + 1.0));

		if (!m_valid) {
			p.setPen(QColor(150, 150, 150));
			p.drawText(rect(), Qt::AlignCenter, QStringLiteral("n/a"));
			return;
		}

		// Negative lag-1 (over-correcting) sits on the LEFT, positive (under-
		// correcting) on the right — matching the "left = over, right = under" hint.
		const double x = toX(m_value);
		p.setBrush(balanceStatusColor(m_value));
		p.setPen(QPen(QColor(20, 20, 20), 1.0));
		p.drawEllipse(QPointF(x, track.center().y()), 6.0, 6.0);
	}

private:
	double m_value = 0.0;
	bool m_valid = false;
};

GuideLogViewerWindow::GuideLogViewerWindow(QWidget *parent) : QMainWindow(parent), m_selectedSessionIndex(-1) {
	setWindowTitle(tr("Ain INDIGO Guide Log Viewer"));
	resize(1400, 840);
	setWindowIcon(QIcon(":/resource/ain_guidelog_viewer.png"));
	setAcceptDrops(true);

	QFile f(":/resource/control_panel.qss");
	if (f.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream ts(&f);
		setStyleSheet(ts.readAll());
		f.close();
	}

	createUi();
	connectSignals();
}

void GuideLogViewerWindow::createUi() {
	QMenu *fileMenu = menuBar()->addMenu("&File");
	QAction *openAction = fileMenu->addAction("&Open Guiding Log...");
	openAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
	connect(openAction, &QAction::triggered, this, [this]() { openLogFileDialog(); });
	fileMenu->addSeparator();
	QAction *exitAction = fileMenu->addAction("E&xit");
	connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

	QWidget *central = new QWidget(this);
	setCentralWidget(central);

	QVBoxLayout *rootLayout = new QVBoxLayout(central);
	rootLayout->setContentsMargins(6, 6, 6, 6);
	rootLayout->setSpacing(6);

	QHBoxLayout *toolbarLayout = new QHBoxLayout();
	m_sessionCombo = new QComboBox(this);
	m_sessionCombo->setMinimumWidth(260);
	// Grow the combo (and its popup) to fit the whole session description.
	m_sessionCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_sessionCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_sessionCombo->addItem("No guiding sessions");
	m_sessionCombo->setEnabled(false);
	m_fileLabel = new QLabel("No file loaded");
	m_statusLabel = new QLabel("Load an Ain/INDIGO guiding log to begin.");
	m_statusLabel->setMinimumWidth(420);
	toolbarLayout->addWidget(new QLabel("Session:"));
	toolbarLayout->addWidget(m_sessionCombo);
	toolbarLayout->addWidget(m_fileLabel, 1);
	toolbarLayout->addWidget(m_statusLabel);
	rootLayout->addLayout(toolbarLayout);

	// --- Stats table (built first so we can size the top row to match it) ---
	m_statsModel = new QStandardItemModel(StatsRowCount, StatsColumnCount, central);
	m_statsModel->setHorizontalHeaderLabels({"Axis", "RMSE (\")", "Peak (\")", "RMSE (px)", "Peak (px)"});
	for (int row = 0; row < StatsRowCount; row++) {
		QStandardItem *name = new QStandardItem(kStatsRowNames[row]);
		name->setEditable(false);
		name->setForeground(QBrush(kStatsRowColors[row]));
		QFont nameFont = name->font();
		nameFont.setBold(true);
		name->setFont(nameFont);
		m_statsModel->setItem(row, StatsColumnAxis, name);
		for (int col = StatsColumnAxis + 1; col < StatsColumnCount; col++) {
			QStandardItem *value = new QStandardItem("n/a");
			value->setEditable(false);
			value->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			m_statsModel->setItem(row, col, value);
		}
	}

	m_statsTable = new QTableView(central);
	m_statsTable->setModel(m_statsModel);
	m_statsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_statsTable->setSelectionMode(QAbstractItemView::NoSelection);
	m_statsTable->setFocusPolicy(Qt::NoFocus);
	m_statsTable->verticalHeader()->setVisible(false);
	m_statsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	m_statsTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_statsTable->setMinimumWidth(360);
	const int statsRowHeight = m_statsTable->verticalHeader()->defaultSectionSize();
	const int topRowHeight = statsRowHeight * (StatsRowCount + 1) + 8;
	m_statsTable->setFixedHeight(topRowHeight);

	// Stats table and the exclude-dithering toggle share one framed box.
	m_excludeDitherCheck = new QCheckBox("Exclude dithering frames from stats", central);
	m_excludeDitherCheck->setChecked(true);

	QFrame *statsFrame = new QFrame(central);
	statsFrame->setFrameShape(QFrame::StyledPanel);
	QVBoxLayout *statsFrameLayout = new QVBoxLayout(statsFrame);
	statsFrameLayout->setContentsMargins(6, 6, 6, 6);
	statsFrameLayout->setSpacing(4);
	statsFrameLayout->addWidget(m_statsTable);
	statsFrameLayout->addWidget(m_excludeDitherCheck);

	const int frameHeight = topRowHeight + m_excludeDitherCheck->sizeHint().height() + 4 + 12;
	statsFrame->setFixedHeight(frameHeight);

	// --- Top row: guiding session header lines (left) + stats frame (right) ---
	m_metadataLabel = new QLabel("Session metadata will appear here.", central);
	m_metadataLabel->setWordWrap(true);
	m_metadataLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_metadataLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	QScrollArea *metadataScroll = new QScrollArea(central);
	metadataScroll->setWidgetResizable(true);
	metadataScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	metadataScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	metadataScroll->setWidget(m_metadataLabel);
	metadataScroll->setFixedHeight(frameHeight);

	QHBoxLayout *topInfoLayout = new QHBoxLayout();
	topInfoLayout->addWidget(metadataScroll, 1);
	topInfoLayout->addWidget(statsFrame);
	rootLayout->addLayout(topInfoLayout);

	// --- Correction-balance row: over/under-correction gauge per axis ---
	m_raBalanceBar = new BalanceBar(central);
	m_decBalanceBar = new BalanceBar(central);
	m_raBalanceLabel = new QLabel("n/a", central);
	m_decBalanceLabel = new QLabel("n/a", central);
	m_raBalanceLabel->setMinimumWidth(170);
	m_decBalanceLabel->setMinimumWidth(170);

	QFrame *balanceFrame = new QFrame(central);
	balanceFrame->setFrameShape(QFrame::StyledPanel);
	QHBoxLayout *balanceLayout = new QHBoxLayout(balanceFrame);
	balanceLayout->setContentsMargins(8, 4, 8, 4);
	balanceLayout->setSpacing(6);
	QLabel *balanceTitle = new QLabel("Correction response", central);
	{
		QFont f = balanceTitle->font();
		f.setBold(true);
		balanceTitle->setFont(f);
	}
	QLabel *balanceHint = new QLabel("(left = over corrected, right = under corrected)", central);
	balanceHint->setStyleSheet("color: #999;");
	QLabel *raName = new QLabel("RA", central);
	raName->setStyleSheet("color: #ff5050; font-weight: bold;");
	QLabel *decName = new QLabel("Dec", central);
	decName->setStyleSheet("color: #3caaf5; font-weight: bold;");
	balanceLayout->addWidget(balanceTitle);
	balanceLayout->addWidget(balanceHint);
	balanceLayout->addSpacing(14);
	balanceLayout->addWidget(raName);
	balanceLayout->addWidget(m_raBalanceBar);
	balanceLayout->addWidget(m_raBalanceLabel);
	balanceLayout->addSpacing(18);
	balanceLayout->addWidget(decName);
	balanceLayout->addWidget(m_decBalanceBar);
	balanceLayout->addWidget(m_decBalanceLabel);
	balanceLayout->addStretch(1);
	rootLayout->addWidget(balanceFrame);

	// --- Y column selector, above the graph ---
	m_yColumnsScroll = new QScrollArea(central);
	m_yColumnsScroll->setWidgetResizable(true);
	m_yColumnsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_yColumnsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_yColumnsScroll->setFixedHeight(48);
	m_yColumnsContainer = new QWidget(m_yColumnsScroll);
	m_yColumnsLayout = new QHBoxLayout(m_yColumnsContainer);
	m_yColumnsLayout->setContentsMargins(4, 4, 4, 4);
	m_yColumnsLayout->setSpacing(10);
	m_yColumnsLayout->addStretch();
	m_yColumnsScroll->setWidget(m_yColumnsContainer);
	rootLayout->addWidget(m_yColumnsScroll);

	m_statsSummaryLabel = new QLabel("Stats will appear here.", central);
	m_statsSummaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_statsSummaryLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	// Plot controls row, kept tight above the graph. The combo and spin boxes
	// are fixed to just fit their contents; the stats summary fills the rest.
	QHBoxLayout *xAxisLayout = new QHBoxLayout();
	QLabel *xAxisLabel = new QLabel("X Axis:");
	m_xAxisCombo = new QComboBox(central);
	m_xAxisCombo->setFixedWidth(130);
	m_yRangeSpin = new QSpinBox(central);
	m_yRangeSpin->setRange(0, 1000);
	m_yRangeSpin->setSingleStep(1);
	m_yRangeSpin->setValue(6);
	m_yRangeSpin->setSpecialValueText("Auto");
	m_yRangeSpin->setFixedWidth(70);
	m_xRangeSpin = new QSpinBox(central);
	m_xRangeSpin->setRange(0, 1000000);
	m_xRangeSpin->setSingleStep(100);
	m_xRangeSpin->setValue(600);
	m_xRangeSpin->setSpecialValueText("All");
	m_xRangeSpin->setFixedWidth(90);
	xAxisLayout->addWidget(xAxisLabel);
	xAxisLayout->addWidget(m_xAxisCombo);
	xAxisLayout->addSpacing(8);
	xAxisLayout->addWidget(new QLabel("Y Range:"));
	xAxisLayout->addWidget(m_yRangeSpin);
	xAxisLayout->addSpacing(8);
	xAxisLayout->addWidget(new QLabel("X Range:"));
	xAxisLayout->addWidget(m_xRangeSpin);
	xAxisLayout->addSpacing(12);
	xAxisLayout->addWidget(m_statsSummaryLabel, 1);
	m_peButton = new QPushButton("PE Reconstruction", central);
	m_peButton->setToolTip("Reconstruct the RA periodic error from the corrections and residual\n"
	                       "of the samples currently shown on the graph.");
	m_peButton->setEnabled(false);
	xAxisLayout->addWidget(m_peButton);
	rootLayout->addLayout(xAxisLayout);

	// --- Graph (grows vertically) ---
	m_plot = new SimplePlot(SimplePlot::Graph, central);
	m_plot->setPlotMargins(48, 12, 16, 28);
	m_plot->xAxis->setLabel("Sample index");
	m_plot->yAxis->setLabel("Value");
	m_plot->xAxis2->setVisible(true);
	m_plot->yAxis2->setVisible(true);
	m_plot->xAxis2->setTickLabels(true);
	m_plot->yAxis2->setTickLabels(true);
	rootLayout->addWidget(m_plot, 1);

	// --- Parsed data table, below the graph (grows vertically) ---
	m_tableView = new QTableView(central);
	m_tableModel = new QStandardItemModel(central);
	m_tableView->setModel(m_tableModel);
	m_tableView->setAlternatingRowColors(true);
	m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	// Columns are sized in fitDataColumns(): each keeps at least its natural
	// (content + header) width, and any spare width is shared out proportionally.
	m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
	m_tableView->viewport()->installEventFilter(this);
	// Use the same header/index font as the stats table for a consistent look.
	const QFont tableHeaderFont = m_statsTable->horizontalHeader()->font();
	m_tableView->horizontalHeader()->setFont(tableHeaderFont);
	m_tableView->verticalHeader()->setFont(tableHeaderFont);
	rootLayout->addWidget(m_tableView, 1);
}

void GuideLogViewerWindow::connectSignals() {
	connect(m_sessionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		applySelectedSession();
	});
	connect(m_xAxisCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
		updatePlot();
	});
	connect(m_yRangeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
		updatePlot();
	});
	connect(m_xRangeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
		updatePlot();
	});
	connect(m_excludeDitherCheck, &QCheckBox::toggled, this, [this](bool) {
		updatePlot();
	});
	connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
		updatePlot();
	});
	connect(m_peButton, &QPushButton::clicked, this, [this]() {
		openPeCurveWindow();
	});
}

void GuideLogViewerWindow::openLogFileDialog() {
	QString filePath = QFileDialog::getOpenFileName(
		this,
		"Open Ain/INDIGO Guiding Log",
		QString(),
		"Log files (*.log *.txt *.csv);;All files (*)"
	);
	if (!filePath.isEmpty()) {
		loadLogFile(filePath);
	}
}

bool GuideLogViewerWindow::loadLogFile(const QString &filePath) {
	QString error;
	QVector<GuideSession> sessions = GuideLogParser::parseFile(filePath, &error);
	if (sessions.isEmpty()) {
		QMessageBox::warning(this, "Load failed",
			error.isEmpty() ? QString("Failed to load %1").arg(filePath) : error);
		return false;
	}

	m_sessions = sessions;
	m_loadedPath = filePath;
	rebuildSessionSelector();
	applySelectedSession();

	m_fileLabel->setText(filePath);
	int totalRows = 0;
	for (const GuideSession &session : m_sessions) {
		totalRows += session.rows.size();
	}
	m_statusLabel->setText(QString("Loaded %1 guiding sessions, %2 total rows").arg(m_sessions.size()).arg(totalRows));

	return true;
}

void GuideLogViewerWindow::rebuildSessionSelector() {
	const QSignalBlocker blocker(m_sessionCombo);
	m_sessionCombo->clear();

	for (int i = 0; i < m_sessions.size(); i++) {
		m_sessionCombo->addItem(m_sessions[i].title, i);
	}

	m_sessionCombo->setEnabled(!m_sessions.isEmpty());
	if (!m_sessions.isEmpty()) {
		m_sessionCombo->setCurrentIndex(0);
	}
}

void GuideLogViewerWindow::applySelectedSession() {
	QString previousXAxisHeader;
	if (m_xAxisCombo->currentData().toInt() >= 0 && m_xAxisCombo->currentIndex() >= 0) {
		previousXAxisHeader = m_xAxisCombo->currentText();
	}

	QSet<QString> previousCheckedYHeaders;
	for (QCheckBox *checkBox : m_yColumnChecks) {
		if (checkBox->isChecked()) {
			previousCheckedYHeaders.insert(checkBox->text());
		}
	}

	QList<int> previousSelectedRows;
	if (m_tableView->selectionModel()) {
		for (const QModelIndex &index : m_tableView->selectionModel()->selectedRows()) {
			previousSelectedRows.append(index.row());
		}
	}

	int sessionIndex = m_sessionCombo->currentData().toInt();
	if (sessionIndex < 0 || sessionIndex >= m_sessions.size()) {
		m_selectedSessionIndex = -1;
		m_headers.clear();
		m_rows.clear();
		m_metadataLabel->setText("Session metadata will appear here.");
		m_peButton->setEnabled(false);
		rebuildTable();
		rebuildColumnSelectors();
		updatePlot();
		return;
	}

	m_selectedSessionIndex = sessionIndex;
	m_peButton->setEnabled(true);
	const GuideSession &session = m_sessions.at(sessionIndex);
	m_headers = session.headers;
	m_rows = session.rows;

	if (session.metadata.isEmpty()) {
		m_metadataLabel->setText("No metadata found for this guiding session.");
	} else {
		m_metadataLabel->setText(session.metadata.join("\n"));
	}

	rebuildTable();
	rebuildColumnSelectors();

	if (!previousXAxisHeader.isEmpty()) {
		const QSignalBlocker xBlocker(m_xAxisCombo);
		int xIndex = m_xAxisCombo->findText(previousXAxisHeader);
		if (xIndex >= 0) {
			m_xAxisCombo->setCurrentIndex(xIndex);
		}
	}

	if (!m_yColumnChecks.isEmpty()) {
		bool restoredAny = false;
		if (previousCheckedYHeaders.isEmpty()) {
			for (QCheckBox *checkBox : m_yColumnChecks) {
				const QSignalBlocker blocker(checkBox);
				checkBox->setChecked(false);
			}
		} else {
			for (QCheckBox *checkBox : m_yColumnChecks) {
				if (previousCheckedYHeaders.contains(checkBox->text())) {
					const QSignalBlocker blocker(checkBox);
					checkBox->setChecked(true);
					restoredAny = true;
				}
			}

			if (restoredAny) {
				for (QCheckBox *checkBox : m_yColumnChecks) {
					if (!previousCheckedYHeaders.contains(checkBox->text())) {
						const QSignalBlocker blocker(checkBox);
						checkBox->setChecked(false);
					}
				}
			}
		}
	}

	if (m_tableView->selectionModel()) {
		m_tableView->clearSelection();
		QItemSelectionModel *selectionModel = m_tableView->selectionModel();
		for (int row : previousSelectedRows) {
			if (row >= 0 && row < m_tableModel->rowCount()) {
				selectionModel->select(m_tableModel->index(row, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
			}
		}
	}

	updatePlot();
}

void GuideLogViewerWindow::rebuildTable() {
	m_tableModel->clear();
	m_tableModel->setColumnCount(m_headers.size());
	m_tableModel->setRowCount(m_rows.size());
	m_tableModel->setHorizontalHeaderLabels(m_headers);

	for (int row = 0; row < m_rows.size(); row++) {
		const QStringList &columns = m_rows.at(row);
		for (int col = 0; col < columns.size(); col++) {
			QStandardItem *item = new QStandardItem(columns.at(col));
			item->setEditable(false);
			m_tableModel->setItem(row, col, item);
		}
	}

	fitDataColumns();
}

// Sizes the data-table columns so every column keeps at least its natural width
// (the wider of its data and its header, so nothing is clipped) and any leftover
// horizontal space is shared out among all columns in proportion to that width.
void GuideLogViewerWindow::fitDataColumns() {
	if (m_fittingColumns) {
		return;
	}
	const int columnCount = m_tableModel->columnCount();
	if (columnCount <= 0) {
		return;
	}

	QHeaderView *header = m_tableView->horizontalHeader();

	m_fittingColumns = true;

	// Natural widths = the wider of content and header, obtained by letting Qt
	// resize to contents, then read back and switch to manual sizing.
	header->setSectionResizeMode(QHeaderView::ResizeToContents);
	QVector<int> natural(columnCount);
	long long total = 0;
	for (int col = 0; col < columnCount; col++) {
		natural[col] = header->sectionSize(col);
		total += natural[col];
	}
	header->setSectionResizeMode(QHeaderView::Interactive);
	if (total <= 0) {
		m_fittingColumns = false;
		return;
	}

	const int available = m_tableView->viewport()->width();
	if (available <= total) {
		// Not enough room: keep natural widths and let the view scroll.
		for (int col = 0; col < columnCount; col++) {
			header->resizeSection(col, natural[col]);
		}
	} else {
		const long long spare = available - total;
		long long distributed = 0;
		for (int col = 0; col < columnCount; col++) {
			const long long extra = (col == columnCount - 1)
				? (spare - distributed)
				: (spare * natural[col] / total);
			distributed += extra;
			header->resizeSection(col, natural[col] + static_cast<int>(extra));
		}
	}
	m_fittingColumns = false;
}

void GuideLogViewerWindow::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasUrls()) {
		const QList<QUrl> urls = event->mimeData()->urls();
		if (!urls.isEmpty() && urls.first().isLocalFile())
			event->acceptProposedAction();
	}
}

void GuideLogViewerWindow::dropEvent(QDropEvent *event) {
	const QList<QUrl> urls = event->mimeData()->urls();
	if (!urls.isEmpty()) {
		const QString filePath = urls.first().toLocalFile();
		if (!filePath.isEmpty())
			loadLogFile(filePath);
	}
}

bool GuideLogViewerWindow::eventFilter(QObject *watched, QEvent *event) {
	if (watched == m_tableView->viewport()) {
		if (event->type() == QEvent::Resize) {
			fitDataColumns();
		} else if (event->type() == QEvent::MouseButtonPress) {
			// A right-click clears the current row selection.
			QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
			if (mouseEvent->button() == Qt::RightButton) {
				m_tableView->clearSelection();
				return true;
			}
		}
	}
	return QMainWindow::eventFilter(watched, event);
}

void GuideLogViewerWindow::rebuildColumnSelectors() {
	m_numericColumns.clear();
	for (int col = 0; col < m_headers.size(); col++) {
		int validCount = 0;
		for (int row = 0; row < m_rows.size(); row++) {
			bool ok = false;
			m_rows.at(row).at(col).toDouble(&ok);
			if (ok) {
				validCount++;
				if (validCount >= 2) {
					break;
				}
			}
		}
		if (validCount >= 2) {
			m_numericColumns.append(col);
		}
	}

	m_xAxisCombo->blockSignals(true);
	m_xAxisCombo->clear();
	m_xAxisCombo->addItem("Sample index", -1);
	int timestampColumn = m_headers.indexOf("Timestamp");
	if (timestampColumn >= 0) {
		m_xAxisCombo->addItem("Timestamp", -2);
	}
	m_xAxisCombo->setCurrentIndex(0);
	m_xAxisCombo->blockSignals(false);

	for (QCheckBox *checkBox : m_yColumnChecks) {
		m_yColumnsLayout->removeWidget(checkBox);
		checkBox->deleteLater();
	}
	m_yColumnChecks.clear();

	QLayoutItem *item = nullptr;
	while ((item = m_yColumnsLayout->takeAt(0)) != nullptr) {
		delete item;
	}

	for (int col : m_numericColumns) {
		QCheckBox *checkBox = new QCheckBox(m_headers.at(col), m_yColumnsContainer);
		const QColor color = colorForGuideColumn(m_headers.at(col), col);
		checkBox->setProperty("column", col);
		checkBox->setStyleSheet(QString("QCheckBox { color: %1; } QCheckBox::indicator { margin-right: 4px; }").arg(color.name()));
		m_yColumnsLayout->addWidget(checkBox);
		m_yColumnChecks.append(checkBox);
		connect(checkBox, &QCheckBox::toggled, this, [this](bool) {
			updatePlot();
		});
	}

	m_yColumnsLayout->addStretch();

	bool anyChecked = false;
	for (QCheckBox *checkBox : m_yColumnChecks) {
		QString name = checkBox->text();
		if (name.startsWith("RA Dif") || name.startsWith("Dec Dif")) {
			checkBox->setChecked(true);
			anyChecked = true;
		}
	}

	if (!anyChecked && !m_yColumnChecks.isEmpty()) {
		m_yColumnChecks.at(0)->setChecked(true);
		if (m_yColumnChecks.size() > 1) {
			m_yColumnChecks.at(1)->setChecked(true);
		}
	}
}

void GuideLogViewerWindow::updatePlot() {
	m_plot->clearGraphs();
	m_plot->clearCustomXAxisTicks();

	if (m_rows.isEmpty()) {
		showStatsMessage("No data loaded.");
		m_plot->replot();
		syncPeWindow({});
		return;
	}

	const QList<int> selectedRows = selectedTableRows();
	const bool selectionOnlyMode = selectedRows.size() > 1;
	const bool verticalMarkerMode = selectedRows.size() == 1;
	const int selectedRow = selectedRows.isEmpty() ? -1 : selectedRows.at(selectedRows.size() / 2);

	const GuideColumns columns(m_headers);

	const bool excludeDither = m_excludeDitherCheck && m_excludeDitherCheck->isChecked();

	// Translate the current UI state into a unit-independent row filter. Dither
	// exclusion is applied to the statistics only, not to the plotted rows.
	GuideRowFilter filter;
	filter.selectionOnly = selectionOnlyMode;
	for (int row : selectedRows) {
		filter.selectedRows.insert(row);
	}

	const int xRangePoints = m_xRangeSpin->value();
	if (xRangePoints > 0 && !selectionOnlyMode) {
		const int windowSize = qMax(1, xRangePoints);
		int windowStart;
		int windowEnd;
		if (selectedRow >= 0) {
			// A single row is selected: centre the window on it.
			const int halfWindow = windowSize / 2;
			windowStart = selectedRow - halfWindow;
			windowEnd = windowStart + windowSize - 1;
			if (windowStart < 0) {
				windowStart = 0;
				windowEnd = qMin(m_rows.size() - 1, windowSize - 1);
			}
			if (windowEnd >= m_rows.size()) {
				windowEnd = m_rows.size() - 1;
				windowStart = qMax(0, windowEnd - windowSize + 1);
			}
		} else {
			// Nothing selected: show the first X-range values instead of all data.
			windowStart = 0;
			windowEnd = qMin(m_rows.size() - 1, windowSize - 1);
		}
		filter.useWindow = true;
		filter.windowStart = windowStart;
		filter.windowEnd = windowEnd;
	}

	const GuideRowSelection selection = GuideLogStats::filterRows(m_rows, columns, filter);

	// The graph shows every visible row (dither included). Statistics are taken
	// over the same rows minus dithering frames when the option is enabled.
	QVector<int> statsRows = selection.visibleRows;
	int ditherRowsExcluded = 0;
	if (excludeDither) {
		const GuideRowSelection statsSelection =
			GuideLogStats::excludeDitherRows(m_rows, selection.visibleRows, columns);
		statsRows = statsSelection.visibleRows;
		ditherRowsExcluded = statsSelection.ditherRowsExcluded;
	}

	// Draw the plot; only report statistics when something was actually plotted.
	if (renderPlot(selection.visibleRows, selectedRows, verticalMarkerMode)) {
		GuideStatsResult stats = GuideLogStats::compute(
			m_rows, statsRows, columns, m_rows.size(), ditherRowsExcluded);
		// "Rows shown" reflects what is on the graph, not the stats subset.
		stats.rowsShown = selection.visibleRows.size();
		showStats(stats);
	} else {
		showStatsMessage("No plotted points after applying the current filters.");
	}

	// Keep the PE window in step with the rows currently on the graph.
	syncPeWindow(selection.visibleRows);
}

double GuideLogViewerWindow::currentSessionCalibration() const {
	if (m_selectedSessionIndex < 0 || m_selectedSessionIndex >= m_sessions.size()) {
		return 0.0;
	}
	// Look for a "Calibration: RA = <n> px/s, ..." line in the session metadata.
	static const QRegularExpression re(
		"calibration.*\\bRA\\s*=\\s*([-+]?[0-9]*\\.?[0-9]+)\\s*px/s",
		QRegularExpression::CaseInsensitiveOption);
	for (const QString &line : m_sessions.at(m_selectedSessionIndex).metadata) {
		const QRegularExpressionMatch match = re.match(line);
		if (match.hasMatch()) {
			bool ok = false;
			const double value = match.captured(1).toDouble(&ok);
			if (ok && value > 0.0) {
				return value;
			}
		}
	}
	return 0.0;
}

void GuideLogViewerWindow::openPeCurveWindow() {
	if (!m_peWindow) {
		m_peWindow = new PECurveWindow(this);
		// Force a full session push (with calibration pre-fill) into the fresh
		// window. Once open it stays in sync via updatePlot()/syncPeWindow(), so
		// re-clicking the button just re-shows it without clobbering a
		// hand-entered calibration.
		m_pePushedSession = -1;
		updatePlot();
	}
	m_peWindow->show();
	m_peWindow->raise();
	m_peWindow->activateWindow();
}

void GuideLogViewerWindow::syncPeWindow(const QVector<int> &visibleRows) {
	if (!m_peWindow) {
		return;
	}

	QVector<QStringList> subset;
	subset.reserve(visibleRows.size());
	for (int row : visibleRows) {
		if (row >= 0 && row < m_rows.size()) {
			subset.append(m_rows.at(row));
		}
	}

	if (m_pePushedSession != m_selectedSessionIndex) {
		// New session: reset the window and pre-fill the calibration from the log.
		m_peWindow->setSession(m_headers, subset, currentSessionCalibration());
		m_pePushedSession = m_selectedSessionIndex;
	} else {
		// Same session, the graph window just changed: keep the user's calibration.
		m_peWindow->updateRows(m_headers, subset);
	}
}

QList<int> GuideLogViewerWindow::selectedTableRows() const {
	QList<int> rows;
	if (!m_tableView->selectionModel()) {
		return rows;
	}
	for (const QModelIndex &index : m_tableView->selectionModel()->selectedRows()) {
		rows.append(index.row());
	}
	std::sort(rows.begin(), rows.end());
	rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
	return rows;
}

bool GuideLogViewerWindow::renderPlot(const QVector<int> &visibleRows, const QList<int> &selectedRows, bool verticalMarkerMode) {
	const int xColumn = m_xAxisCombo->currentData().toInt();
	const int timestampColumn = m_headers.indexOf("Timestamp");
	const bool useTimestampXAxis = (xColumn == -2 && timestampColumn >= 0);

	double xMin = 0;
	double xMax = 0;
	double yMin = 0;
	double yMax = 0;
	bool hasAnyPoint = false;
	QStringList activeYLabels;

	for (QCheckBox *checkBox : m_yColumnChecks) {
		if (!checkBox->isChecked()) {
			continue;
		}

		int yColumn = checkBox->property("column").toInt();
		activeYLabels.append(checkBox->text());

		QVector<double> keys;
		QVector<double> values;
		keys.reserve(visibleRows.size());
		values.reserve(visibleRows.size());

		for (int row : visibleRows) {
			bool yOk = false;
			double y = m_rows.at(row).at(yColumn).toDouble(&yOk);
			if (!yOk) {
				continue;
			}

			double x = static_cast<double>(row);
			if (xColumn >= 0) {
				bool xOk = false;
				double parsedX = m_rows.at(row).at(xColumn).toDouble(&xOk);
				if (xOk) {
					x = parsedX;
				}
			}

			keys.append(x);
			values.append(y);

			if (!hasAnyPoint) {
				xMin = xMax = x;
				yMin = yMax = y;
				hasAnyPoint = true;
			} else {
				xMin = qMin(xMin, x);
				xMax = qMax(xMax, x);
				yMin = qMin(yMin, y);
				yMax = qMax(yMax, y);
			}
		}

		if (keys.isEmpty()) {
			continue;
		}

		SimpleGraph *graph = m_plot->addGraph();
		QPen pen(colorForGuideColumn(m_headers.at(yColumn), yColumn));
		pen.setWidth(1);
		graph->setPen(pen);
		graph->setData(keys, values);
		graph->setName(checkBox->text());
	}

	if (!hasAnyPoint) {
		m_plot->xAxis->setRange(0, 1);
		m_plot->yAxis->setRange(0, 1);
		m_plot->replot();
		return false;
	}

	double xSpan = xMax - xMin;
	double ySpan = yMax - yMin;
	double yPad = (ySpan > 0) ? ySpan * 0.08 : 1.0;
	double xAxisLower = xMin;
	double xAxisUpper = xMax;
	if (xSpan <= 0.0) {
		xAxisLower = xMin - 0.5;
		xAxisUpper = xMax + 0.5;
	}

	if (useTimestampXAxis) {
		if (!visibleRows.isEmpty()) {
			const int targetTickCount = qMax(2, m_plot->xAxis->autoTickCount());
			const int sourceCount = visibleRows.size();
			QVector<double> tickPositions;
			QStringList tickLabels;
			tickPositions.reserve(targetTickCount + 1);
			tickLabels.reserve(targetTickCount + 1);

			int previousRow = -1;
			for (int i = 0; i < targetTickCount; ++i) {
				int sourceIndex = (targetTickCount == 1) ? 0 : ((sourceCount - 1) * i) / (targetTickCount - 1);
				int row = visibleRows.at(sourceIndex);
				if (row == previousRow || row < 0 || row >= m_rows.size()) {
					continue;
				}
				previousRow = row;
				tickPositions.append(static_cast<double>(row));
				tickLabels.append(timeOnlyLabel(m_rows.at(row).at(timestampColumn)));
			}

			if (!tickPositions.isEmpty()) {
				m_plot->setCustomXAxisTicks(tickPositions, tickLabels);
			}
		}
	}

	double yAxisLower = yMin - yPad;
	double yAxisUpper = yMax + yPad;
	if (m_yRangeSpin->value() > 0) {
		yAxisLower = -m_yRangeSpin->value();
		yAxisUpper = m_yRangeSpin->value();
	}

	m_plot->xAxis->setRange(xAxisLower, xAxisUpper);
	m_plot->yAxis->setRange(yAxisLower, yAxisUpper);
	m_plot->xAxis2->setRange(xAxisLower, xAxisUpper);
	m_plot->yAxis2->setRange(yAxisLower, yAxisUpper);

	if (verticalMarkerMode && !selectedRows.isEmpty()) {
		int row = selectedRows.first();
		double markerX = static_cast<double>(row);
		if (xColumn >= 0) {
			bool xOk = false;
			double parsedX = m_rows.at(row).at(xColumn).toDouble(&xOk);
			if (xOk) {
				markerX = parsedX;
			}
		}

		SimpleGraph *marker = m_plot->addGraph();
		QPen markerPen(QColor(230, 230, 230, 180));
		markerPen.setWidth(1);
		markerPen.setStyle(Qt::DashLine);
		marker->setPen(markerPen);
		marker->setData(QVector<double>{markerX, markerX}, QVector<double>{yAxisLower, yAxisUpper});
		marker->setName("Selection");
	}

	QString xLabel;
	if (useTimestampXAxis) {
		xLabel = "Timestamp";
	} else {
		xLabel = (xColumn >= 0) ? m_headers.at(xColumn) : QString("Sample index");
	}
	m_plot->xAxis->setLabel(xLabel);
	m_plot->yAxis->setLabel(activeYLabels.isEmpty() ? QString("Value") : activeYLabels.join(" | "));

	m_plot->replot();
	return true;
}

void GuideLogViewerWindow::showStats(const GuideStatsResult &result) {
	m_statsSummaryLabel->setText(QString("Rows shown: %1 / %2").arg(result.rowsShown).arg(result.totalRows));

	const GuideAxisStats &px = result.pixels;
	const GuideAxisStats &arc = result.arcsec;
	const bool pxOk = px.isValid();
	const bool arcOk = arc.isValid();

	const QString arcUnit = "\"";
	const QString pxUnit = " px";

	m_statsModel->item(StatsRowRa, StatsColumnRmseArc)->setText(formatValue(arcOk, arc.raRmse, arcUnit));
	m_statsModel->item(StatsRowRa, StatsColumnPeakArc)->setText(formatValue(arcOk, arc.raPeak, arcUnit));
	m_statsModel->item(StatsRowRa, StatsColumnRmsePx)->setText(formatValue(pxOk, px.raRmse, pxUnit));
	m_statsModel->item(StatsRowRa, StatsColumnPeakPx)->setText(formatValue(pxOk, px.raPeak, pxUnit));

	m_statsModel->item(StatsRowDec, StatsColumnRmseArc)->setText(formatValue(arcOk, arc.decRmse, arcUnit));
	m_statsModel->item(StatsRowDec, StatsColumnPeakArc)->setText(formatValue(arcOk, arc.decPeak, arcUnit));
	m_statsModel->item(StatsRowDec, StatsColumnRmsePx)->setText(formatValue(pxOk, px.decRmse, pxUnit));
	m_statsModel->item(StatsRowDec, StatsColumnPeakPx)->setText(formatValue(pxOk, px.decPeak, pxUnit));

	m_statsModel->item(StatsRowTotal, StatsColumnRmseArc)->setText(formatValue(arcOk, arc.combinedRmse, arcUnit));
	m_statsModel->item(StatsRowTotal, StatsColumnPeakArc)->setText(formatValue(arcOk, arc.combinedPeak, arcUnit));
	m_statsModel->item(StatsRowTotal, StatsColumnRmsePx)->setText(formatValue(pxOk, px.combinedRmse, pxUnit));
	m_statsModel->item(StatsRowTotal, StatsColumnPeakPx)->setText(formatValue(pxOk, px.combinedPeak, pxUnit));

	showCorrectionBalance(result.balance);
}

void GuideLogViewerWindow::showCorrectionBalance(const CorrectionBalance &balance) {
	auto apply = [](BalanceBar *bar, QLabel *label, double value, bool valid) {
		bar->setValue(value, valid);
		label->setText(balanceVerdictText(value, valid));
		const QColor c = valid ? balanceStatusColor(value) : QColor(150, 150, 150);
		label->setStyleSheet(QString("color: %1;").arg(c.name()));
	};
	apply(m_raBalanceBar, m_raBalanceLabel, balance.raLag1, balance.raValid);
	apply(m_decBalanceBar, m_decBalanceLabel, balance.decLag1, balance.decValid);
}

void GuideLogViewerWindow::clearCorrectionBalance() {
	showCorrectionBalance(CorrectionBalance());
}

void GuideLogViewerWindow::showStatsMessage(const QString &message) {
	m_statsSummaryLabel->setText(message);
	clearStatsValues();
	clearCorrectionBalance();
}

void GuideLogViewerWindow::clearStatsValues() {
	for (int row = 0; row < StatsRowCount; row++) {
		for (int col = StatsColumnAxis + 1; col < StatsColumnCount; col++) {
			m_statsModel->item(row, col)->setText("n/a");
		}
	}
}
