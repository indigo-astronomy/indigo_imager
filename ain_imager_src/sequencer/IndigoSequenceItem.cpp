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
#include <QDateTimeEdit>
#include "SelectObject.h"

IndigoSequenceItem::IndigoSequenceItem(const QString &type, QWidget *parent)
	: QWidget(parent), type(type), overlay(nullptr), isEnabledState(true), m_omitted(false) {
	setObjectName(type);
	setupUI();

	// Initialize statusButton checked state
	statusButton->setChecked(false);

	connect(this, &IndigoSequenceItem::enable, this, &IndigoSequenceItem::setEnabledState);
}

void IndigoSequenceItem::setupUI() {
	frame = new QFrame(this);
	frame->setFrameStyle(QFrame::Box | QFrame::Plain);
	frame->setLineWidth(1);
	frame->setObjectName("IndigoSequenceFrame");
	if (type == SC_REPEAT) {
		//frame->setStyleSheet("QWidget#IndigoSequenceFrame { border: 1px solid #303030; }");
	} else {
		frame->setStyleSheet("QWidget#IndigoSequenceFrame { background-color: #282828; border: 1px solid #292929; }");
	}
	frame->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

	const auto& model = SequenceItemModel::instance();
	auto typeInfo = model.getWidgetTypes()[type];

	if(!model.getWidgetTypes().contains(type)) {
		typeInfo.label = type + "(): unknown call";
	}

	// Replace statusLabel with statusButton
	statusButton = new QToolButton(this);
	statusButton->setCheckable(true);
	statusButton->setFixedSize(20, 20);
	statusButton->setStyleSheet("QToolButton { border: none; background: transparent; }");
	statusButton->setContentsMargins(5, 0, 0, 0);
	statusButton->setToolTip("Toggle execution flag.\n* Item will be executed.");
	connect(statusButton, &QToolButton::clicked, this, &IndigoSequenceItem::toggleOmitted);

	setIdle(); // Initialize with idle icon

	typeLabel = new QLabel(typeInfo.label);
	QFont boldFont = typeLabel->font();
	boldFont.setBold(true);
	typeLabel->setFont(boldFont);
	typeLabel->setContentsMargins(0, 0, 10, 0);
	typeLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	if (type == SC_RECOVERY_POINT || type == SC_CONTINUE_ON_FAILURE || type == SC_ABORT_ON_FAILURE || type == SC_RECOVER_ON_FAILURE) {
		typeLabel->setStyleSheet("color: grey; background: transparent;");
	} else {
		typeLabel->setStyleSheet("background: transparent;");
	}

	deleteButton = new QToolButton(this);
	deleteButton->setText(QString::fromUtf8("\u2715"));
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
	mainLayout->addWidget(statusButton);
	mainLayout->addWidget(typeLabel);
	mainLayout->addStretch();

	QList<int> keys = typeInfo.parameters.keys();
	for (int key : keys) {
		addInputWidget(typeInfo.parameters[key].label, typeInfo.parameters[key].paramWidget, key);
	}

	mainLayout->addWidget(deleteButton);
	mainLayout->setSpacing(4);

	outerLayout = new QVBoxLayout(frame);
	outerLayout->addLayout(mainLayout);
	outerLayout->setContentsMargins(3, 3, 3, 3);
	outerLayout->setSpacing(2);
	outerLayout->setAlignment(Qt::AlignTop);

	if (type == SC_REPEAT) {
		repeatLayout = new QVBoxLayout();
		repeatLayout->setSpacing(5);
		repeatLayout->setContentsMargins(30, 5, 0, 5);
		outerLayout->addLayout(repeatLayout);

		// Add iteration label
		iterationLabel = new QLabel(this);
		setIteration(0);
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

void IndigoSequenceItem::addInputWidget(const QString &paramName, const ParamWidget paramWidget, int key) {
	QLabel *label = new QLabel(paramName+":", this);
	label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	label->setStyleSheet("background: transparent;");
	label->setContentsMargins(3, 0, 0, 0);
	QWidget *input = nullptr;

	switch(paramWidget) {
		case LineEditSG_RA:
			input = new QLineEditSG(QLineEditSG::RA, this);
			break;
		case LineEditSG_DEC:
			input = new QLineEditSG(QLineEditSG::DEC, this);
			break;
		case LineEdit:
			input = new QLineEdit(this);
			break;
		case SpinBox:
			input = new QSpinBox(this);
			break;
		case DoubleSpinBox:
			input = new QDoubleSpinBox(this);
			static_cast<QDoubleSpinBox*>(input)->setDecimals(3);
			break;
		case ComboBox:
			input = new QComboBox(this);
			break;
		case CheckBox:
			input = new QCheckBox(this);
			input->setStyleSheet("QCheckBox { background: transparent; }");
			break;
		case DateTimeEdit:
			input = new QDateTimeEdit(this);
			QDateTimeEdit* dateTimeEdit = static_cast<QDateTimeEdit*>(input);
			dateTimeEdit->setDisplayFormat(DATE_TIME_FORMAT);
			dateTimeEdit->setTimeSpec(Qt::UTC);
			dateTimeEdit->setCurrentSection(QDateTimeEdit::MinuteSection);
			QDateTime currentTimeUtc = QDateTime::currentDateTimeUtc();
			QTime timeWithZeroSeconds(currentTimeUtc.time().hour(), currentTimeUtc.time().minute(), 0);
			currentTimeUtc.setTime(timeWithZeroSeconds);
			dateTimeEdit->setDateTime(currentTimeUtc);
			break;
	}

	const auto& model = SequenceItemModel::instance();

	if (paramWidget == ComboBox) {
		QComboBox* combo = qobject_cast<QComboBox*>(input);
		combo->setEditable(true);
		combo->setInsertPolicy(QComboBox::NoInsert);
		combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
		if (combo) {
			combo->addItems(model.getComboOptions(type, key));
		}
	} else if (paramWidget == SpinBox) {
		QSpinBox* spin = qobject_cast<QSpinBox*>(input);
		if (spin) {
			auto range = model.getNumericRange(type, key);
			spin->setRange(range.first, range.second);
			spin->setSingleStep(model.getNumericIncrement(type, key));
			spin->setValue(model.getNumericDefultValue(type, key));
			spin->setToolTip(QString("%1, range: [%2, %3] step: %4")
				.arg(paramName).arg(range.first).arg(range.second)
				.arg(model.getNumericIncrement(type, key)));
		}
	} else if (paramWidget == DoubleSpinBox) {
		QDoubleSpinBox* spin = qobject_cast<QDoubleSpinBox*>(input);
		if (spin) {
			auto range = model.getNumericRange(type, key);
			spin->setRange(range.first, range.second);
			spin->setSingleStep(model.getNumericIncrement(type, key));
			spin->setValue(model.getNumericDefultValue(type, key));
			spin->setToolTip(QString("%1, range: [%2, %3] step: %4")
				.arg(paramName).arg(range.first).arg(range.second)
				.arg(model.getNumericIncrement(type, key)));
		}
	} else if (paramWidget == LineEdit) {
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

		int raIndex = -1;
		int decIndex = -1;
		const auto& parameters = model.getWidgetTypes()[type].parameters;
		for (auto it = parameters.begin(); it != parameters.end(); ++it) {
			if (it.value().paramWidget == LineEditSG_RA) {
				raIndex = it.key();
			} else if (it.value().paramWidget == LineEditSG_DEC) {
				decIndex = it.key();
			}
		}

		if (raIndex != -1 && decIndex != -1 && paramWidget == LineEditSG_DEC) {
			QToolButton *selectObjectButton = new QToolButton(this);
			selectObjectButton->setIcon(QIcon(":resource/find.png"));
			selectObjectButton->setToolTip("Select object from database");
			connect(selectObjectButton, &QToolButton::clicked, this, [this, selectObjectButton, raIndex, decIndex]() {
				if (!isEnabledState) return;

				QMenu *menu = new QMenu(this);
				SelectObject *selectObjectWidget = new SelectObject(this);
				QWidgetAction *widgetAction = new QWidgetAction(menu);
				widgetAction->setDefaultWidget(selectObjectWidget);
				menu->addAction(widgetAction);

				connect(selectObjectWidget, &SelectObject::objectSelected, this, [this, menu, raIndex, decIndex](const QString &name, double ra, double dec) {
					Q_UNUSED(name);
					if (QLineEditSG *raInput = qobject_cast<QLineEditSG*>(parameterWidgets[raIndex])) {
						raInput->setValue(ra);
					}
					if (QLineEditSG *decInput = qobject_cast<QLineEditSG*>(parameterWidgets[decIndex])) {
						decInput->setValue(dec);
					}
					menu->close();
				});

				menu->exec(selectObjectButton->mapToGlobal(QPoint(0, selectObjectButton->height())));
			});
			mainLayout->addWidget(selectObjectButton);
		}
	}
}

void IndigoSequenceItem::addItem(IndigoSequenceItem *item) {
	if (type == SC_REPEAT && repeatLayout) {
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
	} else if (QDateTimeEdit *dateTimeEdit = qobject_cast<QDateTimeEdit*>(widget)) {
		bool ok;
		qint64 timestamp = value.toLongLong(&ok);
		if (ok) {
			dateTimeEdit->setDateTime(QDateTime::fromSecsSinceEpoch(timestamp, Qt::UTC));
		} else {
			dateTimeEdit->setDateTime(value.toDateTime());
		}
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
		return lineEdit->text().trimmed();
	} else if (QSpinBox *spinBox = qobject_cast<QSpinBox *>(widget)) {
		return spinBox->value();
	} else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox *>(widget)) {
		return doubleSpinBox->value();
	} else if (QComboBox *comboBox = qobject_cast<QComboBox *>(widget)) {
		return comboBox->currentText().trimmed();
	} else if (QCheckBox *checkBox = qobject_cast<QCheckBox *>(widget)) {
		return checkBox->isChecked();
	} else if (QDateTimeEdit *dateTimeEdit = qobject_cast<QDateTimeEdit *>(widget)) {
		return dateTimeEdit->dateTime().toString(DATE_TIME_FORMAT);
	} else {
		return QVariant();
	}
}

void IndigoSequenceItem::removeWidget() {
	if(!isEnabledState) return;
	resetParentSequence();
	delete this;
}

void IndigoSequenceItem::mousePressEvent(QMouseEvent *event) {
	// Prevent triggering a drag when clicking on status button
	if (statusButton->geometry().contains(event->pos())) {
		QWidget::mousePressEvent(event);
		return;
	}

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
		dataStream << m_omitted;

		QVariantMap parameters;
		for (auto it = parameterWidgets.begin(); it != parameterWidgets.end(); ++it) {
			parameters[QString::number(it.key())] = getParameter(it.key());
		}
		dataStream << parameters;

		if (type == SC_REPEAT) {
			int nestedItemCount = repeatLayout->count();
			dataStream << nestedItemCount;

			for (int i = 0; i < nestedItemCount; ++i) {
				IndigoSequenceItem *nestedItem = qobject_cast<IndigoSequenceItem *>(repeatLayout->itemAt(i)->widget());
				if (nestedItem) {
					QString nestedType = nestedItem->getType();
					dataStream << nestedType;
					dataStream << nestedItem->isOmitted();

					QVariantMap nestedParameters;
					for (auto it = nestedItem->parameterWidgets.begin(); it != nestedItem->parameterWidgets.end(); ++it) {
						nestedParameters[QString::number(it.key())] = nestedItem->getParameter(it.key());
					}
					dataStream << nestedParameters;
				}
			}
		}

		QMimeData *mimeData = new QMimeData;
		mimeData->setData("application/x-indigosequenceitem", itemData);

		QDrag *drag = new QDrag(this);
		drag->setMimeData(mimeData);
		showDragOverlay();
		drag->setPixmap(this->grab());

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
	if (type == SC_REPEAT && repeatLayout) {
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
	if (!isEnabledState) return;

	if (type == SC_REPEAT && event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		QByteArray itemData = event->mimeData()->data("application/x-indigosequenceitem");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);
		QString draggedType;
		dataStream >> draggedType;

		// Check if this would exceed nesting limit
		if (draggedType == SC_REPEAT && getNestingLevel() >= 0) {
			event->ignore();
			return;
		}

		event->acceptProposedAction();
	}
}

void IndigoSequenceItem::dragMoveEvent(QDragMoveEvent *event) {
	if (!isEnabledState) return;

	if (type == SC_REPEAT && event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		event->acceptProposedAction();
		int insertAt = determineInsertPosition(event->pos());
		showDropIndicator(insertAt);
	}
}

void IndigoSequenceItem::dragLeaveEvent(QDragLeaveEvent *event) {
	if (type == SC_REPEAT) {
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
	if (!isEnabledState) return;

	if (type == SC_REPEAT && event->mimeData()->hasFormat("application/x-indigosequenceitem")) {
		// Check nesting level
		QByteArray itemData = event->mimeData()->data("application/x-indigosequenceitem");
		QDataStream dataStream(&itemData, QIODevice::ReadOnly);
		QString draggedType;
		bool draggedIsOmitted;
		dataStream >> draggedType >> draggedIsOmitted;

		if (draggedType == SC_REPEAT && getNestingLevel() >= 0) {
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

		// Propagate omitted state from parent loop
		if (isOmitted()) {
			draggedWidget->setOmitted(true);
		}

		draggedWidget->show();

		resetParentSequence();

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
			if (repeatItem->getType() == SC_REPEAT) {
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
	if (!isEnabledState) return;

	if (type != SC_REPEAT) {
		QWidget::contextMenuEvent(event);
		return;
	}

	QMenu contextMenu(this);
	contextMenuPos = event->pos();

	// Determine insert position and show indicator
	int insertAt = determineInsertPosition(contextMenuPos);
	showDropIndicator(insertAt);

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

	// Add categories
	QMap<QString, QMenu*> submenus;
	const auto& submenuCategories = SequenceItemModel::instance().getCategories();
	const auto& categoryIcons = SequenceItemModel::instance().getCategoryIcons();
	int maxWidth = 0;
	QFontMetrics fm(captionLabel->font());
	for (const auto& category : submenuCategories) {
		if (category.first == __SEPARATOR__) {
			contextMenu.addSeparator();
		} else {
			submenus[category.first] = contextMenu.addMenu(categoryIcons[category.first], category.first);
			//submenus[category.first] = contextMenu.addMenu(category.first);
			QAction *menuAction = submenus[category.first]->menuAction();
			QFont boldFontMenu = menuAction->font();
			boldFontMenu.setBold(true);
			menuAction->setFont(boldFontMenu);
			//submenus[category.first]->setFont(boldFontMenu);
			int itemWidth = fm.horizontalAdvance(category.first);
			maxWidth = qMax(maxWidth, itemWidth);
		}
	}
	contextMenu.setMinimumWidth(maxWidth + 75);

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

				if (type == SC_REPEAT && getNestingLevel() >= 0) {
					action->setEnabled(false);
				}

				connect(action, &QAction::triggered, this, &IndigoSequenceItem::addItemFromMenu);

				submenus[category.first]->addAction(action);
			}
		}
	}

	contextMenu.exec(event->globalPos());
	dropIndicator->setVisible(false);
}

void IndigoSequenceItem::addItemFromMenu() {
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action) return;

	QString itemType = action->data().toString();
	IndigoSequenceItem *item = new IndigoSequenceItem(itemType, this);

	if (isOmitted()) {
		item->setOmitted(true);
	}

	// Determine position to insert using the stored position
	int insertAt = determineInsertPosition(contextMenuPos);
	repeatLayout->insertWidget(insertAt, item);
	resetParentSequence();
}

void IndigoSequenceItem::resetParentSequence() {
	IndigoSequence* sequence = nullptr;
	QWidget* parent = parentWidget();
	while (parent) {
		if (sequence = qobject_cast<IndigoSequence*>(parent)) {
			break;
		}
		parent = parent->parentWidget();
	}
	if (sequence) {
		sequence->setIdle();
	}
}

void IndigoSequenceItem::updateNumericRange(int paramId, double min, double max) {
	auto it = parameterWidgets.find(paramId);
	if (it != parameterWidgets.end()) {
		QString paramName = SequenceItemModel::instance().getWidgetTypes()[type].parameters[paramId].label;
		if (QSpinBox* spin = qobject_cast<QSpinBox*>(it.value())) {
			spin->setRange(min, max);
			spin->setToolTip(QString("%1, range: [%2, %3] step: %4").arg(paramName).arg(min).arg(max).arg(spin->singleStep()));
		} else if (QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(it.value())) {
			dspin->setRange(min, max);
			dspin->setToolTip(QString("%1, range: [%2, %3] step: %4").arg(paramName).arg(min).arg(max).arg(dspin->singleStep()));
		}
	}
}

void IndigoSequenceItem::updateNumericIncrement(int paramId, double increment) {
	auto it = parameterWidgets.find(paramId);
	if (it != parameterWidgets.end()) {
		QString paramName = SequenceItemModel::instance().getWidgetTypes()[type].parameters[paramId].label;
		if (QSpinBox* spin = qobject_cast<QSpinBox*>(it.value())) {
			spin->setSingleStep(increment);
			spin->setToolTip(QString("%1, range: [%2, %3] step: %4").arg(paramName).arg(spin->minimum()).arg(spin->maximum()).arg(increment));
		} else if (QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(it.value())) {
			dspin->setSingleStep(increment);
			dspin->setToolTip(QString("%1, range: [%2, %3] step: %4").arg(paramName).arg(dspin->minimum()).arg(dspin->maximum()).arg(increment));
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
	if (this->type == SC_REPEAT) {
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
	if (this->type == SC_REPEAT) {
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
	if (this->type == SC_REPEAT) {
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
	if (m_omitted && type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/led-noexec-cb.png"));
	} else if (m_omitted) {
		statusButton->setIcon(QIcon(":/resource/led-noexec.png"));
	} else if (type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/loop-grey.png"));
	} else if (type == SC_RECOVERY_POINT) {
		statusButton->setIcon(QIcon(":/resource/recovery-point-grey.png"));
	} else if (type == SC_RESUME_POINT) {
		statusButton->setIcon(QIcon(":/resource/resume-grey.png"));
	} else if (type == SC_BREAK_AT || type == SC_BREAK_AT_HA) {
		statusButton->setIcon(QIcon(":/resource/condition-grey.png"));
	} else if (type == SC_CONTINUE_ON_FAILURE || type == SC_ABORT_ON_FAILURE || type == SC_RECOVER_ON_FAILURE) {
		statusButton->setIcon(QIcon(":/resource/recovery_policy.png"));
	} else {
		statusButton->setIcon(QIcon(":/resource/led-grey.png"));
	}
}

void IndigoSequenceItem::setBusy() {
	if (m_omitted && type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/led-noexec-cb.png"));
	} else if (m_omitted) {
		statusButton->setIcon(QIcon(":/resource/led-noexec.png"));
	} else if (type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/loop-orange.png"));
	} else if (type == SC_RECOVERY_POINT) {
		statusButton->setIcon(QIcon(":/resource/recovery-point-grey.png"));
	} else if (type == SC_RESUME_POINT) {
		statusButton->setIcon(QIcon(":/resource/resume-orange.png"));
	} else if (type == SC_BREAK_AT || type == SC_BREAK_AT_HA) {
		statusButton->setIcon(QIcon(":/resource/condition-orange.png"));
	} else if (type == SC_CONTINUE_ON_FAILURE || type == SC_ABORT_ON_FAILURE || type == SC_RECOVER_ON_FAILURE) {
		statusButton->setIcon(QIcon(":/resource/recovery_policy.png"));
	} else {
		statusButton->setIcon(QIcon(":/resource/led-orange.png"));
	}
}

void IndigoSequenceItem::setAlert() {
	if (m_omitted && type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/led-noexec-cb.png"));
	} else if (m_omitted) {
		statusButton->setIcon(QIcon(":/resource/led-noexec.png"));
	} else if (type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/loop-red.png"));
	} else if (type == SC_RECOVERY_POINT) {
		statusButton->setIcon(QIcon(":/resource/recovery-point-grey.png"));
	} else if (type == SC_RESUME_POINT) {
		statusButton->setIcon(QIcon(":/resource/resume-red.png"));
	} else if (type == SC_BREAK_AT || type == SC_BREAK_AT_HA) {
		statusButton->setIcon(QIcon(":/resource/condition-red.png"));
	} else if (type == SC_CONTINUE_ON_FAILURE || type == SC_ABORT_ON_FAILURE || type == SC_RECOVER_ON_FAILURE) {
		statusButton->setIcon(QIcon(":/resource/recovery_policy.png"));
	} else {
		statusButton->setIcon(QIcon(":/resource/led-red.png"));
	}
}

void IndigoSequenceItem::setOk() {
	if (m_omitted && type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/led-noexec-cb.png"));
	} else if (m_omitted) {
		statusButton->setIcon(QIcon(":/resource/led-noexec.png"));
	} else if (type == SC_REPEAT) {
		statusButton->setIcon(QIcon(":/resource/loop-green.png"));
	} else if (type == SC_RECOVERY_POINT) {
		statusButton->setIcon(QIcon(":/resource/recovery-point-green.png"));
	} else if (type == SC_RESUME_POINT) {
		statusButton->setIcon(QIcon(":/resource/resume-green.png"));
	} else if (type == SC_BREAK_AT || type == SC_BREAK_AT_HA) {
		statusButton->setIcon(QIcon(":/resource/condition-green.png"));
	} else if (type == SC_CONTINUE_ON_FAILURE || type == SC_ABORT_ON_FAILURE || type == SC_RECOVER_ON_FAILURE) {
		statusButton->setIcon(QIcon(":/resource/recovery_policy.png"));
	} else {
		statusButton->setIcon(QIcon(":/resource/led-green.png"));
	}
}

void IndigoSequenceItem::setIteration(int count) {
	if (type == SC_REPEAT && iterationLabel) {
		iterationLabel->setText(QString("Iteration: <b>%1</b>").arg(count));
	}
}

void IndigoSequenceItem::setEnabledState(bool enabled) {
	isEnabledState = enabled;

	for (auto it = parameterWidgets.begin(); it != parameterWidgets.end(); ++it) {
		QWidget *widget = it.value();
		if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(widget)) {
			lineEdit->setEnabled(enabled);
		} else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget)) {
			spinBox->setEnabled(enabled);
		} else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
			doubleSpinBox->setEnabled(enabled);
		} else if (QComboBox *comboBox = qobject_cast<QComboBox*>(widget)) {
			comboBox->setEnabled(enabled);
		} else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(widget)) {
			checkBox->setEnabled(enabled);
		} else if (QDateTimeEdit *dateTimeEdit = qobject_cast<QDateTimeEdit*>(widget)) {
			dateTimeEdit->setEnabled(enabled);
		}
	}

	if (type == SC_REPEAT && repeatLayout) {
		for (int i = 0; i < repeatLayout->count(); ++i) {
			IndigoSequenceItem* nestedItem = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(i)->widget());
			if (nestedItem) {
				emit nestedItem->enable(enabled);
			}
		}
	}
}

void IndigoSequenceItem::setOmitted(bool omitted, bool setChildren) {
	if (m_omitted != omitted) {
		m_omitted = omitted;
		statusButton->setChecked(omitted);
		statusButton->setToolTip(omitted ? "Toggle execution flag.\n* Item will NOT be executed." : "Toggle execution flag.\n* Item will be executed.");
		setIdle();

		// Propagate omitted state to nested items if this is a repeat block
		if (type == SC_REPEAT && repeatLayout && setChildren) {
			for (int i = 0; i < repeatLayout->count(); ++i) {
				IndigoSequenceItem* nestedItem = qobject_cast<IndigoSequenceItem*>(repeatLayout->itemAt(i)->widget());
				if (nestedItem) {
					nestedItem->setOmitted(omitted);
				}
			}
		}

		// If this item is not omitted and in a repeat block, ensure the parent repeat block is also not omitted
		if (!omitted) {
			QWidget* parent = parentWidget();
			while (parent) {
				IndigoSequenceItem* parentItem = qobject_cast<IndigoSequenceItem*>(parent);
				if (parentItem && parentItem->type == SC_REPEAT) {
					parentItem->setOmitted(false, false);
					break;
				}
				parent = parent->parentWidget();
			}
		}
	}
}

void IndigoSequenceItem::toggleOmitted() {
	if (!isEnabledState) {
		statusButton->setChecked(m_omitted);
		return;
	}
	bool newState = statusButton->isChecked();
	setOmitted(newState);
	resetParentSequence();
}