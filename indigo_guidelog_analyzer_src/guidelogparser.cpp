// Copyright (c) 2026
// All rights reserved.

#include "guidelogparser.h"

#include <QFile>
#include <QTextStream>

QStringList GuideLogParser::splitCsvLine(const QString &line) {
	QStringList columns = line.split(',', Qt::KeepEmptyParts);
	for (QString &column : columns) {
		column = column.trimmed();
	}
	return columns;
}

bool GuideLogParser::isLikelyDataRow(const QStringList &columns) {
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

QStringList GuideLogParser::sanitizeMetadataLines(const QStringList &lines, const QStringList &headers) {
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

QString GuideLogParser::makeSessionTitle(int index, const GuideSession &session) {
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

QVector<GuideSession> GuideLogParser::parseFile(const QString &filePath, QString *errorMessage) {
	QVector<GuideSession> sessions;

	QFile file(filePath);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		if (errorMessage) {
			*errorMessage = QString("Failed to open %1").arg(filePath);
		}
		return sessions;
	}

	GuideSession currentSession;
	bool inGuidingSession = false;
	bool expectingDataRows = false;

	auto finalizeSession = [&sessions](GuideSession &session) {
		if (!session.rows.isEmpty()) {
			session.metadata = sanitizeMetadataLines(session.metadata, session.headers);
			sessions.append(session);
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

	if (sessions.isEmpty()) {
		if (errorMessage) {
			*errorMessage = "No guiding data rows were found in the selected file.";
		}
		return sessions;
	}

	for (int i = 0; i < sessions.size(); i++) {
		sessions[i].title = makeSessionTitle(i, sessions[i]);
	}

	return sessions;
}
