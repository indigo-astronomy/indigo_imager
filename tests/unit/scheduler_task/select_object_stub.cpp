// The object database is not needed by the test; it only has to link.
#include "SelectObject.h"

SelectObjectWidget::SelectObjectWidget(QWidget *parent) : QFrame(parent) {}
SelectObjectWidget::~SelectObjectWidget() {}
void SelectObjectWidget::showEvent(QShowEvent *event) { QFrame::showEvent(event); }
void SelectObjectWidget::onSearchTextChanged(const QString &) {}
void SelectObjectWidget::onSearchEntered() {}
void SelectObjectWidget::onCustomObjectsOnlyChecked(bool) {}
void SelectObjectWidget::onObjectSelected() {}
void SelectObjectWidget::onObjectClicked(QListWidgetItem *) {}
void SelectObjectWidget::updateObjectList(const QString &) {}
