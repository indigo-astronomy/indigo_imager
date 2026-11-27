// Copyright (c) 2026 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "SchedulerTask.h"

#include <QRegularExpression>

const QStringList& SchedulerTask::knownKeys() {
	static const QStringList keys = {
		"task", "priority", "target", "altitude", "airmass", "ha", "moon", "illumination",
		"twilight", "window", "duration", "repeat", "retry", "preempted", "enabled",
		"folder", "observer", "pi", "project"
	};
	return keys;
}

SchedulerTask SchedulerTask::fromText(const QString& text) {
	static const QRegularExpression headerLine(R"(^\s*//\s*@([A-Za-z_]+)(?:[ \t]+(.*))?$)");
	static const QRegularExpression emptyComment(R"(^\s*//\s*$)");

	SchedulerTask task;
	QString source = text;
	if (source.startsWith(QChar(0xFEFF))) {
		source.remove(0, 1);
	}
	QStringList lines = source.split('\n');
	int line = 0;
	for (; line < lines.size(); line++) {
		QString current = lines[line];
		if (current.endsWith('\r')) {
			current.chop(1);
		}
		if (emptyComment.match(current).hasMatch()) {
			continue;
		}
		QRegularExpressionMatch match = headerLine.match(current);
		if (!match.hasMatch()) {
			break;
		}
		task.m_header.append(qMakePair(match.captured(1).toLower(), match.captured(2).trimmed()));
	}
	QStringList body = lines.mid(line);
	// the blank line that ends the header is written again by toText()
	while (!body.isEmpty() && body.first().trimmed().isEmpty()) {
		body.removeFirst();
	}
	task.m_body = body.join('\n');
	return task;
}

QString SchedulerTask::toText() const {
	int width = 0;
	for (const auto& entry : m_header) {
		width = qMax(width, entry.first.length());
	}
	QString text;
	for (const auto& entry : m_header) {
		QString line = "// @" + entry.first.leftJustified(width) + " " + entry.second;
		text += line.trimmed() + "\n";
	}
	if (!m_header.isEmpty()) {
		text += "\n";
	}
	text += m_body;
	if (!text.endsWith('\n')) {
		text += "\n";
	}
	return text;
}

QString SchedulerTask::value(const QString& key) const {
	for (const auto& entry : m_header) {
		if (entry.first == key) {
			return entry.second;
		}
	}
	return QString();
}

bool SchedulerTask::has(const QString& key) const {
	for (const auto& entry : m_header) {
		if (entry.first == key) {
			return true;
		}
	}
	return false;
}

void SchedulerTask::set(const QString& key, const QString& value) {
	for (auto& entry : m_header) {
		if (entry.first == key) {
			entry.second = value;
			return;
		}
	}
	m_header.append(qMakePair(key, value));
}

void SchedulerTask::remove(const QString& key) {
	for (int i = 0; i < m_header.size(); i++) {
		if (m_header[i].first == key) {
			m_header.removeAt(i);
			return;
		}
	}
}

QStringList scheduler_label_columns(const QString& label, int count) {
	QStringList columns;
	int start = 0;
	for (int i = 0; i < count - 1; i++) {
		int separator = label.indexOf(" | ", start);
		if (separator < 0) {
			break;
		}
		columns.append(label.mid(start, separator - start));
		start = separator + 3;
	}
	columns.append(label.mid(start));
	while (columns.size() < count) {
		columns.append("-");
	}
	for (QString& column : columns) {
		column = column.trimmed();
		if (column == "-") {
			column.clear();
		}
	}
	return columns;
}
