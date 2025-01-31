#ifndef __INDIGOSEQUENCEPARSER_H
#define __INDIGOSEQUENCEPARSER_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QObject>

struct FunctionCall {
	QString objectName;
	QString functionName;
	QStringList parameters;
	QVector<FunctionCall> nestedCalls;
};

class IndigoSequenceParser : public QObject {
	Q_OBJECT
public:
	QVector<FunctionCall> parse(QString code) const;
	bool validateCalls(const QVector<FunctionCall>& calls) const;
	QString generate(const QVector<FunctionCall>& calls, int indent = 0) const;

signals:
	void validationError(QString errorMessage) const;
};

#endif // __INDIGOSEQUENCEPARSER_H