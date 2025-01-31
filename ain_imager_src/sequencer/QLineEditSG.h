#ifndef __QLINEREDITSG_H
#define __QLINEREDITSG_H

#include <QLineEdit>
#include "SexagesimalConverter.h"

class QLineEditSG : public QLineEdit {
	Q_OBJECT
public:
	enum Mode { RA, DEC };
	explicit QLineEditSG(Mode mode, QWidget* parent = nullptr);
	void setValue(double value);
	double value() const;

private:
	Mode mode;
};

#endif // __QLINEREDITSG_H