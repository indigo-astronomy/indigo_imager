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

#ifndef GUIDELOGPARSER_H
#define GUIDELOGPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>

// One session parsed out of an Ain guiding log. A session is either a guiding
// run or a calibration run, distinguished by its start/finish markers.
struct GuideSession {
	QString title;
	QString kind = "Guiding"; // "Guiding" or "Calibration"
	QStringList metadata;
	QStringList headers;
	QVector<QStringList> rows;
};

// Parser for Ain / IDIGO guiding logs. Turns a log file into a list of
// guiding sessions; all dialogs / status reporting stay in the window.
class GuideLogParser {
public:
	// Parses the file into sessions. On failure returns an empty list and, if
	// errorMessage is non-null, fills it with a human-readable reason.
	static QVector<GuideSession> parseFile(const QString &filePath, QString *errorMessage = nullptr);

private:
	static QStringList splitCsvLine(const QString &line);
	static bool isHeaderLine(const QString &line);
	static bool isLikelyDataRow(const QStringList &columns);
	static QStringList sanitizeMetadataLines(const QStringList &lines, const QStringList &headers);
	static QString makeSessionTitle(int index, const GuideSession &session);
};

#endif // GUIDELOGPARSER_H
