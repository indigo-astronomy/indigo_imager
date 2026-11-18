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

#ifndef __SCHEDULERPROPERTY_H
#define __SCHEDULERPROPERTY_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QList>

/* A copy of an INDIGO property, so that the scheduler widgets do not depend on
   the INDIGO client. The host converts the properties it receives and sends the
   requests the widgets make. */

enum SchedulerState {
	SCHEDULER_IDLE = 0,	// the values of indigo_property_state
	SCHEDULER_OK = 1,
	SCHEDULER_BUSY = 2,
	SCHEDULER_ALERT = 3
};

struct SchedulerItem {
	QString name;
	QString label;
	QString text;
	double number = 0;
	double target = 0;
	double min = 0;
	double max = 0;
	double step = 0;
	bool on = false;
	int light = SCHEDULER_IDLE;
};

struct SchedulerProperty {
	QString device;
	QString name;
	QString label;
	int state = SCHEDULER_IDLE;
	QVector<SchedulerItem> items;

	const SchedulerItem *item(const QString& itemName) const {
		for (const SchedulerItem& i : items) {
			if (i.name == itemName) return &i;
		}
		return nullptr;
	}
	QString text(const QString& itemName) const {
		const SchedulerItem *i = item(itemName);
		return i ? i->text : QString();
	}
	double number(const QString& itemName) const {
		const SchedulerItem *i = item(itemName);
		return i ? i->number : 0;
	}
	bool on(const QString& itemName) const {
		const SchedulerItem *i = item(itemName);
		return i ? i->on : false;
	}
};

/* One property change to send. The requests of one batch are sent in order. */
struct SchedulerRequest {
	enum Type { Text, Switch, Number };
	Type type = Text;
	QString device;
	QString property;
	QStringList items;
	QStringList texts;
	QList<bool> switches;
	QList<double> numbers;

	static SchedulerRequest text(const QString& device, const QString& property, const QStringList& items, const QStringList& values) {
		SchedulerRequest r; r.type = Text; r.device = device; r.property = property; r.items = items; r.texts = values; return r;
	}
	static SchedulerRequest text(const QString& device, const QString& property, const QString& item, const QString& value) {
		return text(device, property, QStringList{ item }, QStringList{ value });
	}
	static SchedulerRequest switches1(const QString& device, const QString& property, const QString& item, bool value = true) {
		SchedulerRequest r; r.type = Switch; r.device = device; r.property = property; r.items = QStringList{ item }; r.switches = { value }; return r;
	}
};

#endif // __SCHEDULERPROPERTY_H
