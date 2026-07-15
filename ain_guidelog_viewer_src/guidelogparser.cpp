// Copyright (c) 2026
// All rights reserved.

#include "guidelogparser.h"

#include <QDateTime>
#include <QFile>
#include <QTextStream>

namespace {

// Parses a log timestamp ("yyyy-MM-dd HH:mm:ss.zzz", optionally quoted). Returns
// an invalid QDateTime on failure.
QDateTime parseLogTimestamp(QString value) {
	value = value.trimmed();
	if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"')) {
		value = value.mid(1, value.size() - 2);
	}
	QDateTime dt = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss.zzz");
	if (!dt.isValid()) {
		dt = QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss");
	}
	return dt;
}

// Formats a duration in seconds as "1h 05m", "12m 34s" or "45s".
QString formatDuration(qint64 seconds) {
	if (seconds < 0) {
		seconds = 0;
	}
	const qint64 h = seconds / 3600;
	const qint64 m = (seconds % 3600) / 60;
	const qint64 s = seconds % 60;
	if (h > 0) {
		return QString("%1h %2m").arg(h).arg(m, 2, 10, QChar('0'));
	}
	if (m > 0) {
		return QString("%1m %2s").arg(m).arg(s, 2, 10, QChar('0'));
	}
	return QString("%1s").arg(s);
}

} // namespace

// RFC 4180 style CSV line splitter, tolerant of the older log format.
//
// A field may be quoted with either double or single quotes (older logs used
// single quotes; current logs use double). Whichever quote opens a field also
// closes it, and a doubled quote inside that field is a literal quote (e.g. the
// arc-second unit in "RA Dif("")" -> RA Dif(")). Commas inside a quoted field
// are not separators. Unquoted fields are taken verbatim, so an unquoted header
// like Timestamp,RA Dif(") still parses correctly.
QStringList GuideLogParser::splitCsvLine(const QString &line) {
	QStringList columns;
	QString field;
	QChar quoteChar; // null unless currently inside a quoted field
	for (int i = 0; i < line.size(); i++) {
		const QChar c = line.at(i);
		if (!quoteChar.isNull()) {
			if (c == quoteChar) {
				if (i + 1 < line.size() && line.at(i + 1) == quoteChar) {
					field.append(c); // doubled quote -> literal quote
					i++;
				} else {
					quoteChar = QChar();
				}
			} else {
				field.append(c);
			}
		} else if ((c == '"' || c == '\'') && field.trimmed().isEmpty()) {
			quoteChar = c; // quote only opens a field at its start
		} else if (c == ',') {
			columns.append(field.trimmed());
			field.clear();
		} else {
			field.append(c);
		}
	}
	columns.append(field.trimmed());
	return columns;
}

bool GuideLogParser::isHeaderLine(const QString &line) {
	// The header row may or may not quote its fields, so compare the first
	// column after CSV splitting (which strips quotes) rather than the raw text.
	QStringList columns = splitCsvLine(line);
	return !columns.isEmpty() && columns.first() == "Timestamp";
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
		if (isHeaderLine(line)) {
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
	QString kind = session.kind.isEmpty() ? QString("Guiding") : session.kind;

	if (session.rows.isEmpty()) {
		return QString("%1 %2").arg(kind).arg(index + 1);
	}

	const int rowCount = session.rows.size();
	int timestampColumn = session.headers.indexOf("Timestamp");
	if (timestampColumn >= 0) {
		const QDateTime start = parseLogTimestamp(session.rows.first().at(timestampColumn));
		const QDateTime end = parseLogTimestamp(session.rows.last().at(timestampColumn));
		if (start.isValid() && end.isValid()) {
			const QString startStr = start.toString("yyyy-MM-dd HH:mm:ss");
			const QString durationStr = formatDuration(start.secsTo(end));
			return QString("%1 %2 · %3 · %4 · %5 pts")
			    .arg(kind).arg(index + 1).arg(startStr).arg(durationStr).arg(rowCount);
		}
	}

	return QString("%1 %2 · %3 pts").arg(kind).arg(index + 1).arg(rowCount);
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

		// Marker matching is case insensitive. A session starts with either a
		// guiding or a calibration "started" line and ends with the matching
		// "finished"/"ended" line.
		bool calibrationStart = lower.contains("calibration started");
		bool guidingStart = lower.contains("guiding started");
		if (guidingStart || calibrationStart) {
			if (inGuidingSession) {
				finalizeSession(currentSession);
			}
			inGuidingSession = true;
			expectingDataRows = false;
			currentSession = GuideSession();
			currentSession.kind = calibrationStart ? "Calibration" : "Guiding";
			currentSession.metadata.append(line);
			continue;
		}

		bool sessionFinished =
			lower.contains("guiding finished") || lower.contains("guiding ended") ||
			lower.contains("calibration finished") || lower.contains("calibration ended");
		if (sessionFinished) {
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

		if (isHeaderLine(line)) {
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
