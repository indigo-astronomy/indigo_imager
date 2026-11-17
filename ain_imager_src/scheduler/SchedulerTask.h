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

#ifndef __SCHEDULERTASK_H
#define __SCHEDULERTASK_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>

/* A task of the INDIGO Scheduler Agent: a header of "// @key value" lines
   followed by a Sequencer.js body, see INDIGO_SCHEDULER_GUIDE.md.

   The header ends at the first line that does not start with "//", or that is
   a comment without "@", the same way the agent splits it. Empty "//" lines
   inside the header are dropped. Keys are kept in lower case, in their order. */
class SchedulerTask {
public:
	static SchedulerTask fromText(const QString& text);
	QString toText() const;

	QString value(const QString& key) const;
	bool has(const QString& key) const;
	void set(const QString& key, const QString& value);
	void remove(const QString& key);
	const QList<QPair<QString, QString>>& header() const { return m_header; }
	void setHeader(const QList<QPair<QString, QString>>& header) { m_header = header; }

	QString name() const { return value("task"); }

	const QString& body() const { return m_body; }
	void setBody(const QString& body) { m_body = body; }

	/* The header keys the agent knows, in the order the editor writes them. */
	static const QStringList& knownKeys();

private:
	QList<QPair<QString, QString>> m_header;
	QString m_body;
};

/* The columns of an item label of AGENT_SCHEDULER_TASK_STATE, AGENT_SCHEDULER_PLAN
   and AGENT_SCHEDULER_HISTORY: six columns joined by " | ", the last one may
   contain " | " itself. "-" stands for an empty column. */
QStringList scheduler_label_columns(const QString& label);

#endif // __SCHEDULERTASK_H
