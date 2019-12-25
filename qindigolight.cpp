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


#include <QHBoxLayout>
#include "qindigolight.h"


QIndigoLight::QIndigoLight(QIndigoProperty* p, indigo_property* property, indigo_item* item, QWidget *parent)
	: QWidget(parent), QIndigoItem(p, property, item)
{
	led = new QLabel();
	led->setObjectName("INDIGO_property");
	label = new QLabel(m_item->label);
	label->setObjectName("INDIGO_property");
	update();

	//  Lay the labels out somehow in the widget
	QHBoxLayout* hbox = new QHBoxLayout();
	setLayout(hbox);
	hbox->setAlignment(Qt::AlignLeft);
	hbox->setMargin(0);
	hbox->setSpacing(0);
	hbox->addWidget(led);
	hbox->addWidget(label);
	hbox->addStretch();
}

QIndigoLight::~QIndigoLight() {
	//delete led;
	//delete label;
}

void
QIndigoLight::update() {
	switch (m_item->light.value) {
	case INDIGO_IDLE_STATE:
		led->setPixmap(QPixmap(":resource/led-grey.png"));
		break;
	case INDIGO_BUSY_STATE:
		led->setPixmap(QPixmap(":resource/led-orange.png"));
		break;
	case INDIGO_ALERT_STATE:
		led->setPixmap(QPixmap(":resource/led-red.png"));
		break;
	case INDIGO_OK_STATE:
		led->setPixmap(QPixmap(":resource/led-green.png"));
		break;
	}
}
