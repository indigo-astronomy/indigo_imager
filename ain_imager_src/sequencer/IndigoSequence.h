#ifndef __INDIGOSEQUENCE_H
#define __INDIGOSEQUENCE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QMenu>
#include <QLineEdit>
#include "IndigoSequenceItem.h"
#include "IndigoSequenceParser.h"

class IndigoSequence : public QWidget {
	Q_OBJECT

public:
	explicit IndigoSequence(QWidget *parent = nullptr);

	void addItem(IndigoSequenceItem *item);
	void removeItem(IndigoSequenceItem* item);

	QList<IndigoSequenceItem *> getItems() const;
	IndigoSequenceItem* getItemAt(int index) const;

	void scrollToItem(int index);

	QString getSequenceName() const { return sequenceNameEdit->text(); }
	void setSequenceName(const QString& name) { sequenceNameEdit->setText(name); }

	void viewFromFunctionCalls(const QVector<FunctionCall>& calls);
	const QVector<FunctionCall> functionCallsFromView(const QString& objectName = "sequence") const;

	void saveSequence();
	void loadSequence();

	void setDragSourceWidget(IndigoSequenceItem* widget) { dragSourceWidget = widget; }
	IndigoSequenceItem* getDragSourceWidget() const { return dragSourceWidget; }

signals:
	void requestSequence();

protected:
	void contextMenuEvent(QContextMenuEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void showDropIndicator(int insertAt);
	int determineInsertPosition(const QPoint &pos);
	int getIndexOfItem(IndigoSequenceItem* item) const;

private slots:
	void addItemFromMenu();
	void onNumericRangeChanged(const QString& type, int paramId, double min, double max);
	void onNumericIncrementChanged(const QString& type, int paramId, double increment);
	void onComboOptionsChanged(const QString& type, int paramId, const QStringList& options);

private:
	QVBoxLayout *layout;
	QScrollArea *scrollArea;
	QWidget *container;
	QVBoxLayout *containerLayout;
	QFrame *dropIndicator;
	QPoint contextMenuPos;
	IndigoSequenceItem* dragSourceWidget;
	QLineEdit* sequenceNameEdit;

	QToolButton *m_download_sequence_button;
	QToolButton *m_load_sequence_button;
	QToolButton *m_save_sequence_button;
};

#endif // __INDIGOSEQUENCE_H