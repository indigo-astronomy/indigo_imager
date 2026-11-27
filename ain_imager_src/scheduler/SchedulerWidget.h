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

#ifndef __SCHEDULERWIDGET_H
#define __SCHEDULERWIDGET_H

#include <QWidget>
#include <QMap>
#include "SchedulerProperty.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;
class QProgressBar;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
class QDoubleSpinBox;
class QGridLayout;
class SchedulerTaskEditor;
class SchedulerSkyChart;

/* The user interface of the INDIGO Scheduler Agent: a control panel with the
   state of the scheduler, and a view with the task list, the night plan, the
   run history, the settings and the task editor.

   The host passes the properties of the "Scheduler Agent" devices and of the
   Scripting Agent on the same server, and sends the requests of sendRequests(). */
class SchedulerWidget : public QWidget {
	Q_OBJECT
public:
	explicit SchedulerWidget(QWidget *parent = nullptr);

	QWidget *controlPanel() const { return m_control_panel; }

	/* true for the devices whose properties the widget needs */
	static bool isSchedulerDevice(const QString& device);
	bool wantsDevice(const QString& device) const;
	/* the properties the widget needs: of the scheduler and Scripting Agent, and the exposure ones of the Imager Agent */
	bool wantsProperty(const QString& device, const QString& name) const;

	void defineProperty(const SchedulerProperty& property);
	void updateProperty(const SchedulerProperty& property);
	/* the message that came with a property of a scheduler agent, before the property itself */
	void noteMessage(const QString& device, const QString& property, const QString& message);
	/* an empty name deletes all properties of the device */
	void deleteProperty(const QString& device, const QString& name);

	QString selectedAgent() const;
	/* "Scripting Agent" on the server of the selected scheduler */
	QString scriptingAgent() const;
	/* "Imager Agent" on the server of the selected scheduler */
	QString imagerAgent() const;
	/* "Mount Agent" on the server of the selected scheduler, it has the site */
	QString mountAgent() const;
	bool isRunning() const;
	/* the task or hook the scheduler runs now, empty when none */
	QString runningTask() const;

signals:
	void sendRequests(const QList<SchedulerRequest>& requests);
	/* the host enumerates the properties of the newly selected agent */
	void agentSelected(const QString& agent);

private slots:
	void onAgentSelected(int index);
	void onTaskActivated(QTreeWidgetItem *item, int column);
	void onTasksContextMenu(const QPoint& pos);
	void onExportTasks();
	void onImportTasks();

private:
	void buildControlPanel();
	QWidget *buildTasksPage();
	QWidget *buildPlanPage();
	QWidget *buildHistoryPage();
	QWidget *buildHooksPage();
	QWidget *buildSettingsPage();
	void addSetting(QGridLayout *grid, int *row, const char *name);

	void handleProperty(const SchedulerProperty& property);
	void handleScripting(const SchedulerProperty& property);
	void handleImager(const SchedulerProperty& property);
	void handleMount(const SchedulerProperty& property);
	void updateChart();
	void selectionChanged(QTreeWidget *from);
	void updateTaskProgress();
	void updateExposureProgress();
	void resetProgress();
	void clearAgent();

	void updateState();
	void updateTasks();
	void updatePlan();
	void updateHistory();
	void updateSettings();
	void sendHooks();
	void sendSettings();
	void updateEditorRunning();
	void checkPendingResult(const SchedulerProperty& property);

	QString taskProperty(const QString& taskName) const;
	QString taskText(const QString& taskName) const;
	QStringList storedTaskNames() const;
	QString selectedTask() const;

	/* urgent: a run of the history of a one-off urgent task, which is not stored */
	void editTask(const QString& name, bool urgent = false);
	void taskCommand(const QString& name, const QString& command);
	void deleteTask(const QString& name);
	void saveTaskToFile(const QString& name);
	void send(const QList<SchedulerRequest>& requests);
	void sendTask(const QString& property, const QString& text);

	QWidget *m_control_panel;
	QComboBox *m_agent_select;
	QPushButton *m_start_button;
	QPushButton *m_stop_button;
	QPushButton *m_abort_button;
	QLabel *m_state_label;
	QLabel *m_current_label;
	QLabel *m_reason_label;
	QLabel *m_counters_label;
	QLabel *m_next_label;
	QLabel *m_task_status;
	QLabel *m_sequence_label;
	QProgressBar *m_exposure_progress;
	QProgressBar *m_batch_progress;
	QProgressBar *m_sequence_progress;
	SchedulerSkyChart *m_chart;

	QTabWidget *m_tabs;
	QTreeWidget *m_tasks;
	QTreeWidget *m_plan;
	QTreeWidget *m_history;
	SchedulerTaskEditor *m_editor;

	QComboBox *m_night;
	// AGENT_SCHEDULER_STRATEGY, hidden for an agent without it
	QComboBox *m_strategy;
	QLabel *m_strategy_label;
	QMap<QString, QComboBox*> m_hooks;
	QMap<QString, QLabel*> m_hook_labels;
	QComboBox *m_safety_missing;
	QLabel *m_safety_missing_label;
	QLabel *m_safety_title;
	// the items of AGENT_SCHEDULER_SETTINGS at their place on the page, the items this
	// version does not know are added under "Other"
	QMap<QString, QDoubleSpinBox*> m_settings;
	QMap<QString, QLabel*> m_setting_labels;
	QGridLayout *m_other_layout;
	QLabel *m_other_title;
	QStringList m_other_settings;
	QMap<QString, QCheckBox*> m_data;
	QLineEdit *m_data_dir;

	// the properties of the selected scheduler agent by name
	QMap<QString, SchedulerProperty> m_properties;
	// the sequence state of the Scripting Agent on its server
	SchedulerProperty m_sequence_state;
	SchedulerProperty m_step_state;
	SchedulerProperty m_loop_state;
	// the exposure state of the Imager Agent on its server
	SchedulerProperty m_imager_process;
	SchedulerProperty m_imager_stats;
	SchedulerProperty m_imager_batch;
	// the site of the Mount Agent on its server
	SchedulerProperty m_site;
	// the progress shows the last task the scheduler ran, from the start of its sequence to its end
	QString m_progress_current;
	bool m_progress_shown = false;
	bool m_task_started = false;
	bool m_task_finished = false;
	bool m_exposure_running = false;
	// a selection made in one list while the other list follows it
	bool m_syncing_selection = false;

	// the property of the last task upload or edit sent from the editor, until its result arrives
	QString m_pending_property;
	// the last message of each property of the selected agent
	QMap<QString, QString> m_messages;
	// a result shown without a message; its message may come after the property
	QString m_result_property;
	bool m_result_ok = false;
};

#endif // __SCHEDULERWIDGET_H
