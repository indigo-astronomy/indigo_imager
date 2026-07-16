// Copyright (c) 2026
// All rights reserved.

#ifndef GUIDELOGVIEWERWINDOW_H
#define GUIDELOGVIEWERWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QStringList>
#include <QList>

#include "guidelogparser.h"

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
class PECurveWindow;
class BalanceBar;

struct GuideAxisStats;
struct GuideStatsResult;

class GuideLogViewerWindow : public QMainWindow {
public:
	explicit GuideLogViewerWindow(QWidget *parent = nullptr);

protected:
	bool eventFilter(QObject *watched, QEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;

private:
	void createUi();
	void connectSignals();
	void openLogFileDialog();
	bool loadLogFile(const QString &filePath);
	void closeLogFile();
	void showAboutDialog();
	void rebuildSessionSelector();
	void applySelectedSession();
	void rebuildTable();
	void rebuildColumnSelectors();
	void fitDataColumns();
	void updatePlot();
	void openPeCurveWindow();
	// Pushes the rows currently visible on the graph into the PE window so its
	// reconstruction always matches what is displayed.
	void syncPeWindow(const QVector<int> &visibleRows);
	double currentSessionCalibration() const;

	QList<int> selectedTableRows() const;
	bool renderPlot(const QVector<int> &visibleRows, const QList<int> &selectedRows, bool verticalMarkerMode);
	void showStats(const GuideStatsResult &result);
	void showStatsMessage(const QString &message);
	void clearStatsValues();
	void showCorrectionBalance(const struct CorrectionBalance &balance);
	void clearCorrectionBalance();

	QLabel *m_fileLabel;
	QLabel *m_statusLabel;
	QComboBox *m_sessionCombo;
	QLabel *m_metadataLabel;

	QComboBox *m_xAxisCombo;
	QSpinBox *m_yRangeSpin;
	QSpinBox *m_xRangeSpin;
	QCheckBox *m_excludeDitherCheck;
	QPushButton *m_peButton;
	QLabel *m_statsSummaryLabel;
	BalanceBar *m_raBalanceBar;
	BalanceBar *m_decBalanceBar;
	QLabel *m_raBalanceLabel;
	QLabel *m_decBalanceLabel;
	QTableView *m_statsTable;
	QStandardItemModel *m_statsModel;
	QScrollArea *m_yColumnsScroll;
	QWidget *m_yColumnsContainer;
	QHBoxLayout *m_yColumnsLayout;
	QVector<QCheckBox *> m_yColumnChecks;
	QTableView *m_tableView;
	QStandardItemModel *m_tableModel;
	SimplePlot *m_plot;

	QString m_loadedPath;
	QVector<GuideSession> m_sessions;
	bool m_fittingColumns = false;
	int m_selectedSessionIndex;
	QStringList m_headers;
	QVector<QStringList> m_rows;
	QVector<int> m_numericColumns;
	PECurveWindow *m_peWindow = nullptr;
	int m_pePushedSession = -1;
};

#endif // GUIDELOGVIEWERWINDOW_H
