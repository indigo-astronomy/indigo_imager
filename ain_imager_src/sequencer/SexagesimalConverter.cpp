#include "SexagesimalConverter.h"
#include <QDebug>
#include <cmath>

// Update regex patterns to handle whitespace
const QRegularExpression SexagesimalConverter::fullFormat(R"(^\s*([+-]?\d+):([0-5]?\d):([0-5]?\d)\.(\d+)\s*$)");
const QRegularExpression SexagesimalConverter::fullNoDecFormat(R"(^\s*([+-]?\d+):([0-5]?\d):([0-5]?\d)\s*$)");
const QRegularExpression SexagesimalConverter::shortFormat(R"(^\s*([+-]?\d+):(\d{1,2})\.(\d+)\s*$)");
const QRegularExpression SexagesimalConverter::shortNoDecFormat(R"(^\s*([+-]?\d+):([0-5]?\d)\s*$)");
const QRegularExpression SexagesimalConverter::decimalFormat(R"(^\s*([+-]?\d+)(\.(\d+))?\s*$)");

bool SexagesimalConverter::validateComponents(int degrees, int minutes, double seconds, QString& errorMsg) {
	Q_UNUSED(degrees);
	if (minutes > MAX_MINUTES) {
		errorMsg = QString("Minutes value %1 exceeds maximum of 59").arg(minutes);
		return false;
	}
	if (seconds >= 60.0) {
		errorMsg = QString("Seconds value %1 exceeds maximum of 59.999").arg(seconds);
		return false;
	}
	return true;
}

double SexagesimalConverter::stringToDouble(const QString& str, bool* ok) {
	QString errorMsg;
	if (ok) *ok = true;

	QString trimmed = str.trimmed();

	QRegularExpressionMatch match = fullFormat.match(trimmed);
	if (match.hasMatch()) {
		int deg = match.captured(1).toInt();
		int min = match.captured(2).toInt();
		double sec = match.captured(3).toDouble();
		double frac = match.captured(4).toDouble() / std::pow(10, match.captured(4).length());
		sec += frac;

		if (!validateComponents(deg, min, sec, errorMsg)) {
			if (ok) *ok = false;
			qDebug() << "Validation error:" << errorMsg;
			return 0.0;
		}

		// Preserve sign for zero degree values
		bool isNegative = str.startsWith('-');
		double value = std::abs(deg) + min/60.0 + sec/3600.0;
		return isNegative ? -value : value;
	}

	// Try no decimal format (dd:mm:ss)
	match = fullNoDecFormat.match(trimmed);
	if (match.hasMatch()) {
		int deg = match.captured(1).toInt();
		int min = match.captured(2).toInt();
		double sec = match.captured(3).toDouble();

		if (!validateComponents(deg, min, sec, errorMsg)) {
			if (ok) *ok = false;
			qDebug() << "Validation error:" << errorMsg;
			return 0.0;
		}

		// Preserve sign for zero degree values
		bool isNegative = str.startsWith('-');
		double value = std::abs(deg) + min/60.0 + sec/3600.0;
		return isNegative ? -value : value;
	}

	// Try short format with decimals (dd:mm.ff)
	match = shortFormat.match(trimmed);
	if (match.hasMatch()) {
		int deg = match.captured(1).toInt();
		int min = match.captured(2).toInt();
		double frac = match.captured(3).toDouble() / std::pow(10, match.captured(3).length());
		double sec = frac * 60.0;  // Convert decimal minutes to seconds

		if (!validateComponents(deg, min, sec, errorMsg)) {
			if (ok) *ok = false;
			qDebug() << "Validation error:" << errorMsg;
			return 0.0;
		}

		bool isNegative = trimmed.startsWith('-');
		double value = std::abs(deg) + min/60.0 + sec/3600.0;
		return isNegative ? -value : value;
	}

	// Try short format (dd:mm)
	match = shortNoDecFormat.match(trimmed);
	if (match.hasMatch()) {
		int deg = match.captured(1).toInt();
		int min = match.captured(2).toInt();

		if (!validateComponents(deg, min, 0, errorMsg)) {
			if (ok) *ok = false;
			qDebug() << "Validation error:" << errorMsg;
			return 0.0;
		}

		// Preserve sign for zero degree values
		bool isNegative = str.startsWith('-');
		double value = std::abs(deg) + min/60.0;
		return isNegative ? -value : value;
	}

	// Try decimal format (dd.ff)
	match = decimalFormat.match(trimmed);
	if (match.hasMatch()) {
		return str.toDouble(ok);
	}

	if (ok) *ok = false;
	return 0.0;
}

QString SexagesimalConverter::doubleToString(double value, int decimals, bool showPlusSign) {
	bool isNegative = std::signbit(value);
	double absValue = std::abs(value);

	int degrees = static_cast<int>(absValue);
	double minutesTotal = (absValue - degrees) * 60.0;
	int minutes = static_cast<int>(minutesTotal);
	double seconds = (minutesTotal - minutes) * 60.0;

	seconds = std::round(seconds * std::pow(10.0, decimals)) / std::pow(10.0, decimals);

	if (seconds >= 60.0) {
		minutes++;
		seconds = 0.0;
		if (minutes >= 60) {
			degrees++;
			minutes = 0;
		}
	}

	// Handle sign based on flag
	QString sign = isNegative ? "-" : (showPlusSign ? "+" : "");

	return QString("%1%2:%3:%4").arg(sign)
							   .arg(degrees, 2, 10, QChar('0'))
							   .arg(minutes, 2, 10, QChar('0'))
							   .arg(seconds, decimals + 3, 'f', decimals, QChar('0'));
}