#ifndef __SEXAGESIMALCONVERTER_H
#define __SEXAGESIMALCONVERTER_H

#include <QString>
#include <QRegularExpression>

class SexagesimalConverter {
public:
	static double stringToDouble(const QString& str, bool* ok = nullptr);
	static QString doubleToString(double value, int decimals = 2, bool showPlusSign = false);

private:
	static bool validateComponents(int degrees, int minutes, double seconds, QString& errorMsg);
	static const QRegularExpression fullFormat;    // dd:mm:ss.ff
	static const QRegularExpression fullNoDecFormat;   // dd:mm:ss
	static const QRegularExpression shortFormat;  // dd:mm.ff
	static const QRegularExpression shortNoDecFormat;   // dd:mm
	static const QRegularExpression decimalFormat; // dd.ff

	static const int MAX_MINUTES = 59;
	static const int MAX_SECONDS = 59;
};

#endif // __SEXAGESIMALCONVERTER_H