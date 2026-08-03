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

#include "guidelogtime.h"

#include <QDateTime>
#include <QString>
#include <QStringView>
#include <QTimeZone>

namespace {

// Strips surrounding whitespace and one layer of matching quotes, without
// copying the string.
QStringView unwrap(const QString &value) {
	QStringView v = QStringView(value).trimmed();
	if (v.size() >= 2 && (v.front() == u'"' || v.front() == u'\'') && v.back() == v.front()) {
		v = v.mid(1, v.size() - 2);
	}
	return v;
}

// Reads n decimal digits at offset off. Returns -1 if any of them is not a digit.
int digits(QStringView v, int off, int n) {
	int result = 0;
	for (int i = 0; i < n; i++) {
		const char16_t c = v.at(off + i).unicode();
		if (c < u'0' || c > u'9') {
			return -1;
		}
		result = result * 10 + int(c - u'0');
	}
	return result;
}

// Days from 1970-01-01 to the given proleptic Gregorian date (Howard Hinnant's
// days_from_civil). y/m/d are assumed already range-checked.
qint64 daysFromCivil(int y, int m, int d) {
	y -= (m <= 2);
	const qint64 era = (y >= 0 ? y : y - 399) / 400;
	const qint64 yoe = y - era * 400;                                  // [0, 399]
	const qint64 doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
	const qint64 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
	return era * 146097 + doe - 719468;
}

// Slow path: anything that does not match the fixed layout. The text is read as
// a bare wall-clock reading and re-stamped as UTC, matching the fast path.
QDateTime parseFallback(QStringView v) {
	const QString s = v.toString();
	QDateTime dt = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss.zzz");
	if (!dt.isValid()) {
		dt = QDateTime::fromString(s, "yyyy-MM-dd HH:mm:ss");
	}
	if (!dt.isValid()) {
		dt = QDateTime::fromString(s, Qt::ISODateWithMs);
	}
	if (!dt.isValid()) {
		return QDateTime();
	}
	return QDateTime(dt.date(), dt.time(), QTimeZone(QTimeZone::UTC));
}

} // namespace

bool GuideLogTime::parseMSecs(const QString &value, qint64 *msecs) {
	const QStringView v = unwrap(value);

	// "yyyy-MM-dd HH:mm:ss" is 19 characters; ".zzz" makes 23. The separator
	// between date and time is a space in the log format, but ISO 8601's 'T' is
	// accepted too since it costs nothing.
	if (v.size() >= 19 && v.at(4) == u'-' && v.at(7) == u'-' &&
	    (v.at(10) == u' ' || v.at(10) == u'T') && v.at(13) == u':' && v.at(16) == u':') {
		const int year = digits(v, 0, 4);
		const int month = digits(v, 5, 2);
		const int day = digits(v, 8, 2);
		const int hour = digits(v, 11, 2);
		const int minute = digits(v, 14, 2);
		const int second = digits(v, 17, 2);
		if (year >= 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31 &&
		    hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 60) {
			int milli = 0;
			if (v.size() >= 23 && v.at(19) == u'.') {
				milli = digits(v, 20, 3);
				if (milli < 0) {
					milli = 0;
				}
			}
			if (msecs) {
				const qint64 days = daysFromCivil(year, month, day);
				*msecs = ((days * 24 + hour) * 60 + minute) * 60000LL + second * 1000LL + milli;
			}
			return true;
		}
	}

	const QDateTime dt = parseFallback(v);
	if (!dt.isValid()) {
		return false;
	}
	if (msecs) {
		*msecs = dt.toMSecsSinceEpoch();
	}
	return true;
}

QDateTime GuideLogTime::parse(const QString &value) {
	qint64 msecs = 0;
	if (!parseMSecs(value, &msecs)) {
		return QDateTime();
	}
	return QDateTime::fromMSecsSinceEpoch(msecs, QTimeZone(QTimeZone::UTC));
}
