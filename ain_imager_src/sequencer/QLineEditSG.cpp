#include "QLineEditSG.h"
#include <QRegularExpressionValidator>
#include <QDebug>

QLineEditSG::QLineEditSG(Mode mode, QWidget* parent) : QLineEdit(parent), mode(mode) {
}

void QLineEditSG::setValue(double value) {
	QString text = SexagesimalConverter::doubleToString(value, 2, mode == DEC);
	setText(text);
}

double QLineEditSG::value() const {
	bool ok;
	double val = SexagesimalConverter::stringToDouble(text(), &ok);
	return ok ? val : 0.0;
}