// Copyright (c) 2022 Rumen G.Bogdanovski & David Hulse
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
#include <widget_state.h>

QAddCustomObject::QAddCustomObject(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Add Object");

	QWidget *frame = new QWidget;
	QGridLayout *frame_layout = new QGridLayout();
	frame_layout->setAlignment(Qt::AlignTop);
	frame->setLayout(frame_layout);
	frame->setContentsMargins(0, 0, 0, 0);
	frame_layout->setColumnStretch(0, 1);
	frame_layout->setColumnStretch(1, 4);
	//frame_layout->setColumnStretch(2, 1);
	//frame_layout->setColumnStretch(3, 1);

	int row = 0;
	QLabel *label = new QLabel("Object name:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_name_line = new QLineEdit();
	m_name_line->setPlaceholderText("Name");
	set_ok(m_name_line);
	frame_layout->addWidget(m_name_line , row, 1);

	row++;
	label = new QLabel("Right Ascension:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_ra_line = new QLineEdit();
	m_ra_line->setPlaceholderText("hh:mm:ss");
	set_alert(m_ra_line);
	frame_layout->addWidget(m_ra_line , row, 1);

	row++;
	label = new QLabel("Declination:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_dec_line = new QLineEdit();
	m_dec_line->setPlaceholderText("+dd:mm:ss");
	frame_layout->addWidget(m_dec_line , row, 1);

	row++;
	label = new QLabel("Magnitude:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_mag_box = new QDoubleSpinBox();
	frame_layout->addWidget(m_mag_box, row, 1);

	row++;
	label = new QLabel("Description:");
	label->setStyleSheet(QString("QLabel { font-weight: bold; }"));
	frame_layout->addWidget(label, row, 0);
	m_description_line = new QLineEdit();
	m_description_line->setPlaceholderText("Onbjct description");
	frame_layout->addWidget(m_description_line, row, 1);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	m_button_box = new QDialogButtonBox;
	m_add_button = m_button_box->addButton(tr("Add object"), QDialogButtonBox::ActionRole);
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);
	horizontalLayout->addWidget(m_button_box);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(frame);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);

	QObject::connect(m_add_button, SIGNAL(clicked()), this, SLOT(onAddCustomObject()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onClose()));
}

void QAddCustomObject::onClose() {
	close();
}


void QAddCustomObject::onAddCustomObject() {
	//QString service_str = m_name_line->text().trimmed();
	//if (service_str.isEmpty()) {
	//	indigo_debug("Trying to add empty service!");
	//	return;
	//}

	CustomObject object("Test", 12, 13, 5.0, "info");
//	QIndigoService indigo_service(service.toUtf8(), hostname.toUtf8(), port);
	emit(requestAddCustomObject(object));
//	m_service_line->setText("");
//	indigo_debug("ADD: Service '%s' host '%s' port = %d\n", service.toUtf8().constData(), hostname.toUtf8().constData(), port);
}
