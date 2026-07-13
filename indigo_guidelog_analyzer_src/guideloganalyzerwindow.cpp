// Copyright (c) 2026
// All rights reserved.

#include "guideloganalyzerwindow.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextStream>
#include <QVBoxLayout>
#include <QSet>

#include <algorithm>

#include <simpleplot.h>

namespace {

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

GuideLogAnalyzerWindow::GuideLogAnalyzerWindow(QWidget *parent) : QMainWindow(parent), m_selectedSessionIndex(-1) {
	setWindowTitle(tr("INDIGO Guide Log Analyzer"));
	resize(1400, 840);
	setWindowIcon(QIcon(":/resource/indigo_guidelog_analyzer.png"));

	QFile f(":/resource/control_panel.qss");
	if (f.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream ts(&f);
		setStyleSheet(ts.readAll());
		f.close();
	}

	createUi();
	connectSignals();
}

void GuideLogAnalyzerWindow::createUi() {
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
	m_openButton = new QPushButton("Open Log");
	m_sessionCombo = new QComboBox(this);
	m_sessionCombo->setMinimumWidth(260);
	m_sessionCombo->addItem("No guiding sessions");
	m_sessionCombo->setEnabled(false);
	m_fileLabel = new QLabel("No file loaded");
	m_statusLabel = new QLabel("Load an Ain guiding log to begin.");
	m_statusLabel->setMinimumWidth(420);
	toolbarLayout->addWidget(m_openButton);
	toolbarLayout->addWidget(new QLabel("Session:"));
	toolbarLayout->addWidget(m_sessionCombo);
	toolbarLayout->addWidget(m_fileLabel, 1);
	toolbarLayout->addWidget(m_statusLabel);
	rootLayout->addLayout(toolbarLayout);

	QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

	QWidget *leftPane = new QWidget(splitter);
	QVBoxLayout *leftLayout = new QVBoxLayout(leftPane);
	leftLayout->setContentsMargins(0, 0, 0, 0);
	leftLayout->setSpacing(6);

	QLabel *yColumnsLabel = new QLabel("Y Columns");
	m_yColumnsScroll = new QScrollArea(leftPane);
	m_yColumnsScroll->setWidgetResizable(true);
	m_yColumnsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_yColumnsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_yColumnsScroll->setFixedHeight(58);
	m_yColumnsContainer = new QWidget(m_yColumnsScroll);
	m_yColumnsLayout = new QHBoxLayout(m_yColumnsContainer);
	m_yColumnsLayout->setContentsMargins(4, 4, 4, 4);
	m_yColumnsLayout->setSpacing(10);
	m_yColumnsLayout->addStretch();
	m_yColumnsScroll->setWidget(m_yColumnsContainer);

	QLabel *tableLabel = new QLabel("Parsed Data Table");
	m_tableView = new QTableView(leftPane);
	m_tableModel = new QStandardItemModel(leftPane);
	m_tableView->setModel(m_tableModel);
	m_tableView->setAlternatingRowColors(true);
	m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
	m_tableView->horizontalHeader()->setStretchLastSection(true);

	leftLayout->addWidget(yColumnsLabel);
	leftLayout->addWidget(m_yColumnsScroll);
	leftLayout->addWidget(tableLabel);
	leftLayout->addWidget(m_tableView, 1);

	QWidget *rightPane = new QWidget(splitter);
	QVBoxLayout *rightLayout = new QVBoxLayout(rightPane);
	rightLayout->setContentsMargins(0, 0, 0, 0);
	rightLayout->setSpacing(6);

	QHBoxLayout *xAxisLayout = new QHBoxLayout();
	QLabel *xAxisLabel = new QLabel("X Axis:");
	m_xAxisCombo = new QComboBox(rightPane);
	m_yRangeSpin = new QSpinBox(rightPane);
	m_yRangeSpin->setRange(0, 1000);
	m_yRangeSpin->setSingleStep(1);
	m_yRangeSpin->setValue(6);
	m_yRangeSpin->setSpecialValueText("Auto");
	m_xRangeSpin = new QSpinBox(rightPane);
	m_xRangeSpin->setRange(0, 1000000);
	m_xRangeSpin->setSingleStep(100);
	m_xRangeSpin->setValue(200);
	m_xRangeSpin->setSpecialValueText("All");
	xAxisLayout->addWidget(xAxisLabel);
	xAxisLayout->addWidget(m_xAxisCombo, 1);
	xAxisLayout->addSpacing(8);
	xAxisLayout->addWidget(new QLabel("Y Range:"));
	xAxisLayout->addWidget(m_yRangeSpin);
	xAxisLayout->addSpacing(8);
	xAxisLayout->addWidget(new QLabel("X Range:"));
	xAxisLayout->addWidget(m_xRangeSpin);

	m_metadataLabel = new QLabel("Session metadata will appear here.", rightPane);
	m_metadataLabel->setWordWrap(true);
	m_metadataLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_metadataLabel->setMinimumHeight(48);

	m_plot = new SimplePlot(SimplePlot::Graph, rightPane);
	m_plot->setPlotMargins(48, 12, 16, 28);
	m_plot->xAxis->setLabel("Sample index");
	m_plot->yAxis->setLabel("Value");
	m_plot->xAxis2->setVisible(true);
	m_plot->yAxis2->setVisible(true);
	m_plot->xAxis2->setTickLabels(true);
	m_plot->yAxis2->setTickLabels(true);

	rightLayout->addLayout(xAxisLayout);
	rightLayout->addWidget(m_metadataLabel);
	rightLayout->addWidget(m_plot, 1);

	splitter->addWidget(leftPane);
	splitter->addWidget(rightPane);
	splitter->setStretchFactor(0, 45);
	splitter->setStretchFactor(1, 55);
	rootLayout->addWidget(splitter, 1);
}

void GuideLogAnalyzerWindow::connectSignals() {
	connect(m_openButton, &QPushButton::clicked, this, [this]() { openLogFileDialog(); });
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
	connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() {
		updatePlot();
	});
}

void GuideLogAnalyzerWindow::openLogFileDialog() {
	QString filePath = QFileDialog::getOpenFileName(
		this,
		"Open Ain Guiding Log",
		QString(),
		"Log files (*.log *.txt);;All files (*)"
	);
	if (!filePath.isEmpty()) {
		loadLogFile(filePath);
	}
}

QStringList GuideLogAnalyzerWindow::splitCsvLine(const QString &line) {
	QStringList columns = line.split(',', Qt::KeepEmptyParts);
	for (QString &column : columns) {
		column = column.trimmed();
	}
	return columns;
}

bool GuideLogAnalyzerWindow::isLikelyDataRow(const QStringList &columns) {
	int numericCount = 0;
	for (int i = 1; i < columns.size(); i++) {
		bool ok = false;
		columns.at(i).toDouble(&ok);
		if (ok) {
			numericCount++;
		}
	}
	return numericCount >= 3;
}

bool GuideLogAnalyzerWindow::loadLogFile(const QString &filePath) {
	QFile file(filePath);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		QMessageBox::warning(this, "Open failed", QString("Failed to open %1").arg(filePath));
		return false;
	}

	m_sessions.clear();
	GuideSession currentSession;
	bool inGuidingSession = false;
	bool expectingDataRows = false;

	auto finalizeSession = [this](GuideSession &session) {
		if (!session.rows.isEmpty()) {
			session.metadata = sanitizeMetadataLines(session.metadata, session.headers);
			m_sessions.append(session);
		}
		session = GuideSession();
	};

	QTextStream stream(&file);
	while (!stream.atEnd()) {
		QString line = stream.readLine().trimmed();
		if (line.isEmpty()) {
			continue;
		}

		QString lower = line.toLower();

		if (lower.contains("guiding started")) {
			if (inGuidingSession) {
				finalizeSession(currentSession);
			}
			inGuidingSession = true;
			expectingDataRows = false;
			currentSession = GuideSession();
			currentSession.metadata.append(line);
			continue;
		}

		if (lower.contains("guiding finished")) {
			if (inGuidingSession) {
				currentSession.metadata.append(line);
				finalizeSession(currentSession);
			}
			inGuidingSession = false;
			expectingDataRows = false;
			continue;
		}

		// Files can omit explicit start/finish markers. In that case treat
		// the whole block as one session (or until next explicit start line).
		if (!inGuidingSession) {
			inGuidingSession = true;
			currentSession = GuideSession();
			expectingDataRows = false;
		}

		if (line.startsWith("Timestamp,")) {
			currentSession.headers = splitCsvLine(line);
			expectingDataRows = true;
			continue;
		}

		if (!expectingDataRows || currentSession.headers.isEmpty()) {
			currentSession.metadata.append(line);
			continue;
		}

		QStringList columns = splitCsvLine(line);
		if (columns.size() == currentSession.headers.size() && isLikelyDataRow(columns)) {
			currentSession.rows.append(columns);
			continue;
		}

		currentSession.metadata.append(line);
	}

	if (inGuidingSession) {
		finalizeSession(currentSession);
	}

	if (m_sessions.isEmpty()) {
		QMessageBox::warning(this, "Parse failed", "No guiding data rows were found in the selected file.");
		return false;
	}

	for (int i = 0; i < m_sessions.size(); i++) {
		m_sessions[i].title = makeSessionTitle(i, m_sessions[i]);
	}

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

QStringList GuideLogAnalyzerWindow::sanitizeMetadataLines(const QStringList &lines, const QStringList &headers) {
	QStringList sanitized;
	for (const QString &line : lines) {
		if (line.startsWith("Timestamp,")) {
			continue;
		}

		QStringList columns = splitCsvLine(line);
		if (!headers.isEmpty() && columns.size() == headers.size() && isLikelyDataRow(columns)) {
			continue;
		}
		sanitized.append(line);
	}
	return sanitized;
}

void GuideLogAnalyzerWindow::rebuildSessionSelector() {
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

void GuideLogAnalyzerWindow::applySelectedSession() {
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
		rebuildTable();
		rebuildColumnSelectors();
		updatePlot();
		return;
	}

	m_selectedSessionIndex = sessionIndex;
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

void GuideLogAnalyzerWindow::rebuildTable() {
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

	m_tableView->resizeColumnsToContents();
}

void GuideLogAnalyzerWindow::rebuildColumnSelectors() {
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
	for (int col : m_numericColumns) {
		m_xAxisCombo->addItem(m_headers.at(col), col);
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

void GuideLogAnalyzerWindow::updatePlot() {
	m_plot->clearGraphs();
	m_plot->clearCustomXAxisTicks();

	if (m_rows.isEmpty()) {
		m_plot->replot();
		return;
	}

	QList<int> selectedRows;
	for (const QModelIndex &index : m_tableView->selectionModel()->selectedRows()) {
		selectedRows.append(index.row());
	}

	std::sort(selectedRows.begin(), selectedRows.end());
	selectedRows.erase(std::unique(selectedRows.begin(), selectedRows.end()), selectedRows.end());

	QSet<int> selectedRowSet;
	for (int row : selectedRows) {
		selectedRowSet.insert(row);
	}

	const bool selectionOnlyMode = selectedRows.size() > 1;
	const bool verticalMarkerMode = selectedRows.size() == 1;
	const int selectedRow = selectedRows.isEmpty() ? -1 : selectedRows.at(selectedRows.size() / 2);
	const int xRangePoints = m_xRangeSpin->value();
	const bool xWindowMode = (xRangePoints > 0 && selectedRow >= 0 && !selectionOnlyMode);
	int xWindowStart = 0;
	int xWindowEnd = m_rows.size() - 1;
	if (xWindowMode) {
		const int windowSize = qMax(1, xRangePoints);
		const int halfWindow = windowSize / 2;
		xWindowStart = selectedRow - halfWindow;
		xWindowEnd = xWindowStart + windowSize - 1;
		if (xWindowStart < 0) {
			xWindowStart = 0;
			xWindowEnd = qMin(m_rows.size() - 1, windowSize - 1);
		}
		if (xWindowEnd >= m_rows.size()) {
			xWindowEnd = m_rows.size() - 1;
			xWindowStart = qMax(0, xWindowEnd - windowSize + 1);
		}
	}

	int xColumn = m_xAxisCombo->currentData().toInt();
	int timestampColumn = m_headers.indexOf("Timestamp");
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
		keys.reserve(m_rows.size());
		values.reserve(m_rows.size());

		for (int row = 0; row < m_rows.size(); row++) {
			if (xWindowMode && (row < xWindowStart || row > xWindowEnd)) {
				continue;
			}

			if (selectionOnlyMode && !selectedRowSet.contains(row)) {
				continue;
			}

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
		return;
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
		QList<int> candidateRows;
		if (selectionOnlyMode) {
			candidateRows = selectedRows;
		} else if (xWindowMode) {
			candidateRows.reserve(xWindowEnd - xWindowStart + 1);
			for (int i = xWindowStart; i <= xWindowEnd; ++i) {
				candidateRows.append(i);
			}
		} else {
			candidateRows.reserve(m_rows.size());
			for (int i = 0; i < m_rows.size(); ++i) {
				candidateRows.append(i);
			}
		}

		if (!candidateRows.isEmpty()) {
			const int targetTickCount = qMax(2, m_plot->xAxis->autoTickCount());
			const int sourceCount = candidateRows.size();
			QVector<double> tickPositions;
			QStringList tickLabels;
			tickPositions.reserve(targetTickCount + 1);
			tickLabels.reserve(targetTickCount + 1);

			int previousRow = -1;
			for (int i = 0; i < targetTickCount; ++i) {
				int sourceIndex = (targetTickCount == 1) ? 0 : ((sourceCount - 1) * i) / (targetTickCount - 1);
				int row = candidateRows.at(sourceIndex);
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

	if (verticalMarkerMode) {
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
}

QString GuideLogAnalyzerWindow::makeSessionTitle(int index, const GuideSession &session) {
	if (session.rows.isEmpty()) {
		return QString("Guiding %1").arg(index + 1);
	}

	int timestampColumn = session.headers.indexOf("Timestamp");
	if (timestampColumn >= 0) {
		QString firstTimestamp = session.rows.first().at(timestampColumn);
		QString lastTimestamp = session.rows.last().at(timestampColumn);
		return QString("Guiding %1 (%2 to %3)").arg(index + 1).arg(firstTimestamp).arg(lastTimestamp);
	}

	return QString("Guiding %1 (%2 rows)").arg(index + 1).arg(session.rows.size());
}
