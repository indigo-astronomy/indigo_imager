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

#include "IndigoSequence.h"
#include "SequenceItemModel.h"
#include <QContextMenuEvent>
#include <QAction>
#include <QMenu>
#include <QLabel>
#include <QWidgetAction>
#include <QDrag>
#include <QMimeData>
#include <QWidget>
#include <QScrollBar>
#include <QFrame>
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QLayoutItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>

IndigoSequence::IndigoSequence(QWidget *parent) : QWidget(parent) {
	layout = new QVBoxLayout(this);
	scrollArea = new QScrollArea(this);
	container = new QWidget(scrollArea);
	containerLayout = new QVBoxLayout(container);
	containerLayout->setAlignment(Qt::AlignTop);

	container->setLayout(containerLayout);
	scrollArea->setWidget(container);
	scrollArea->setWidgetResizable(true);

	layout->addWidget(scrollArea);

	// Create button toolbar
	QHBoxLayout *toolbox = new QHBoxLayout();

	// Add sequence name edit
	sequenceNameEdit = new QLineEdit(this);
	sequenceNameEdit->setPlaceholderText("Sequence Name");
	toolbox->addWidget(sequenceNameEdit);

	// Add buttons aligned right
	toolbox->addStretch();
	m_download_sequence_button = new QToolButton(this);
	m_download_sequence_button->setIcon(QIcon(":resource/download.png"));
	m_download_sequence_button->setToolTip("Download active sequence from server");
	toolbox->addWidget(m_download_sequence_button);
	connect(m_download_sequence_button, &QToolButton::clicked, this, [this]() {
		emit requestSequence();
	});

	m_load_sequence_button = new QToolButton(this);
	m_load_sequence_button->setIcon(QIcon(":resource/folder.png"));
	m_load_sequence_button->setToolTip("Load sequence from file");
	toolbox->addWidget(m_load_sequence_button);
	connect(m_load_sequence_button, &QToolButton::clicked, this, &IndigoSequence::loadSequence);

	m_save_sequence_button = new QToolButton(this);
	m_save_sequence_button->setIcon(QIcon(":resource/save.png"));
	m_save_sequence_button->setToolTip("Save sequence to file");
	toolbox->addWidget(m_save_sequence_button);
	connect(m_save_sequence_button, &QToolButton::clicked, this, &IndigoSequence::saveSequence);

	// Add toolbox to layout
	layout->addLayout(toolbox);

	setLayout(layout);

	// Enable drag and drop
	setAcceptDrops(true);

	dropIndicator = new QFrame(this);
	dropIndicator->setFrameShape(QFrame::HLine);
	dropIndicator->setFrameShadow(QFrame::Sunken);
	dropIndicator->setVisible(false);

	dragSourceWidget = nullptr;

	// Create overlay label
	overlayPrompt = new QLabel("Right-click to add items<br><br>Drag & drop to rearrange", this);
	overlayPrompt->setAlignment(Qt::AlignCenter);
	overlayPrompt->setStyleSheet("QLabel { background-color: rgba(255, 255, 255, 10); color: gray; font-size: 20px; }");
	overlayPrompt->raise();

	// Connect to model signals
	connect(&SequenceItemModel::instance(), &SequenceItemModel::numericRangeChanged,
			this, &IndigoSequence::onNumericRangeChanged);
	connect(&SequenceItemModel::instance(), &SequenceItemModel::numericIncrementChanged,
			this, &IndigoSequence::onNumericIncrementChanged);
	connect(&SequenceItemModel::instance(), &SequenceItemModel::comboOptionsChanged,
			this, &IndigoSequence::onComboOptionsChanged);
}


int IndigoSequence::itemCount() const {
	int totalCount = 0;

	for (int i = 0; i < containerLayout->count(); ++i) {
		IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(containerLayout->itemAt(i)->widget());
		if (!item) continue;

		totalCount++;

		if (item->getType() == "repeat") {
			QVBoxLayout* repeatLayout = item->getRepeatLayout();
			for (int j = 0; j < repeatLayout->count(); ++j) {
				IndigoSequenceItem* nestedItem = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(j)->widget());
				if (nestedItem) {
					totalCount++;
				}
			}
		}
	}

	return totalCount;
}

void IndigoSequence::addItem(IndigoSequenceItem *item) {
	containerLayout->addWidget(item);
}

QList<IndigoSequenceItem *> IndigoSequence::getItems() const {
	QList<IndigoSequenceItem *> items;
	for (int i = 0; i < containerLayout->count(); ++i) {
		IndigoSequenceItem *item = qobject_cast<IndigoSequenceItem *>(containerLayout->itemAt(i)->widget());
		if (item) {
			items.append(item);
		}
	}
	return items;
}

IndigoSequenceItem* IndigoSequence::getItemAt(int index) const {
	if (index < 0) {
		return nullptr;
	}

	int currentIndex = 0;
	for (int i = 0; i < containerLayout->count(); ++i) {
		IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(containerLayout->itemAt(i)->widget());
		if (!item) {
			continue;
		}

		if (currentIndex == index) {
			return item;
		}
		currentIndex++;

		if (item->getType() == "repeat") {
			QVBoxLayout* repeatLayout = item->getRepeatLayout();
			for (int j = 0; j < repeatLayout->count(); ++j) {
				IndigoSequenceItem* nestedItem = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(j)->widget());
				if (!nestedItem) {
					continue;
				}

				if (currentIndex == index) {
					return nestedItem;
				}
				currentIndex++;
			}
		}
	}

	return nullptr;
}

void IndigoSequence::scrollToItem(int index) {
	IndigoSequenceItem* widget = getItemAt(index);
	if (!widget) {
		return;
	}

	if(widget->getType() == "repeat") {
		widget = getItemAt(++index);
	}

	if (!widget) {
		return;
	}

	scrollArea->ensureWidgetVisible(widget, 50, 2 * widget->height());
}

void IndigoSequence::contextMenuEvent(QContextMenuEvent *event) {
	QMenu contextMenu(this);
	contextMenuPos = event->pos();

	QLabel *captionLabel = new QLabel("Select Action :", this);
	QFont boldFont = captionLabel->font();
	boldFont.setBold(true);
	captionLabel->setFont(boldFont);
	captionLabel->setAlignment(Qt::AlignCenter);
	captionLabel->setContentsMargins(5, 5, 5, 5);
	QWidgetAction *captionAction = new QWidgetAction(&contextMenu);
	captionAction->setDefaultWidget(captionLabel);
	contextMenu.addAction(captionAction);
	contextMenu.addSeparator();

	int insertAt = determineInsertPosition(contextMenuPos);
	showDropIndicator(insertAt);

	// Add categories
	QMap<QString, QMenu*> submenus;
	const auto& submenuCategories = SequenceItemModel::instance().getCategories();
	const auto& categoryIcons = SequenceItemModel::instance().getCategoryIcons();
	for (const auto& category : submenuCategories) {
		if (category.first == __SEPARATOR__) {
			contextMenu.addSeparator();
		} else {
			submenus[category.first] = contextMenu.addMenu(categoryIcons[category.first], category.first);
			//submenus[category.first] = contextMenu.addMenu(category.first);
		}
	}

	// Add actions
	const auto& widgetTypes = SequenceItemModel::instance().getWidgetTypes();
	for (const auto& category : submenuCategories) {
		for (const auto& type : category.second) {
			if (type == __SEPARATOR__) {
				submenus[category.first]->addSeparator();
			} else if (widgetTypes.contains(type)) {
				QString actionText = widgetTypes[type].label;
				QAction *action = new QAction(actionText, &contextMenu);
				action->setData(type);

				connect(action, &QAction::triggered, this, &IndigoSequence::addItemFromMenu);

				submenus[category.first]->addAction(action);
			}
		}
	}

	contextMenu.exec(event->globalPos());
	dropIndicator->setVisible(false);
}

void IndigoSequence::addItemFromMenu() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action) return;

	QString type = action->data().toString(); // Retrieve the type from the action's data
	IndigoSequenceItem *item = new IndigoSequenceItem(type, this);

	int insertAt = determineInsertPosition(contextMenuPos);
	containerLayout->insertWidget(insertAt, item);
}

void IndigoSequence::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		event->acceptProposedAction();
	}
}

void IndigoSequence::dragMoveEvent(QDragMoveEvent *event) {
	if (event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		event->acceptProposedAction();
		int insertAt = determineInsertPosition(event->pos());
		showDropIndicator(insertAt);
	}
}

void IndigoSequence::dropEvent(QDropEvent *event) {
	dropIndicator->setVisible(false);
	if (dragSourceWidget) {
		dragSourceWidget->hideDragOverlay();
	}

	if (event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		QByteArray itemData = event->mimeData()->data("application/x-indigosequenceitem");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);

		QString type;
		QVariantMap parameters;
		dataStream >> type >> parameters;

		if (dragSourceWidget) {
			// Clear existing nested items if it's a repeat widget
			if (type == "repeat") {
				dragSourceWidget->clearItems();
			}

			// Move the widget to new position
			containerLayout->removeWidget(dragSourceWidget);
			int insertAt = determineInsertPosition(event->pos());
			containerLayout->insertWidget(insertAt, dragSourceWidget);

			// Restore parameters
			for (auto it = parameters.begin(); it != parameters.end(); ++it) {
				dragSourceWidget->setParameter(it.key().toInt(), it.value());
			}

			// Restore nested items for repeat widget
			if (type == "repeat") {
				int nestedItemCount;
				dataStream >> nestedItemCount;

				for (int i = 0; i < nestedItemCount; ++i) {
					QString nestedType;
					QVariantMap nestedParameters;
					dataStream >> nestedType >> nestedParameters;

					IndigoSequenceItem *nestedItem = new IndigoSequenceItem(nestedType, dragSourceWidget);
					for (auto it = nestedParameters.begin(); it != nestedParameters.end(); ++it) {
						nestedItem->setParameter(it.key().toInt(), it.value());
					}
					dragSourceWidget->addItem(nestedItem);
				}
			}
			dragSourceWidget = nullptr;
		}
		event->acceptProposedAction();
	}
}

void IndigoSequence::dragLeaveEvent(QDragLeaveEvent *event) {
	Q_UNUSED(event);
	dropIndicator->setVisible(false);
}

void IndigoSequence::showDropIndicator(int insertAt) {
	const int thickness = 2;
	const int magicOffset = 2; // Offset to make the drop indicator look better - no idea why it's needed
	QMargins margins = layout->contentsMargins() + containerLayout->contentsMargins();
	int spacing = containerLayout->spacing() + layout->spacing();
	int scrollOffset = scrollArea->verticalScrollBar()->value();

	if (containerLayout->count() > 0) {
		if (insertAt < containerLayout->count()) {
			QWidget* targetWidget = containerLayout->itemAt(insertAt)->widget();
			QWidget* prevWidget = (insertAt > 0) ? containerLayout->itemAt(insertAt-1)->widget() : nullptr;

			int yPos;
			if (prevWidget) {
				// Position between two widgets
				yPos = prevWidget->geometry().bottom() + magicOffset + ((spacing + margins.bottom()) / 2);
			} else {
				// Position before first widget
				yPos = targetWidget->geometry().top() - 2 * magicOffset + ((spacing + margins.top()) / 2);
			}

			dropIndicator->setGeometry(
				margins.left() + spacing,
				yPos - scrollOffset,
				container->width() - (margins.left() + margins.right()),
				thickness
			);
		} else {
			// Position after last widget
			QWidget* lastWidget = containerLayout->itemAt(containerLayout->count()-1)->widget();
			int yPos = lastWidget->geometry().bottom() + magicOffset + ((spacing + margins.bottom()) / 2);

			dropIndicator->setGeometry(
				margins.left() + spacing,
				yPos - scrollOffset,
				container->width() - (margins.left() + margins.right()),
				thickness
			);
		}
	} else {
		// Empty container
		dropIndicator->setGeometry(
			margins.left() + spacing,
			margins.top(),
			container->width() - (margins.left() + margins.right()),
			thickness
		);
	}
	dropIndicator->setVisible(true);
}

int IndigoSequence::determineInsertPosition(const QPoint &pos) {
	QPoint dropPos = pos + QPoint(0, scrollArea->verticalScrollBar()->value());
	int insertAt = containerLayout->count();
	for (int i = 0; i < containerLayout->count(); ++i) {
		QRect itemRect = containerLayout->itemAt(i)->widget()->geometry();
		int itemMiddle = itemRect.top() + itemRect.height() / 2;
		if (dropPos.y() < itemMiddle) {
			insertAt = i;
			break;
		} else if (dropPos.y() < itemRect.bottom()) {
			insertAt = i + 1;
			break;
		}
	}
	return insertAt;
}

int IndigoSequence::getIndexOfItem(IndigoSequenceItem* item) const {
	for (int i = 0; i < containerLayout->count(); ++i) {
		QWidget* widget = containerLayout->itemAt(i)->widget();
		if (widget == item) {
			return i;
		}
	}
	return -1;
}

void IndigoSequence::removeItem(IndigoSequenceItem* item) {
	for (int i = 0; i < containerLayout->count(); ++i) {
		QWidget* widget = containerLayout->itemAt(i)->widget();
		if (widget == item) {
			QLayoutItem* layoutItem = containerLayout->takeAt(i);
			delete layoutItem->widget();
			delete layoutItem;
			break;
		}
	}
}

void IndigoSequence::onNumericRangeChanged(const QString& type, int paramId, double min, double max) {
	for (IndigoSequenceItem* item : getItems()) {
		if (item->getType() == type) {
			item->updateNumericRange(paramId, min, max);
		}
		if (item->getType() == "repeat") {
			item->updateNestedNumericRanges(type, paramId, min, max);
		}
	}
}

void IndigoSequence::onNumericIncrementChanged(const QString& type, int paramId, double increment) {
	for (IndigoSequenceItem* item : getItems()) {
		if (item->getType() == type) {
			item->updateNumericIncrement(paramId, increment);
		}
		if (item->getType() == "repeat") {
			item->updateNestedNumericIncrements(type, paramId, increment);
		}
	}
}

void IndigoSequence::onComboOptionsChanged(const QString& type, int paramId, const QStringList& options) {
	for (IndigoSequenceItem* item : getItems()) {
		if (item->getType() == type) {
			item->updateComboOptions(paramId, options);
		}
		if (item->getType() == "repeat") {
			item->updateNestedComboOptions(type, paramId, options);
		}
	}
}

void IndigoSequence::viewFromFunctionCalls(const QVector<FunctionCall>& calls) {
	// Clear existing items
	while (containerLayout->count() > 0) {
		QLayoutItem* item = containerLayout->takeAt(0);
		if (item->widget()) {
			delete item->widget();
		}
		delete item;
	}
	sequenceNameEdit->setText("");

	if (calls.isEmpty()) return;

	auto cleanStringParameter = [](const QString& param) -> QString {
		// Skip cleaning if it's a number
		bool isNumber;
		param.toDouble(&isNumber);
		if (isNumber) return param;

		QString result = param;

		result.replace("\\'", "'");
		result.replace("\\\"", "\"");

		if (result.startsWith('"') && result.endsWith('"')) {
			result = result.mid(1, result.length() - 2);
		}
		return result;
	};

	for (const FunctionCall& call : calls) {
		if (call.functionName == "Sequence") {
			sequenceNameEdit->setText(call.parameters.isEmpty() ? "" : cleanStringParameter(call.parameters[0]));
			continue;
		}
		if (call.functionName == "start") {
			continue;
		}

		IndigoSequenceItem* item = new IndigoSequenceItem(call.functionName, this);

		const auto& widgetTypes = SequenceItemModel::instance().getWidgetTypes();
		if (widgetTypes.contains(call.functionName)) {
			const auto& widgetInfo = widgetTypes[call.functionName];
			for (int i = 0; i < call.parameters.size() && i < widgetInfo.parameters.size(); ++i) {
				const auto& paramInfo = widgetInfo.parameters[i];
				if (paramInfo.paramWidget == CheckBox) {
					bool checked = call.parameters[i].toLower() == "true";
					item->setParameter(i, checked);
				} else {
					QString cleanedParam = cleanStringParameter(call.parameters[i]);
					item->setParameter(i, cleanedParam);
				}
			}
		}

		if (call.functionName == "repeat" && !call.nestedCalls.isEmpty()) {
			for (const FunctionCall& nestedCall : call.nestedCalls) {
				if (nestedCall.functionName != "Sequence" && nestedCall.functionName != "start") {
					IndigoSequenceItem* nestedItem = new IndigoSequenceItem(nestedCall.functionName, item);
					if (widgetTypes.contains(nestedCall.functionName)) {
						const auto& nestedWidgetInfo = widgetTypes[nestedCall.functionName];
						for (int i = 0; i < nestedCall.parameters.size() && i < nestedWidgetInfo.parameters.size(); ++i) {
							const auto& paramInfo = nestedWidgetInfo.parameters[i];
							if (paramInfo.paramWidget == CheckBox) {
								bool checked = nestedCall.parameters[i].toLower() == "true";
								nestedItem->setParameter(i, checked);
							} else {
								QString cleanedParam = cleanStringParameter(nestedCall.parameters[i]);
								nestedItem->setParameter(i, cleanedParam);
							}
						}
					}
					item->addItem(nestedItem);
				}
			}
		}

		addItem(item);
	}
}

const QVector<FunctionCall> IndigoSequence::functionCallsFromView(const QString& objectName) const {
	QVector<FunctionCall> calls;
	FunctionCall constructorCall;
	constructorCall.objectName = objectName;
	constructorCall.functionName = "Sequence";

	QString seqName = sequenceNameEdit->text().trimmed();
	if (!seqName.isEmpty()) {
		QString escapedSeqName = seqName.replace("\"", "\\\"");
		escapedSeqName.replace("'", "\\'");
		constructorCall.parameters.append("\"" + escapedSeqName + "\"");
	}
	calls.append(constructorCall);

	for (int i = 0; i < containerLayout->count(); ++i) {
		QWidget* widget = containerLayout->itemAt(i)->widget();
		IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(widget);
		if (!item) continue;

		FunctionCall call;
		call.objectName = objectName;
		call.functionName = item->getType();

		const auto& widgetTypes = SequenceItemModel::instance().getWidgetTypes();
		if (widgetTypes.contains(item->getType())) {
			const auto& widgetInfo = widgetTypes[item->getType()];
			for (auto paramIt = widgetInfo.parameters.constBegin(); paramIt != widgetInfo.parameters.constEnd(); ++paramIt) {
				QVariant value = item->getParameter(paramIt.key());
				if (value.type() == QVariant::Bool) {
					call.parameters.append(value.toBool() ? "true" : "false");
				} else if (
					paramIt.value().paramWidget == LineEdit ||
					paramIt.value().paramWidget == ComboBox
				)
				{
					QString escapedValue = value.toString().replace("\"", "\\\"");
					escapedValue.replace("'", "\\'");
					call.parameters.append(QString("\"%1\"").arg(escapedValue));
				} else {
					call.parameters.append(value.toString());
				}
			}
		}

		if (item->getType() == "repeat") {
			QVBoxLayout* repeatLayout = item->getRepeatLayout();
			for (int j = 0; j < repeatLayout->count(); ++j) {
				QWidget* nestedWidget = repeatLayout->itemAt(j)->widget();
				IndigoSequenceItem* nestedItem = qobject_cast<IndigoSequenceItem*>(nestedWidget);
				if (!nestedItem) continue;

				FunctionCall nestedCall;
				nestedCall.objectName = objectName;
				nestedCall.functionName = nestedItem->getType();

				if (widgetTypes.contains(nestedItem->getType())) {
					const auto& nestedWidgetInfo = widgetTypes[nestedItem->getType()];
					for (auto paramIt = nestedWidgetInfo.parameters.constBegin(); paramIt != nestedWidgetInfo.parameters.constEnd(); ++paramIt) {
						QVariant value = nestedItem->getParameter(paramIt.key());
						if (value.type() == QVariant::Bool) {
							nestedCall.parameters.append(value.toBool() ? "true" : "false");
						} else if (
							paramIt.value().paramWidget == LineEdit ||
							paramIt.value().paramWidget == ComboBox
						)
						{
							QString escapedValue = value.toString().replace("\"", "\\\"");
							escapedValue.replace("'", "\\'");
							nestedCall.parameters.append(QString("\"%1\"").arg(escapedValue));
						} else {
							nestedCall.parameters.append(value.toString());
						}
					}
				}
				call.nestedCalls.append(nestedCall);
			}
		}
		calls.append(call);
	}

	FunctionCall startCall;
	startCall.objectName = objectName;
	startCall.functionName = "start";
	calls.append(startCall);

	return calls;
}

void IndigoSequence::loadScriptToView(const QString& script) {
	IndigoSequenceParser parser;
	QStringList validationErrors;

	connect(&parser, &IndigoSequenceParser::validationError,
		[&validationErrors](const QString& error) {
			validationErrors.append(error);
		});

	QVector<FunctionCall> calls = parser.parse(script);

	if (parser.validateCalls(calls)) {
		viewFromFunctionCalls(calls);
	} else {
		QString errorMessage = tr("The sequence file contains the following errors:\n\n");
		errorMessage += validationErrors.join("\n");
		QMessageBox::critical(this, tr("Validation Error"), errorMessage);
	}
}

QString IndigoSequence::makeScriptFromView() const {
	IndigoSequenceParser parser;
	QVector<FunctionCall> calls = functionCallsFromView();
	return parser.generate(calls);
}

void IndigoSequence::loadSequence() {
	QString homeDir = QDir::homePath();
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Load Sequence"),
		homeDir,
		tr("JavaScript Files (*.js);;All Files (*)"));

	if (fileName.isEmpty()) {
		return;
	}

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QMessageBox::critical(this, tr("Error"), tr("Could not open file: %1").arg(file.errorString()));
		return;
	}

	QTextStream in(&file);
	QString script = in.readAll();
	file.close();

	loadScriptToView(script);
}

void IndigoSequence::saveSequence() {
	QString defaultFileName = sequenceNameEdit->text().trimmed();
	if (defaultFileName.isEmpty()) {
		defaultFileName = "untitled_sequence";
	}
	defaultFileName += ".js";

	QString homeDir = QDir::homePath();

	QString fileName = QFileDialog::getSaveFileName(this,
		tr("Save Sequence"),
		QDir(homeDir).filePath(defaultFileName),
		tr("JavaScript Files (*.js);;All Files (*)"));

	if (fileName.isEmpty()) {
		return;
	}

	QString script = makeScriptFromView();

	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::critical(this, tr("Error"), tr("Could not save file: %1").arg(file.errorString()));
		return;
	}

	QTextStream out(&file);
	out << script;
	file.close();
}

void IndigoSequence::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	overlayPrompt->setGeometry(scrollArea->geometry());
}

void IndigoSequence::paintEvent(QPaintEvent *event) {
	QWidget::paintEvent(event);
	overlayPrompt->setVisible(containerLayout->count() == 0);
}

double IndigoSequence::totalExposure() const {
	double totalExposureTime = 0.0;

	for (int i = 0; i < containerLayout->count(); ++i) {
		IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(containerLayout->itemAt(i)->widget());
		if (!item) continue;

		if (item->getType() == SC_CAPTURE_BATCH) {
			int count = item->getParameter(0).toInt();
			double exposure = item->getParameter(1).toDouble();
			totalExposureTime += count * exposure;
		} else if (item->getType() == SC_REPEAT) {
			int repeatCount = item->getParameter(0).toInt();
			QVBoxLayout* repeatLayout = item->getRepeatLayout();
			for (int j = 0; j < repeatLayout->count(); ++j) {
				IndigoSequenceItem* nestedItem = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(j)->widget());
				if (!nestedItem) continue;

				if (nestedItem->getType() == SC_CAPTURE_BATCH) {
					int count = nestedItem->getParameter(0).toInt();
					double exposure = nestedItem->getParameter(1).toDouble();
					totalExposureTime += repeatCount * count * exposure;
				}
			}
		}
	}

	return totalExposureTime;
}