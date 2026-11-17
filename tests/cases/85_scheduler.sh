# The Scheduler tab: tasks are added from the editor, the scheduler runs them, the lists and the
# state follow the agent, and the task commands reach it.

sched() {	# the SCHED lines of a new dump
	ain_do sched_dump
	tac "$CASE_DIR/ain.out" | sed '/^SCHED_DUMP/q' | tac | grep '^SCHED '
}

sched_has() {	# text, the SCHED lines contain it
	sched | grep -qF "$1"
}

sched_lacks() {
	! sched_has "$1"
}

hook_is() {	# item, value as indigo_prop_tool prints it
	equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_HOOKS' "$1")" "$2"
}

cat > "$CASE_DIR/test.task" <<'EOF'
// @task     Test
// @duration 1

var sequence = new Sequence(scheduler_task.name);
sequence.wait(3);
sequence.start();
EOF

cat > "$CASE_DIR/code.task" <<'EOF'
// @task Code

var sequence = new Sequence(scheduler_task.name);
if (scheduler_task.run > 1) {
	sequence.wait(1);
}
sequence.start();
EOF

cat > "$CASE_DIR/bad.task" <<'EOF'
// @task  Bad
// @bogus 1

var sequence = new Sequence(scheduler_task.name);
sequence.wait(1);
sequence.start();
EOF

rm -rf "$HOME"
services local:$PORT
make_config
start_server indigo_agent_scripting indigo_agent_mount indigo_agent_scheduler
start_ain
ain_do tab Scheduler

section "the agent is found"
check "the scheduler is selected and not started" wait_for 20 sched_has "SCHED SchedulerState Not started"

section "a task from a file is added"
ain_do sched_open "$CASE_DIR/test.task"
check "it is shown in the visual editor" sched_has "SCHED EditorMode visual"
ain_do sched_click "Add to scheduler"
check "the scheduler accepts it" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
check "the task list shows it" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"
check "the editor now edits the stored task" sched_has "SCHED SchedulerEditorTitle Task 'Test'"
ain_do sched_mode Text
ain_do sched_text
ain_do tab "Task editor"
ain_do shot "$CASE_DIR/text.png"
ain_do tab Tasks
check "the stored task has the steps of the editor" grep -q '^SCHED_TEXT.*sequence.wait(3);' "$CASE_DIR/ain.out"
ain_do sched_mode Visual
check "and is shown visually again" sched_has "SCHED EditorMode visual"

section "a rejected task"
ain_do sched_open "$CASE_DIR/bad.task"
ain_do sched_click "Add to scheduler"
check "the reason is shown" wait_for 10 sched_has "unknown key"
check "it is not stored" sched_lacks "SCHED SchedulerTasks Bad"

section "a task with code besides steps"
ain_do sched_open "$CASE_DIR/code.task"
check "it is edited as text" sched_has "SCHED EditorMode text"
ain_do sched_click "Add to scheduler"
check "it is stored" wait_for 10 sched_has "SCHED SchedulerTasks Code | QUEUED"
ain_do sched_select Code
ain_do answer Yes
ain_do sched_click Delete
check "it is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Code"

section "the scheduler runs the task"
ain_do sched_click Start
check "the task runs" wait_for 30 sched_has "SCHED SchedulerCurrent Running: Test"
ain_do shot "$CASE_DIR/running.png"
check "the history shows the run" wait_for 30 sched_has "| Test | run 1 | task | SUCCEEDED"
check "the task is done" wait_for 10 sched_has "SCHED SchedulerTasks Test | DONE"
check "the scheduler stops by itself" wait_for 15 sched_has "SCHED SchedulerState Stopped"

section "task commands"
ain_do sched_select Test
ain_do sched_click Reset
check "reset queues the task again" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"
ain_do sched_click Disable
check "disable" wait_for 10 sched_has "SCHED SchedulerTasks Test | DISABLED"
ain_do sched_click Enable
check "enable" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"

section "an edit of the stored task"
ain_do sched_edit Test
ain_do shot "$CASE_DIR/editor.png"
ain_do tab Steps
ain_do shot "$CASE_DIR/steps.png"
ain_do tab Properties
ain_do sched_click Target
ain_do sched_press "Select the object from the database; an empty task name is set to the object name"
ain_do popup_shot "$CASE_DIR/search.png"
ain_do popup_close
ain_do sched_click Target
check "the object search has its frame" bash -c "grep '^POPUP frame' '$CASE_DIR/ain.out' | tail -1 | grep -qv 'frame [-0]'"
ain_do sched_click "Apply to 'Test'"
check "the edit is accepted" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
ain_do tab "Night plan"
ain_do shot "$CASE_DIR/plan.png"

section "settings"
ain_do tab Settings
ain_do sched_hook IDLE Test
ain_do sched_click Apply
check "the hook is set" wait_for 10 hook_is IDLE '"Test"'
check "the settings keep their values" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_SETTINGS' IDLE_CHECK_INTERVAL)" 30
check "the night limit is kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_NIGHT' NAUTICAL)" ON
check "the data switches are kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_DATA' OBJECT)$(item 'Scheduler Agent.AGENT_SCHEDULER_DATA' FOLDER)" ONOFF
check "the task list shows the hook" wait_for 10 sched_has "SCHED SchedulerTasks Test | HOOK"
ain_do sched_hook IDLE ""
ain_do sched_click Apply
check "the hook is cleared" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"
ain_do tab Tasks

section "delete"
ain_do answer Yes
ain_do sched_select Test
ain_do sched_click Delete
check "the task is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Test"

stop_ain
stop_server
finish_case
