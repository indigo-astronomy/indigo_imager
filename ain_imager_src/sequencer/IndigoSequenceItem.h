// Copyright (c) 2025 Rumen G.Bogdanovski
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

	bool isOmitted() const { return m_omitted; }
	void setOmitted(bool omitted, bool setChildren = true);

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

signals:
	void enable(bool enabled);

public slots:
	void setEnabledState(bool enabled);
	void toggleOmitted();

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
	void addItemFromMenu();
	void removeWidget();

private:
	QString type;
	QToolButton *statusButton; // Changed from QLabel to QToolButton
	QLabel *typeLabel;
	QToolButton *deleteButton;
	QHBoxLayout *mainLayout;
	QVBoxLayout *outerLayout;
	QFrame *frame;
	QFrame *dropIndicator;
	QPoint contextMenuPos;
	QWidget *overlay;
	QMap<int, QWidget *> parameterWidgets;
	QVBoxLayout *repeatLayout;
	QLabel *iterationLabel;

	bool isEnabledState;
	bool m_omitted = false;

	void setupUI();
	void addInputWidget(const QString &paramName, ParamWidget paramWidget, int key);
	void showDropIndicator(int yPos);
	int determineInsertPosition(const QPoint &pos);
	bool isAncestorOf(QWidget* possibleChild) const;
	int getNestingLevel() const;
	int getIndexOfItem(IndigoSequenceItem* item) const;
};

#endif // __INDIGOSEQUENCEITEM_H