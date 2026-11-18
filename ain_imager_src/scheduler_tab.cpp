// Copyright (c) 2026 Rumen G.Bogdanovski
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

// The Scheduler tab: the SchedulerWidget gets copies of the properties of the
// Scheduler Agent and of the Scripting Agent on its server, and its requests
// are sent here.

#include "imagerwindow.h"
#include "indigoclient.h"
#include "propertycache.h"
#include <conf.h>

static SchedulerProperty scheduler_property(indigo_property *property) {
	SchedulerProperty copy;
	copy.device = QString::fromUtf8(property->device);
	copy.name = QString::fromUtf8(property->name);
	copy.label = QString::fromUtf8(property->label);
	copy.state = property->state;
	copy.items.reserve(property->count);
	for (int i = 0; i < property->count; i++) {
		indigo_item *item = &property->items[i];
		SchedulerItem copy_item;
		copy_item.name = QString::fromUtf8(item->name);
		copy_item.label = QString::fromUtf8(item->label);
		switch (property->type) {
			case INDIGO_TEXT_VECTOR:
				// task texts are longer than INDIGO_VALUE_SIZE
				copy_item.text = QString::fromUtf8(indigo_get_text_item_value(item));
				break;
			case INDIGO_NUMBER_VECTOR:
				copy_item.number = item->number.value;
				copy_item.target = item->number.target;
				copy_item.min = item->number.min;
				copy_item.max = item->number.max;
				copy_item.step = item->number.step;
				break;
			case INDIGO_SWITCH_VECTOR:
				copy_item.on = item->sw.value;
				break;
			case INDIGO_LIGHT_VECTOR:
				copy_item.light = item->light.value;
				break;
			default:
				break;
		}
		copy.items.append(copy_item);
	}
	return copy;
}

void ImagerWindow::create_scheduler_tab(QFrame *scheduler_frame) {
	QVBoxLayout *layout = new QVBoxLayout();
	layout->setAlignment(Qt::AlignTop);
	layout->setContentsMargins(0, 0, 0, 0);
	scheduler_frame->setLayout(layout);
	scheduler_frame->setFrameShape(QFrame::StyledPanel);
	scheduler_frame->setMinimumWidth(TOOLBAR_MIN_WIDTH);
	scheduler_frame->setContentsMargins(0, 0, 0, 0);

	m_scheduler = new SchedulerWidget();
	m_scheduler->setVisible(false);
	layout->addWidget(m_scheduler->controlPanel());

	connect(m_scheduler, &SchedulerWidget::sendRequests, this, &ImagerWindow::on_scheduler_send_requests);
	connect(m_scheduler, &SchedulerWidget::agentSelected, this, &ImagerWindow::on_scheduler_agent_selected);
}

void ImagerWindow::on_scheduler_agent_selected(const QString& agent) {
	if (agent.isEmpty()) {
		return;
	}
	QtConcurrent::run([=]() {
		indigo_enumerate_properties(nullptr, &INDIGO_ALL_PROPERTIES);
	});
}

void ImagerWindow::on_scheduler_send_requests(const QList<SchedulerRequest>& requests) {
	// one job sends the batch in order, so that a task name precedes its command
	QtConcurrent::run([=]() {
		for (const SchedulerRequest& request : requests) {
			QByteArray device = request.device.toUtf8();
			QByteArray property = request.property.toUtf8();
			QList<QByteArray> item_names;
			for (const QString& item : request.items) {
				item_names.append(item.toUtf8());
			}
			QVector<const char*> items;
			for (const QByteArray& item : item_names) {
				items.append(item.constData());
			}
			indigo_debug("[SCHEDULER] %s.%s (%d items)", device.constData(), property.constData(), int(items.size()));
			switch (request.type) {
				case SchedulerRequest::Text: {
					QList<QByteArray> texts;
					for (const QString& text : request.texts) {
						texts.append(text.toUtf8());
					}
					QVector<const char*> values;
					for (const QByteArray& text : texts) {
						values.append(text.constData());
					}
					indigo_change_text_property(nullptr, device.constData(), property.constData(), items.size(), items.data(), values.data());
					break;
				}
				case SchedulerRequest::Switch: {
					QVector<bool> values;
					for (bool value : request.switches) {
						values.append(value);
					}
					indigo_change_switch_property(nullptr, device.constData(), property.constData(), items.size(), items.data(), values.data());
					break;
				}
				case SchedulerRequest::Number: {
					QVector<double> values;
					for (double value : request.numbers) {
						values.append(value);
					}
					indigo_change_number_property(nullptr, device.constData(), property.constData(), items.size(), items.data(), values.data());
					break;
				}
			}
		}
	});
}

void ImagerWindow::scheduler_property_define(indigo_property *property) {
	m_scheduler->defineProperty(scheduler_property(property));
}

void ImagerWindow::scheduler_property_change(indigo_property *property) {
	m_scheduler->updateProperty(scheduler_property(property));
}

void ImagerWindow::scheduler_property_delete(indigo_property *property) {
	m_scheduler->deleteProperty(QString::fromUtf8(property->device), QString::fromUtf8(property->name));
}
