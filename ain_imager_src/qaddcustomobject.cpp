// Copyright (c) 2022 Rumen G.Bogdanovski
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

#include <QLabel>
#include "qaddcustomobject.h"
#include <indigo/indigo_bus.h>
#include <QRegExp>
#include <widget_state.h>

QAddCustomObject::QAddCustomObject(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Add Object");

	QFrame *frame = new QFrame;
	QGridLayout *frame_layout = new QGridLayout();
	frame_layout->setAlignment(Qt::AlignTop);
	frame->setLayout(frame_layout);
	frame->setContentsMargins(0, 0, 0, 0);
	frame_layout->setContentsMargins(0, 0, 0, 10);
	frame_layout->setColumnStretch(0, 1);
	frame_layout->setColumnStretch(1, 4);

	int row = 0;
	QLabel *label = new QLabel("Object name:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_name_line = new QLineEdit();
	m_name_line->setMinimumWidth(200);
	set_ok(m_name_line);
	frame_layout->addWidget(m_name_line , row, 1, 1, 2);

	row++;
	label = new QLabel("Right ascension:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_ra_line = new QLineEdit();
	frame_layout->addWidget(m_ra_line , row, 1);

	m_load_ra_de_button = new QToolButton(this);
	m_load_ra_de_button->setToolTip(tr("Load telescope coordinates"));
	m_load_ra_de_button->setIcon(QIcon(":resource/guide.png"));
	frame_layout->addWidget(m_load_ra_de_button, row, 2, 2, 1);
	QObject::connect(m_load_ra_de_button, SIGNAL(clicked()), this, SLOT(onRequestRADec()));

	row++;
	label = new QLabel("Declination:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_dec_line = new QLineEdit();
	frame_layout->addWidget(m_dec_line , row, 1);

	row++;
	label = new QLabel("Magnitude:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_mag_box = new QDoubleSpinBox();
	m_mag_box->setRange(-13, 30);
	frame_layout->addWidget(m_mag_box, row, 1, 1, 2);

	row++;
	label = new QLabel("Description:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_description_line = new QLineEdit();
	frame_layout->addWidget(m_description_line, row, 1, 1, 2);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	m_button_box = new QDialogButtonBox;
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);
	m_add_button = m_button_box->addButton(tr("Add object"), QDialogButtonBox::ActionRole);
	horizontalLayout->addWidget(m_button_box);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(frame);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);
	onClear();

	QObject::connect(m_add_button, SIGNAL(clicked()), this, SLOT(onAddCustomObject()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onClose()));
	connect(this, &QAddCustomObject::populate, this, &QAddCustomObject::onPopulate);
	connect(this, &QAddCustomObject::clear, this, &QAddCustomObject::onClear);
}

void QAddCustomObject::onClose() {
	close();
	onClear();
}

void QAddCustomObject::onAddCustomObject() {
	bool error = false;
	QString name_str = m_name_line->text().trimmed();
	if (name_str.isEmpty()) {
		indigo_debug("Object name is empty");
		m_name_line->setPlaceholderText("Object name is empty");
		set_alert(m_name_line);
		error = true;
	} else {
		m_name_line->setPlaceholderText("Name");
		set_ok(m_name_line);
	}

	QString ra_str = m_ra_line->text().trimmed();
	if (ra_str.isEmpty()) {
		indigo_debug("Right ascension is empty");
		m_ra_line->setPlaceholderText("RA is empty");
		set_alert(m_ra_line);
		error = true;
	} else {
		m_ra_line->setPlaceholderText("hh:mm:ss");
		set_ok(m_ra_line);
	}

	QString dec_str = m_dec_line->text().trimmed();
	if (dec_str.isEmpty()) {
		indigo_debug("Declination is empty");
		m_dec_line->setPlaceholderText("Dec is empty");
		set_alert(m_dec_line);
		error = true;
	} else {
		m_dec_line->setPlaceholderText("dd:mm:ss");
		set_ok(m_dec_line);
	}

	QRegExp ra_re("\\d*:?\\d*:?\\d*\\.?\\d*");
	double ra = indigo_stod(ra_str.toUtf8().data());
	if (ra < 0 || ra > 24 || !ra_re.exactMatch(ra_str)) {
		indigo_debug("Right ascenstion is not valid");
		set_alert(m_ra_line);
		error = true;
	}
	QRegExp dec_re("[+-]?\\d*:?\\d*:?\\d*\\.?\\d*");
	double dec = indigo_stod(dec_str.toUtf8().data());
	if (dec < -90 || dec > 90 || !dec_re.exactMatch(dec_str)) {
		indigo_debug("Declination is not valid");
		set_alert(m_dec_line);
		error = true;
	}

	double mag = m_mag_box->value();
	QString description_str = m_description_line->text().trimmed();

	if (!error) {
		// set_alert() because if object exists it will remain alert, if it succeeds it should be cleared with clear()
		set_alert(m_name_line);
		CustomObject object(name_str, ra, dec, mag, description_str);
		emit(requestAddCustomObject(object));
		indigo_debug("ADD: Object '%s' (RA = %f, Dec = %f, Mag = %f)\n", name_str.toUtf8().constData(), ra, dec, mag);
	}
}

void QAddCustomObject::onPopulate(QString name, QString ra, QString dec, double mag, QString description) {
	indigo_debug("%s()", __FUNCTION__);
	if (!name.isNull()) {
		m_name_line->setText(name);
	}
	if (!ra.isNull()) {
		m_ra_line->setText(ra);
	}
	if (!dec.isNull()) {
		m_dec_line->setText(dec);
	}
	if (mag > -30) {
		m_mag_box->setValue(mag);
	}
	if (!description.isNull()) {
		m_description_line->setText(description);
	}
}

void QAddCustomObject::onRequestRADec() {
	indigo_debug("%s()", __FUNCTION__);
	emit(requestPopulate());
}

void QAddCustomObject::onClear() {
	m_name_line->setText("");
	m_name_line->setPlaceholderText("Name");
	set_ok(m_name_line);

	m_ra_line->setText("");
	m_ra_line->setPlaceholderText("hh:mm:ss");
	set_ok(m_ra_line);

	m_dec_line->setText("");
	m_dec_line->setPlaceholderText("dd:mm:ss");
	set_ok(m_dec_line);

	m_mag_box->setValue(0);
	m_description_line->setText("");
	m_description_line->setPlaceholderText("Object description");
}
