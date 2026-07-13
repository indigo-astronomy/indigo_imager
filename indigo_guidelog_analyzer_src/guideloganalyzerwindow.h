// Copyright (c) 2026
// All rights reserved.

#ifndef GUIDELOGANALYZERWINDOW_H
#define GUIDELOGANALYZERWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QStringList>

class QLabel;
class QComboBox;
class QCheckBox;
class QHBoxLayout;
class QScrollArea;
class QTableView;
class QStandardItemModel;
class QPushButton;
class QSpinBox;
class SimplePlot;

struct GuideSession {
	QString title;
	QStringList metadata;
	QStringList headers;
	QVector<QStringList> rows;
};

class GuideLogAnalyzerWindow : public QMainWindow {
public:
	explicit GuideLogAnalyzerWindow(QWidget *parent = nullptr);

private:
	void createUi();
	void connectSignals();
	void openLogFileDialog();
	bool loadLogFile(const QString &filePath);
	void rebuildSessionSelector();
	void applySelectedSession();
	void rebuildTable();
	void rebuildColumnSelectors();
	void updatePlot();

	static QStringList splitCsvLine(const QString &line);
	static bool isLikelyDataRow(const QStringList &columns);
	static QStringList sanitizeMetadataLines(const QStringList &lines, const QStringList &headers);
	static QString makeSessionTitle(int index, const GuideSession &session);

	QPushButton *m_openButton;
	QLabel *m_fileLabel;
	QLabel *m_statusLabel;
	QComboBox *m_sessionCombo;
	QLabel *m_metadataLabel;

	QComboBox *m_xAxisCombo;
	QSpinBox *m_yRangeSpin;
	QSpinBox *m_xRangeSpin;
	QScrollArea *m_yColumnsScroll;
	QWidget *m_yColumnsContainer;
	QHBoxLayout *m_yColumnsLayout;
	QVector<QCheckBox *> m_yColumnChecks;
	QTableView *m_tableView;
	QStandardItemModel *m_tableModel;
	SimplePlot *m_plot;

	QString m_loadedPath;
	QVector<GuideSession> m_sessions;
	int m_selectedSessionIndex;
	QStringList m_headers;
	QVector<QStringList> m_rows;
	QVector<int> m_numericColumns;
};

#endif // GUIDELOGANALYZERWINDOW_H
