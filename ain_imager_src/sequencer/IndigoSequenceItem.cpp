#include "SequenceItemModel.h"
#include "IndigoSequenceItem.h"
#include "IndigoSequence.h"
#include "QLineEditSG.h"
#include <QVariant>
#include <QMessageBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QDebug>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QWidgetAction>

IndigoSequenceItem::IndigoSequenceItem(const QString &type, QWidget *parent)
	: QWidget(parent), type(type), overlay(nullptr) {
	setObjectName(type); // Set the object name to the type
	setupUI();
}

void IndigoSequenceItem::setupUI() {
	frame = new QFrame(this);
	frame->setFrameStyle(QFrame::Box | QFrame::Plain);
	frame->setLineWidth(1);
	frame->setObjectName("IndigoSequenceFrame");
	if (type == "repeat") {
		//frame->setStyleSheet("QWidget#IndigoSequenceFrame { border: 1px solid #303030; }");
	} else {
		frame->setStyleSheet("QWidget#IndigoSequenceFrame { background-color: #252525; border: 1px solid #272727; }");
	}
	frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	const auto& model = SequenceItemModel::instance();
	auto typeInfo = model.getWidgetTypes()[type];

	if(!model.getWidgetTypes().contains(type)) {
		typeInfo.label = type + "(): unknown call";
	}

	statusLabel = new QLabel(this);
	statusLabel->setPixmap(QPixmap(":/resource/led-grey.png"));
	statusLabel->setContentsMargins(5, 0, 0, 0);
	statusLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	statusLabel->setStyleSheet("background: transparent;");

	typeLabel = new QLabel(typeInfo.label);
	QFont boldFont = typeLabel->font();
	boldFont.setBold(true);
	typeLabel->setFont(boldFont);
	typeLabel->setContentsMargins(0, 0, 10, 0);
	typeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	typeLabel->setStyleSheet("background: transparent;");

	deleteButton = new QToolButton(this);
	//deleteButton->setText(QString::fromUtf8("\u274C"));
	deleteButton->setText(QString::fromUtf8("\u2715"));
	//deleteButton->setText(QString::fromUtf8("\u2716"));
	//deleteButton->setText(QString::fromUtf8("\u00D7"));
	deleteButton->setIconSize(QSize(14, 14));
	deleteButton->setFixedSize(22, 22);
	deleteButton->setStyleSheet("QToolButton { \
		border: none; \
		background-color: #303030; \
		font-size: 12px; \
		color: rgb(150, 150, 150); \
		border-radius: 11px; \
	}");
	deleteButton->setContentsMargins(5, 0, 0, 0);
	deleteButton->setToolTip("Remove this item");
	connect(deleteButton, &QToolButton::clicked, this, &IndigoSequenceItem::removeWidget);

	mainLayout = new QHBoxLayout();
	mainLayout->addWidget(statusLabel);
	mainLayout->addWidget(typeLabel);
	mainLayout->addStretch();

	QList<int> keys = typeInfo.parameters.keys();
	for (int key : keys) {
		addInputWidget(typeInfo.parameters[key].label, typeInfo.parameters[key].widgetType, key);
	}

	mainLayout->addWidget(deleteButton);
	mainLayout->setSpacing(4);

	outerLayout = new QVBoxLayout(frame);
	outerLayout->addLayout(mainLayout);
	outerLayout->setContentsMargins(3, 3, 3, 3);
	outerLayout->setSpacing(2);
	outerLayout->setAlignment(Qt::AlignTop);

	if (type == "repeat") {
		repeatLayout = new QVBoxLayout();
		repeatLayout->setSpacing(5);
		repeatLayout->setContentsMargins(30, 5, 0, 5);
		outerLayout->addLayout(repeatLayout);

		// Add iteration label
		iterationLabel = new QLabel("Iteration: 0", this);
		iterationLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
		iterationLabel->setStyleSheet("background: transparent;");
		mainLayout->insertWidget(mainLayout->indexOf(deleteButton) - 2, iterationLabel);

		dropIndicator = new QFrame(this);
		dropIndicator->setFrameShape(QFrame::HLine);
		dropIndicator->setFrameShadow(QFrame::Sunken);
		dropIndicator->setVisible(false);

		setAcceptDrops(true);
	} else {
		iterationLabel = nullptr;
	}

	frame->setLayout(outerLayout);

	QVBoxLayout *mainOuterLayout = new QVBoxLayout(this);
	mainOuterLayout->addWidget(frame);
	mainOuterLayout->setContentsMargins(1, 1, 1, 1);
	mainOuterLayout->setSpacing(0);
	mainOuterLayout->setAlignment(Qt::AlignTop);

	overlay = new QWidget(this);
	overlay->setStyleSheet("background-color: rgba(255, 255, 255, 15);");
	hideDragOverlay();

	setLayout(mainOuterLayout);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
	setAcceptDrops(true);
}

void IndigoSequenceItem::addInputWidget(const QString &paramName, const QString &paramType, int key) {
	QLabel *label = new QLabel(paramName+":", this);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	label->setStyleSheet("background: transparent;");
	label->setContentsMargins(3, 0, 0, 0);
	label->setToolTip(paramType);
	QWidget *input = nullptr;

	if (paramType == "QLineEditSG-RA") {
		input = new QLineEditSG(QLineEditSG::RA, this);
	} else if (paramType == "QLineEditSG-DEC") {
		input = new QLineEditSG(QLineEditSG::DEC, this);
	} else if (paramType == "QLineEdit") {
		input = new QLineEdit(this);
	} else if (paramType == "QSpinBox") {
		input = new QSpinBox(this);
	} else if (paramType == "QDoubleSpinBox") {
		input = new QDoubleSpinBox(this);
	} else if (paramType == "QComboBox") {
		input = new QComboBox(this);
	} else if (paramType == "QCheckBox") {
		input = new QCheckBox(this);
		input->setStyleSheet("QCheckBox { background: transparent; }");
	}

	const auto& model = SequenceItemModel::instance();

	if (paramType == "QComboBox") {
		QComboBox* combo = qobject_cast<QComboBox*>(input);
		combo->setEditable(true);
		combo->setInsertPolicy(QComboBox::NoInsert);
		combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		//combo->setMinimumContentsLength(15); // Set minimum number of characters
		combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		if (combo) {
			combo->addItems(model.getComboOptions(type, key));
		}
	} else if (paramType == "QSpinBox") {
		QSpinBox* spin = qobject_cast<QSpinBox*>(input);
		if (spin) {
			auto range = model.getNumericRange(type, key);
			spin->setRange(range.first, range.second);
			spin->setSingleStep(model.getNumericIncrement(type, key));
		}
	} else if (paramType == "QDoubleSpinBox") {
		QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(input);
		if (spin) {
			auto range = model.getNumericRange(type, key);
			spin->setRange(range.first, range.second);
			spin->setSingleStep(model.getNumericIncrement(type, key));
		}
	} else if (paramType == "QLinedit") {
		QLineEdit* line = qobject_cast<QLineEdit*>(input);
		if (line) {
			line->setMinimumWidth(50);
		}
	}

	if (input) {
		if (!label->text().isEmpty()) {
			mainLayout->addWidget(label);
		}
		mainLayout->addWidget(input);
		parameterWidgets[key] = input;
	}
}

void IndigoSequenceItem::addItem(IndigoSequenceItem *item) {
	if (type == "repeat" && repeatLayout) {
		repeatLayout->addWidget(item);
	}
}

void IndigoSequenceItem::setComboBoxItems(const QStringList &items) {
	for (auto it = parameterWidgets.begin(); it != parameterWidgets.end(); ++it) {
		if (QComboBox *comboBox = qobject_cast<QComboBox *>(it.value())) {
			comboBox->addItems(items);
		}
	}
}

QString IndigoSequenceItem::getComboBoxSelectedItem() const {
	for (auto it = parameterWidgets.begin(); it != parameterWidgets.end(); ++it) {
		if (QComboBox *comboBox = qobject_cast<QComboBox *>(it.value())) {
			return comboBox->currentText();
		}
	}
	return QString();
}

QString IndigoSequenceItem::getType() const {
	return type;
}

void IndigoSequenceItem::setParameter(int paramName, const QVariant &value) {
	QWidget *widget = parameterWidgets.value(paramName, nullptr);
	if (!widget) return;

	if (QLineEditSG *sg = qobject_cast<QLineEditSG*>(widget)) {
		sg->setValue(value.toDouble());
	} else if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget)) {
		lineEdit->setText(value.toString());
	} else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget)) {
		spinBox->setValue(value.toInt());
	} else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
		doubleSpinBox->setValue(value.toDouble());
	} else if (QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
		comboBox->setCurrentText(value.toString());
	} else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(widget)) {
		checkBox->setChecked(value.toBool());
	}
}

QVariant IndigoSequenceItem::getParameter(const int paramID) const {
	if (!parameterWidgets.contains(paramID)) {
		return QVariant();
	}

	QWidget *widget = parameterWidgets[paramID];

	if (QLineEditSG *sg = qobject_cast<QLineEditSG *>(widget)) {
		return sg->value();
	} else if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(widget)) {
		return lineEdit->text();
	} else if (QSpinBox *spinBox = qobject_cast<QSpinBox *>(widget)) {
		return spinBox->value();
	} else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox *>(widget)) {
		return doubleSpinBox->value();
	} else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget)) {
		return comboBox->currentText();
	} else if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(widget)) {
		return checkBox->isChecked();
	} else {
		return QVariant();
	}
}

void IndigoSequenceItem::removeWidget() {
	delete this;
}

void IndigoSequenceItem::mousePressEvent(QMouseEvent *event) {
	if (event->button() == Qt::LeftButton) {
		// Find IndigoSequence parent
		IndigoSequence* parentSequence = nullptr;
		for (QWidget* parent = parentWidget(); parent; parent = parent->parentWidget()) {
			if (auto sequence = qobject_cast<IndigoSequence*>(parent)) {
				parentSequence = sequence;
				break;
			}
			if (auto scrollArea = qobject_cast<QScrollArea*>(parent)) {
				if (auto sequence = qobject_cast<IndigoSequence*>(scrollArea->parentWidget())) {
					parentSequence = sequence;
					break;
				}
			}
		}

		if (!parentSequence) {
			return;
		}

		parentSequence->setDragSourceWidget(this);

		QByteArray itemData;
		QDataStream dataStream(&itemData, QIODevice::WriteOnly);

		dataStream << type;

		QVariantMap parameters;
		for (auto it = parameterWidgets.begin(); it != parameterWidgets.end(); ++it) {
			parameters[QString::number(it.key())] = getParameter(it.key());
		}
		dataStream << parameters;

		if (type == "repeat") {
			int nestedItemCount = repeatLayout->count();
			dataStream << nestedItemCount;

			for (int i = 0; i < nestedItemCount; ++i) {
				IndigoSequenceItem *nestedItem = qobject_cast<IndigoSequenceItem *>(repeatLayout->itemAt(i)->widget());
				if (nestedItem) {
					QVariantMap nestedParameters;
					for (auto it = nestedItem->parameterWidgets.begin(); it != nestedItem->parameterWidgets.end(); ++it) {
						nestedParameters[QString::number(it.key())] = nestedItem->getParameter(it.key());
					}
					dataStream << nestedItem->getType() << nestedParameters;
				}
			}
		}

		QMimeData *mimeData = new QMimeData;
		mimeData->setData("application/x-indigosequenceitem", itemData);

		QDrag *drag = new QDrag(this);
		drag->setMimeData(mimeData);
		drag->setPixmap(this->grab());
		showDragOverlay();

		Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

		if (dropAction == Qt::IgnoreAction) {
			// Drag canceled
			hideDragOverlay();
		}

		event->accept();
		return;
	}
	QWidget::mousePressEvent(event);
}

void IndigoSequenceItem::clearItems() {
	if (type == "repeat" && repeatLayout) {
		while (repeatLayout->count() > 0) {
			QLayoutItem* item = repeatLayout->takeAt(0);
			if (item->widget()) {
				item->widget()->deleteLater();
			}
			delete item;
		}
	}
}

void IndigoSequenceItem::dragEnterEvent(QDragEnterEvent *event) {
	if (type == "repeat" && event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		QByteArray itemData = event->mimeData()->data("application/x-indigosequenceitem");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);
		QString draggedType;
		dataStream >> draggedType;

		// Check if this would exceed nesting limit
		if (draggedType == "repeat" && getNestingLevel() >= 0) {
			event->ignore();
			return;
		}

		event->acceptProposedAction();
	}
}

void IndigoSequenceItem::dragMoveEvent(QDragMoveEvent *event) {
	if (type == "repeat" && event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		event->acceptProposedAction();
		int insertAt = determineInsertPosition(event->pos());
		showDropIndicator(insertAt);
	}
}

void IndigoSequenceItem::dragLeaveEvent(QDragLeaveEvent *event) {
	if (type == "repeat") {
		dropIndicator->setVisible(false);
	}
	QWidget::dragLeaveEvent(event);
}

bool IndigoSequenceItem::isAncestorOf(QWidget* possibleChild) const {
	if (!possibleChild) return false;

	QWidget* parent = possibleChild->parentWidget();
	while (parent) {
		if (parent == this) return true;
		parent = parent->parentWidget();
	}
	return false;
}

void IndigoSequenceItem::dropEvent(QDropEvent *event) {
	if (type == "repeat" && event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		// Check nesting level
		QByteArray itemData = event->mimeData()->data("application/x-indigosequenceitem");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);
		QString draggedType;
		dataStream >> draggedType;

		if (draggedType == "repeat" && getNestingLevel() >= 0) {
			event->ignore();
			return;
		}

		dropIndicator->setVisible(false);

		// Find main sequence
		IndigoSequence* mainSequence = nullptr;
		for (QWidget* parent = parentWidget(); parent; parent = parent->parentWidget()) {
			if (auto sequence = qobject_cast<IndigoSequence*>(parent)) {
				mainSequence = sequence;
				break;
			}
			if (auto scrollArea = qobject_cast<QScrollArea*>(parent)) {
				if (auto sequence = qobject_cast<IndigoSequence*>(scrollArea->parentWidget())) {
					mainSequence = sequence;
					break;
				}
			}
		}

		if (!mainSequence) {
			event->ignore();
			return;
		}

		IndigoSequenceItem* draggedWidget = mainSequence->getDragSourceWidget();
		if (draggedWidget) {
			draggedWidget->hideDragOverlay();
		}

		if (!draggedWidget || draggedWidget == this) {
			event->ignore();
			return;
		}

		// Check if dragged widget is an ancestor of drop target
		if (draggedWidget->isAncestorOf(this)) {
			event->ignore();
			return;
		}

		// Determine position to insert
		int insertAt = determineInsertPosition(event->pos());
		int index = getIndexOfItem(draggedWidget);

		// Adjust insert position to account for the removal of the dragged widget
		if ((index >= 0) && (index < insertAt)) {
			insertAt--;
		}

		// Add to new parent
		draggedWidget->setParent(this);
		repeatLayout->insertWidget(insertAt, draggedWidget);
		draggedWidget->show();

		event->setDropAction(Qt::MoveAction);
		event->accept();
		return;
	}
	event->ignore();
}

void IndigoSequenceItem::showDropIndicator(int insertAt) {
	int leftMargin = repeatLayout->contentsMargins().left();
	int rightMargin = repeatLayout->contentsMargins().right();
	int spacing = repeatLayout->spacing();

	if (repeatLayout->count() > 0) {
		if (insertAt < repeatLayout->count()) {
			QRect itemRect = repeatLayout->itemAt(insertAt)->widget()->geometry();
			int yPos = itemRect.top() - (spacing / 2);
			dropIndicator->setGeometry(
				leftMargin + 10,
				yPos,
				frame->width() - (leftMargin + rightMargin + 20),
				2
			);
		} else {
			QRect itemRect = repeatLayout->itemAt(repeatLayout->count() - 1)->widget()->geometry();
			int yPos = itemRect.bottom() + (spacing / 2);
			dropIndicator->setGeometry(
				leftMargin + 10,
				yPos,
				frame->width() - (leftMargin + rightMargin + 20),
				2
			);
		}
	} else {
		int yPos = leftMargin + spacing;
		dropIndicator->setGeometry(
			leftMargin + 10,
			yPos,
			frame->width() - (leftMargin + rightMargin + 20),
			2
		);
	}
	dropIndicator->setVisible(true);
}

int IndigoSequenceItem::determineInsertPosition(const QPoint &pos) {
	int insertAt = repeatLayout->count();
	for (int i = 0; i < repeatLayout->count(); ++i) {
		QRect itemRect = repeatLayout->itemAt(i)->widget()->geometry();
		int itemMiddle = itemRect.top() + itemRect.height() / 2;
		if (pos.y() < itemMiddle) {
			insertAt = i;
			break;
		} else if (pos.y() < itemRect.bottom()) {
			insertAt = i + 1;
			break;
		}
	}
	return insertAt;
}

int IndigoSequenceItem::getNestingLevel() const {
	int level = 0;
	QWidget* parent = parentWidget();
	while (parent) {
		if (auto repeatItem = qobject_cast<IndigoSequenceItem*>(parent)) {
			if (repeatItem->getType() == "repeat") {
				level++;
			}
		}
		parent = parent->parentWidget();
	}
	return level;
}

int IndigoSequenceItem::getIndexOfItem(IndigoSequenceItem* item) const {
	for (int i = 0; i < repeatLayout->count(); ++i) {
		if (repeatLayout->itemAt(i)->widget() == item) {
			return i;
		}
	}
	return -1;
}

void IndigoSequenceItem::showDragOverlay() {
	overlay->resize(size());
	overlay->show();
	overlay->raise();
}

void IndigoSequenceItem::hideDragOverlay() {
	overlay->hide();
}

void IndigoSequenceItem::contextMenuEvent(QContextMenuEvent *event) {
	if (type != "repeat") {
		QWidget::contextMenuEvent(event);
		return;
	}

	QMenu contextMenu(this);
	QPoint menuPos = event->pos();

	// Add centered bold caption with padding
	QLabel *captionLabel = new QLabel("Select Action :", this);
	QFont boldFont = captionLabel->font();
	boldFont.setBold(true);
	captionLabel->setFont(boldFont);
	captionLabel->setAlignment(Qt::AlignCenter);
	captionLabel->setContentsMargins(5, 5, 5, 5);
	QWidgetAction *captionAction = new QWidgetAction(&contextMenu);
	captionAction->setDefaultWidget(captionLabel);
	contextMenu.addAction(captionAction);

	// Determine insert position and show indicator
	int insertAt = determineInsertPosition(menuPos);
	showDropIndicator(insertAt);

	// Add menu items from model
	const auto& widgetTypes = SequenceItemModel::instance().getWidgetTypes();
	for (auto it = widgetTypes.begin(); it != widgetTypes.end(); ++it) {
		QString actionText = it.value().label;
		QAction *action = new QAction(actionText, &contextMenu);
		action->setData(it.key());
	
		if (it.key() == "repeat" && getNestingLevel() >= 0) {
			action->setEnabled(false);
		}

		connect(action, &QAction::triggered, this, [this, event]() {
			QAction *action = qobject_cast<QAction *>(sender());
			if (!action) return;

			QString itemType = action->data().toString();
			IndigoSequenceItem *item = new IndigoSequenceItem(itemType, this);

			// Determine position to insert using the stored position
			int insertAt = determineInsertPosition(event->pos());
			repeatLayout->insertWidget(insertAt, item);
		});
		contextMenu.addAction(action);
	}

	contextMenu.exec(event->globalPos());
	dropIndicator->setVisible(false);
}

void IndigoSequenceItem::updateNumericRange(int paramId, double min, double max) {
	auto it = parameterWidgets.find(paramId);
	if (it != parameterWidgets.end()) {
		if (QSpinBox* spin = qobject_cast<QSpinBox*>(it.value())) {
			spin->setRange(min, max);
		} else if (QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(it.value())) {
			dspin->setRange(min, max);
		}
	}
}

void IndigoSequenceItem::updateNumericIncrement(int paramId, double increment) {
	auto it = parameterWidgets.find(paramId);
	if (it != parameterWidgets.end()) {
		if (QSpinBox* spin = qobject_cast<QSpinBox*>(it.value())) {
			spin->setSingleStep(increment);
		} else if (QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(it.value())) {
			dspin->setSingleStep(increment);
		}
	}
}

void IndigoSequenceItem::updateComboOptions(int paramId, const QStringList& options) {
	auto it = parameterWidgets.find(paramId);
	if (it != parameterWidgets.end()) {
		if (QComboBox* combo = qobject_cast<QComboBox*>(it.value())) {
			QString currentText = combo->currentText();
			combo->clear();
			combo->addItems(options);
			if(!currentText.isEmpty()) {
				combo->setCurrentText(currentText);
			} else {
				combo->setCurrentIndex(0);
			}
		}
	}
}

void IndigoSequenceItem::updateNestedNumericRanges(const QString& type, int paramId, double min, double max) {
	if (this->type == "repeat") {
		for (int i = 0; i < repeatLayout->count(); ++i) {
			if (IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(i)->widget())) {
				if (item->getType() == type) {
					item->updateNumericRange(paramId, min, max);
				}
				item->updateNestedNumericRanges(type, paramId, min, max);
			}
		}
	}
}

void IndigoSequenceItem::updateNestedNumericIncrements(const QString& type, int paramId, double increment) {
	if (this->type == "repeat") {
		for (int i = 0; i < repeatLayout->count(); ++i) {
			if (IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(i)->widget())) {
				if (item->getType() == type) {
					item->updateNumericIncrement(paramId, increment);
				}
				item->updateNestedNumericIncrements(type, paramId, increment);
			}
		}
	}
}

void IndigoSequenceItem::updateNestedComboOptions(const QString& type, int paramId, const QStringList& options) {
	if (this->type == "repeat") {
		for (int i = 0; i < repeatLayout->count(); ++i) {
			if (IndigoSequenceItem* item = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(i)->widget())) {
				if (item->getType() == type) {
					item->updateComboOptions(paramId, options);
				}
				item->updateNestedComboOptions(type, paramId, options);
			}
		}
	}
}

void IndigoSequenceItem::setIdle() {
	statusLabel->setPixmap(QPixmap(":/resource/led-grey.png"));
}

void IndigoSequenceItem::setBusy() {
	statusLabel->setPixmap(QPixmap(":/resource/led-orange.png"));
}

void IndigoSequenceItem::setAlert() {
	statusLabel->setPixmap(QPixmap(":/resource/led-red.png"));
}

void IndigoSequenceItem::setOk() {
	statusLabel->setPixmap(QPixmap(":/resource/led-green.png"));
}

void IndigoSequenceItem::setIteration(int count) {
    if (type == "repeat" && iterationLabel) {
        iterationLabel->setText(QString("Iteration: %1").arg(count));
    }
}