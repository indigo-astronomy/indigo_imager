// Copyright (c) 2019 Rumen G.Bogdanovski & David Hulse
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


#include "qaddcustomobject.h"
//#include <indigo/indigo_bus.h>

QAddCustomObject::QAddCustomObject(QWidget *parent) : QDialog(parent) {
/*	setWindowTitle("Add Object");


	m_view_box = new QWidget();
	m_button_box = new QDialogButtonBox;

	m_add_service_box = new QWidget();

	m_add_button = m_button_box->addButton(tr("Add object"), QDialogButtonBox::ActionRole);
	m_close_button = m_button_box->addButton(tr("Close"), QDialogButtonBox::ActionRole);

	QHBoxLayout* horizontalLayout = new QHBoxLayout;
	horizontalLayout->addWidget(m_button_box);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(m_view_box);
	mainLayout->addLayout(horizontalLayout);

	setLayout(mainLayout);

	QObject::connect(m_add_button, SIGNAL(clicked()), this, SLOT(onAddCustomObject()));
	QObject::connect(m_close_button, SIGNAL(clicked()), this, SLOT(onClose()));
	*/
}

void QAddCustomObject::onClose() {
	close();
}

/*
void QAddCustomObject::onAddCustomObject() {
	QString service_str = m_name_line->text().trimmed();
	if (service_str.isEmpty()) {
		indigo_debug("Trying to add empty service!");
		return;
	}

//	QIndigoService indigo_service(service.toUtf8(), hostname.toUtf8(), port);
//	emit(requestAddManualService(indigo_service));
//	m_service_line->setText("");
//	indigo_debug("ADD: Service '%s' host '%s' port = %d\n", service.toUtf8().constData(), hostname.toUtf8().constData(), port);
}
*/
