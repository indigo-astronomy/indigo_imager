#ifndef __INDIGOSEQUENCEITEM_H
#define __INDIGOSEQUENCEITEM_H

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMap>
#include <QVariant>
#include <QFrame>
#include <QMouseEvent>
#include <SequenceItemModel.h>

class IndigoSequenceItem : public QWidget {
	Q_OBJECT

public:
	explicit IndigoSequenceItem(const QString &type, QWidget *parent = nullptr);
	QString getType() const;
	void setParameter(int paramName, const QVariant &value);
	QVariant getParameter(int paramName) const;
	void setComboBoxItems(const QStringList &items);
	QString getComboBoxSelectedItem() const;
	void addItem(IndigoSequenceItem *item);
	QVBoxLayout* getRepeatLayout() { return repeatLayout; }
	void clearItems();

	void showDragOverlay();
	void hideDragOverlay();

	void updateNumericRange(int paramId, double min, double max);
	void updateNestedNumericRanges(const QString& type, int paramId, double min, double max);
	void updateNumericIncrement(int paramId, double increment);
	void updateNestedNumericIncrements(const QString& type, int paramId, double increment);
	void updateComboOptions(int paramId, const QStringList& options);
	void updateNestedComboOptions(const QString& type, int paramId, const QStringList& options);

	void setIdle();
	void setBusy();
	void setAlert();
	void setOk();

	void setIteration(int count);

	struct ParameterInfo {
		QString label;
		QString widgetType;
	};

	struct WidgetTypeInfo {
		QString label;
		QMap<int, ParameterInfo> parameters;
	};

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
	void removeWidget();

private:
	QString type;
	QLabel *statusLabel;
	QLabel *typeLabel;
	QToolButton *deleteButton;
	QHBoxLayout *mainLayout;
	QVBoxLayout *outerLayout;
	QFrame *frame;
	QFrame *dropIndicator;
	QWidget *overlay;
	QMap<int, QWidget *> parameterWidgets;
	QVBoxLayout *repeatLayout; // Layout for nested IndigoSequenceItems
	QLabel *iterationLabel;

	void setupUI();
	void addInputWidget(const QString &paramName, ParamWidget paramWidget, int key);
	void showDropIndicator(int yPos);
	int determineInsertPosition(const QPoint &pos);
	bool isAncestorOf(QWidget* possibleChild) const;
	int getNestingLevel() const;
	int getIndexOfItem(IndigoSequenceItem* item) const;
};

#endif // __INDIGOSEQUENCEITEM_H