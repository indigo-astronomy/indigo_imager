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

	int itemCount() const;
	QList<IndigoSequenceItem *> getItems() const;
	IndigoSequenceItem* getItemAt(int index) const;
	int getItemIndexByExecutedStep(int executedIndex) const;

	void scrollToItem(int index);

	QString getSequenceName() const { return sequenceNameEdit->text(); }
	void setSequenceName(const QString& name) { sequenceNameEdit->setText(name); }

	void viewFromFunctionCalls(const QVector<FunctionCall>& calls);
	const QVector<FunctionCall> functionCallsFromView(const QString& objectName = "sequence") const;

	void saveSequence();
	void loadSequence();

	double totalExposure() const;

	void loadScriptToView(const QString& script);
	QString makeScriptFromView() const;

	void setDragSourceWidget(IndigoSequenceItem* widget) { dragSourceWidget = widget; }
	IndigoSequenceItem* getDragSourceWidget() const { return dragSourceWidget; }

signals:
	void requestSequence();
	void enable(bool enabled);
	void setSequenceIdle();

public slots:
	void setEnabledState(bool enabled);
	void setIdle();

protected:
	void contextMenuEvent(QContextMenuEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dragMoveEvent(QDragMoveEvent *event) override;
	void dragLeaveEvent(QDragLeaveEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
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
	QLabel* overlayPrompt;

	bool isEnabledState;
};

#endif // __INDIGOSEQUENCE_H