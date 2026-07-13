// Copyright (c) 2026
// All rights reserved.

#ifndef GUIDELOGPARSER_H
#define GUIDELOGPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>

// One guiding session parsed out of an Ain guiding log.
struct GuideSession {
	QString title;
	QStringList metadata;
	QStringList headers;
	QVector<QStringList> rows;
};

// UI-independent parser for Ain guiding logs. Turns a log file into a list of
// guiding sessions; all dialogs / status reporting stay in the window.
class GuideLogParser {
public:
	// Parses the file into sessions. On failure returns an empty list and, if
	// errorMessage is non-null, fills it with a human-readable reason.
	static QVector<GuideSession> parseFile(const QString &filePath, QString *errorMessage = nullptr);

private:
	static QStringList splitCsvLine(const QString &line);
	static bool isLikelyDataRow(const QStringList &columns);
	static QStringList sanitizeMetadataLines(const QStringList &lines, const QStringList &headers);
	static QString makeSessionTitle(int index, const GuideSession &session);
};

#endif // GUIDELOGPARSER_H
