#ifndef __SEQUENCEITEMMODEL_H
#define __SEQUENCEITEMMODEL_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QPair>
#include <QObject>

class SequenceItemModel : public QObject {
	Q_OBJECT
public:
	struct ParameterInfo {
		QString label;
		QString widgetType;
		QStringList comboOptions;
		QPair<double, double> numericRange;
		double numericIncrement;

		ParameterInfo(const QString& l = "", const QString& w = "") : label(l), widgetType(w), numericRange(0.0, 100.0), numericIncrement(1.0) {}
	};

	struct WidgetTypeInfo {
		QString label;
		QMap<int, ParameterInfo> parameters;
	};

	static SequenceItemModel& instance();

	// Delete copy constructor and assignment operator
	SequenceItemModel(const SequenceItemModel&) = delete;
	SequenceItemModel& operator=(const SequenceItemModel&) = delete;

	const QMap<QString, WidgetTypeInfo>& getWidgetTypes() const;
	void setComboOptions(const QString& type, int paramId, const QStringList& options);
	QStringList getComboOptions(const QString& type, int paramId) const;
	void setNumericRange(const QString& type, int paramId, double min, double max);
	QPair<double, double> getNumericRange(const QString& type, int paramId) const;
	void setNumericIncrement(const QString& type, int paramId, double increment);
	double getNumericIncrement(const QString& type, int paramId) const;

signals:
	void numericRangeChanged(const QString& type, int paramId, double min, double max);
	void numericIncrementChanged(const QString& type, int paramId, double increment);
	void comboOptionsChanged(const QString& type, int paramId, const QStringList& options);

private:
	SequenceItemModel();
	void initializeModel();
	QMap<QString, WidgetTypeInfo> widgetTypeMap;
};

#endif // __SEQUENCEITEMMODEL_H