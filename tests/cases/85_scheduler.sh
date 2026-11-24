# The Scheduler tab: tasks are added from the editor, the scheduler runs them, the lists and the
# state follow the agent, and the task commands reach it.

sched() {	# the SCHED lines of a new dump
	ain_do sched_dump
	tac "$CASE_DIR/ain.out" | sed '/^SCHED_DUMP/q' | tac | grep '^SCHED '
}

sched_has() {	# text, the SCHED lines contain it
	sched | grep -qF "$1"
}

sched_has_all() {	# texts, the SCHED lines of one dump contain all of them
	local lines
	lines=$(sched)
	for text in "$@"; do
		grep -qF "$text" <<< "$lines" || return 1
	done
}

seq() {	# the SEQ lines of a new dump of the Sequence tab
	ain_do seq_dump
	tac "$CASE_DIR/ain.out" | sed '/^SEQ_DUMP/q' | tac | grep '^SEQ '
}

seq_has() {
	seq | grep -qF "$1"
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
sequence.wait(2);
sequence.wait(3);
sequence.start();
EOF

cat > "$CASE_DIR/ain.js" <<'EOF'
var sequence = new Sequence("ain test");
sequence.wait(1);
sequence.wait(3);
sequence.start();
EOF

cat > "$CASE_DIR/capture.task" <<EOF
// @task Capture

var sequence = new Sequence(scheduler_task.name);
sequence.select_imager_camera("$SIM_CAMERA");
sequence.capture_batch(2, 2);
sequence.start();
EOF

cat > "$CASE_DIR/long.task" <<'EOF'
// @task Long

var sequence = new Sequence(scheduler_task.name);
sequence.wait(120);
sequence.start();
EOF

cat > "$CASE_DIR/standby.task" <<'EOF'
// @task     Standby
// @priority hook
// @duration 1

var sequence = new Sequence(scheduler_task.name);
sequence.wait(1);
sequence.start();
EOF

cat > "$CASE_DIR/m31.task" <<'EOF'
// @task     M31
// @target   00:42:44 +41:16:09
// @altitude > 30

var sequence = new Sequence(scheduler_task.name);
sequence.wait(1);
sequence.start();
EOF

cat > "$CASE_DIR/south.task" <<'EOF'
// @task     South
// @target   12:30:00 -80:00:00

var sequence = new Sequence(scheduler_task.name);
sequence.wait(1);
sequence.start();
EOF

cat > "$CASE_DIR/flats.task" <<'EOF'
// @task     Flats
// @priority normal 40
// @twilight nautical
// @repeat   3

var sequence = new Sequence(scheduler_task.name);
sequence.wait(1);
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

cat > "$CASE_DIR/hook.task" <<'EOF'
// @task     Park
// @priority hook
// @duration 1

var sequence = new Sequence(scheduler_task.name);
sequence.wait(1);
sequence.start();
EOF

rm -rf "$HOME"
services local:$PORT
make_config
start_server indigo_agent_scripting indigo_agent_imager indigo_agent_mount indigo_agent_scheduler indigo_ccd_simulator
start_ain
ain_do tab Scheduler

section "the agent is found"
check "the scheduler is selected and not started" wait_for 20 sched_has "SCHED SchedulerState Not started"
check "no task progress is shown" sched_has_all "SCHED SchedulerTaskStatus Idle" "SCHED SchedulerExposure Exposure: Idle" "SCHED SchedulerBatch Batch: Idle" "SCHED SchedulerProgress Task: Idle"

section "a task from a file is added"
ain_do sched_open "$CASE_DIR/test.task"
check "it is shown in the visual editor" sched_has "SCHED EditorMode visual"
ain_do sched_click Apply
check "the scheduler accepts it" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
check "the task list shows it" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"
check "the editor now edits the stored task" sched_has "SCHED SchedulerEditorTitle Task 'Test'"
ain_do sched_page Text
ain_do sched_text
ain_do tab "Task editor"
ain_do shot "$CASE_DIR/text.png"
ain_do tab Tasks
check "the stored task has the steps of the editor" grep -q '^SCHED_TEXT.*sequence.wait(3);' "$CASE_DIR/ain.out"
ain_do sched_page Steps
check "and is shown visually again" sched_has "SCHED EditorMode visual"

section "a rejected task"
ain_do sched_open "$CASE_DIR/bad.task"
ain_do sched_click Apply
check "the reason is shown" wait_for 10 sched_has "unknown key"
check "it is not stored" sched_lacks "SCHED SchedulerTasks Bad"

section "a task with code besides steps"
ain_do sched_open "$CASE_DIR/code.task"
check "it is edited as text" sched_has "SCHED EditorMode text"
ain_do sched_page Steps
check "the forms refuse it and say why" sched_has "SCHED EditorMode text"
check "the reason is shown" sched_has "SCHED SchedulerEditorStatus The task can only be edited as text"
ain_do sched_click Apply
check "it is stored" wait_for 10 sched_has "SCHED SchedulerTasks Code | QUEUED"
ain_do sched_select Code
ain_do answer Yes
ain_do sched_click "Delete the task; a running task is aborted first"
check "it is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Code"

section "the Sequence tab runs its own sequence"
ain_do tab Sequence
ain_do loadseq "$CASE_DIR/ain.js"
sleep 2
ain_do button Run
check "it shows the progress of its run" wait_for 15 seq_has "SEQ SequenceSteps ok busy"
check "and the end of it" wait_for 15 seq_has "SEQ SequenceStatus Complete"
seq >/dev/null

section "the scheduler runs the task"
ain_do tab Scheduler
ain_do sched_edit Test
ain_do tab "Task editor"
ain_do sched_page Scheduling
check "the editor shows the steps of the stored task" sched_has "SCHED EditorSteps idle idle"
ain_do sched_click Start
check "the task runs" wait_for 30 sched_has "SCHED SchedulerCurrent Running: Test"
check "the editor shows it running and read only" wait_for 10 sched_has "SCHED SchedulerEditorTitle Task 'Test' - running, read only"
check "the editor switches to the steps" sched_has "SCHED EditorMode visual"
check "the first step is busy, the panel shows it" wait_for 10 sched_has_all "SCHED EditorSteps busy idle" "SCHED SchedulerTaskStatus In progress" "SCHED SchedulerProgress Task: 0% complete"
check "the second step is busy after the first is done, the panel shows it" wait_for 10 sched_has_all "SCHED EditorSteps ok busy" "SCHED SchedulerTaskStatus In progress" "SCHED SchedulerProgress Task: 50% complete"
seq > "$CASE_DIR/seq_running.txt"
check "the Sequence tab does not show the task of the scheduler" bash -c "! grep -qE 'busy|In progress|Sequence: [1-9]' '$CASE_DIR/seq_running.txt'"
ain_do shot "$CASE_DIR/running.png"
ain_do sched_deselect
check "Escape clears the selection, the running task is not highlighted unless selected" sched_has_all "SCHED SchedulerTasksSelected none" "| highlighted  |"
check "the history shows the run" wait_for 30 sched_has "| Test | run 1 | task | SUCCEEDED"
check "the task is done" wait_for 10 sched_has "SCHED SchedulerTasks Test | DONE"
check "the chart shows the finished run in green" sched_has "| ran Test:ok"
check "the editor allows edits again" wait_for 10 sched_lacks "running, read only"
check "the panel shows the end of the task" sched_has_all "SCHED SchedulerTaskStatus Complete" "SCHED SchedulerProgress Task: 100% complete"
ain_do tab Tasks
check "the scheduler stops by itself" wait_for 15 sched_has "SCHED SchedulerState Stopped"
check "the panel still shows the end of the last task" sched_has "SCHED SchedulerTaskStatus Complete"

section "a task with exposures"
ain_do sched_open "$CASE_DIR/capture.task"
ain_do sched_click Apply
check "the scheduler accepts it" wait_for 10 sched_has "SCHED SchedulerTasks Capture | QUEUED"
ain_do sched_click Start
check "the task runs" wait_for 30 sched_has "SCHED SchedulerCurrent Running: Capture"
check "the panel shows the exposure and the batch" wait_for 30 sched_has_all "SCHED SchedulerTaskStatus In progress" "seconds elapsed" "SCHED SchedulerBatch Batch: exposure" "of 2 complete"
ain_do shot "$CASE_DIR/exposure.png"
check "and the end of them" wait_for 30 sched_has_all "SCHED SchedulerTaskStatus Complete" "SCHED SchedulerExposure Exposure: Complete" "SCHED SchedulerBatch Batch: Complete" "SCHED SchedulerProgress Task: 100% complete"
check "the scheduler stops by itself" wait_for 15 sched_has "SCHED SchedulerState Stopped"
ain_do answer Yes
ain_do sched_select Capture
ain_do sched_click "Delete the task; a running task is aborted first"
check "it is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Capture"

section "edits while a task runs"
ain_do sched_open "$CASE_DIR/long.task"
ain_do tab "Task editor"
ain_do sched_click Apply
check "the long task is stored" wait_for 10 sched_has "SCHED SchedulerTasks Long | QUEUED"
ain_do sched_click Start
check "it runs" wait_for 30 sched_has "SCHED SchedulerCurrent Running: Long"
check "the editor shows it read only" wait_for 10 sched_has "SCHED SchedulerEditorTitle Task 'Long' - running, read only"
check "all its pages can be shown" sched_has "SCHED EditorTabs Scheduling:on Steps:on Extra:on Text:on"
ain_do sched_page Text
check "its text is shown read only" sched_has_all "SCHED EditorMode text" "SCHED EditorText read only"
ain_do sched_page Steps
check "and its steps again" sched_has_all "SCHED EditorMode visual" "SCHED EditorSteps busy" "running, read only"
ain_do sched_click New
check "a new task can be edited" sched_has "SCHED SchedulerEditorTitle New task"
ain_do sched_open "$CASE_DIR/standby.task"
ain_do sched_click Apply
check "a task from a file is uploaded" wait_for 10 sched_has "SCHED SchedulerTasks Standby | HOOK"
ain_do sched_page Scheduling
ain_do sched_click Duration
ain_do sched_click Apply
check "the stored task is edited" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
ain_do sched_edit Standby
ain_do sched_page Text
ain_do sched_text
check "the scheduler has the edit" bash -c "grep '^SCHED_TEXT' '$CASE_DIR/ain.out' | tail -1 | grep 'Standby' | grep -qv 'duration'"
check "Apply was never disabled" bash -c "! grep -q '^DISABLED Apply' '$CASE_DIR/ain.out'"
check "the long task still runs" sched_has "SCHED SchedulerCurrent Running: Long"
ain_do sched_edit Long
check "and is read only when shown again" wait_for 10 sched_has "SCHED SchedulerEditorTitle Task 'Long' - running, read only"
ain_do sched_click Apply
check "and can not be sent" bash -c "grep -q '^DISABLED Apply' '$CASE_DIR/ain.out'"
ain_do answer Yes
ain_do sched_click "Abort the running task at once, run the shutdown hook and stop"
check "the scheduler is aborted" wait_for 30 sched_has "SCHED SchedulerState Stopped"
ain_do tab Tasks
for task in Long Standby; do
	ain_do answer Yes
	ain_do sched_select $task
	ain_do sched_click "Delete the task; a running task is aborted first"
	check "$task is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks $task"
done

section "task commands"
ain_do sched_select Test
ain_do sched_click Reset
check "reset queues the task again" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"

section "an edit of the stored task"
ain_do sched_edit Test
ain_do shot "$CASE_DIR/editor.png"
ain_do tab Steps
ain_do shot "$CASE_DIR/steps.png"
ain_do tab Extra
ain_do shot "$CASE_DIR/extra.png"
ain_do tab Scheduling
ain_do sched_click Target
ain_do shot "$CASE_DIR/target.png"
ain_do sched_press "Select the object from the database; an empty task name is set to the object name"
ain_do popup_shot "$CASE_DIR/search.png"
ain_do popup_close
ain_do sched_click Target
check "the object search has its frame" bash -c "grep '^POPUP frame' '$CASE_DIR/ain.out' | tail -1 | grep -qv 'frame [-0]'"
ain_do sched_click Repeat
ain_do answer No
ain_do sched_click New
check "a new task asks to discard the changes of the task by its name" bash -c "grep '^QUESTION' '$CASE_DIR/ain.out' | tail -1 | grep -qF \"The task 'Test' is being edited and has changes\""
check "and keeps them when not agreed" sched_has "SCHED SchedulerEditorTitle Task 'Test'"
ain_do sched_click Repeat
ain_do sched_click Apply
check "the edit is accepted" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
ain_do sched_open "$CASE_DIR/test.task"
ain_do sched_click Apply
check "a file with the name of a stored task updates it" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
check "and adds no other task" equals "$(sched | grep -c '^SCHED SchedulerTasks Test |')" 1
ain_do tab "Night plan"
ain_do shot "$CASE_DIR/plan.png"

section "hooks and settings"
ain_do tab Hooks
ain_do sched_hook IDLE Test
check "the hook is set" wait_for 10 hook_is IDLE '"Test"'
check "the settings keep their values" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_SETTINGS' IDLE_CHECK_INTERVAL)" 30
check "the night limit is kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_NIGHT' NAUTICAL)" ON
check "the data switches are kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_DATA' OBJECT)$(item 'Scheduler Agent.AGENT_SCHEDULER_DATA' FOLDER)" ONOFF
check "the page shows the settings of the agent" sched_has_all "SCHED Setting IDLE_CHECK_INTERVAL 30 on" "SCHED Setting SETTING_WEIGHT 1.0 on" "SCHED Setting VISIBILITY_FLIP_PENALTY 10 on" "SCHED Setting NIGHT_START_OFFSET 0 on" "SCHED SchedulerStrategy COMPACT on" "SCHED SchedulerSafetyMissing SAFE on" "SCHED Setting SAFE_DELAY 10.0 on"
check "every setting has its place" equals "$(sched | grep -c '^SCHED Setting ')" 12
setting_is() {	# item, value
	equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_SETTINGS' "$1")" "$2"
}
ain_do sched_setting IDLE_HOOK_DELAY 45
check "a setting is sent" wait_for 10 setting_is IDLE_HOOK_DELAY 45
check "and the other settings are kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_SETTINGS' IDLE_CHECK_INTERVAL)" 30
check "the page shows the new value" wait_for 10 sched_has "SCHED Setting IDLE_HOOK_DELAY 45 on"
ain_do sched_setting IDLE_HOOK_DELAY 30
check "and it is set back" wait_for 10 setting_is IDLE_HOOK_DELAY 30
ain_do sched_hook UNSAFE Test
check "a safety hook is set" wait_for 10 hook_is UNSAFE '"Test"'
check "and the other hooks are kept" wait_for 10 hook_is IDLE '"Test"'
ain_do sched_hook UNSAFE ""
check "and it is cleared" wait_for 10 hook_is UNSAFE '""'
ain_do shot "$CASE_DIR/hooks.png"
ain_do tab Settings
strategy_is() {	# item that is on
	equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_STRATEGY' "$1")" ON
}
ain_do sched_choose SchedulerStrategy VISIBILITY
check "the best visibility strategy is sent" wait_for 10 strategy_is VISIBILITY
check "the page shows it" wait_for 10 sched_has "SCHED SchedulerStrategy VISIBILITY on"
ain_do sched_choose SchedulerStrategy COMPACT
check "and compact is set back" wait_for 10 strategy_is COMPACT
safety_missing_is() {	# item that is on
	equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_SAFETY_MISSING' "$1")" ON
}
ain_do sched_choose SchedulerSafetyMissing UNSAFE
check "missing safety information treated as unsafe is sent" wait_for 10 safety_missing_is UNSAFE
check "the page shows it" wait_for 10 sched_has "SCHED SchedulerSafetyMissing UNSAFE on"
ain_do sched_choose SchedulerSafetyMissing SAFE
check "and safe is set back" wait_for 10 safety_missing_is SAFE
ain_do shot "$CASE_DIR/settings.png"
check "the task list shows the hook" wait_for 10 sched_has "SCHED SchedulerTasks Test | HOOK"
ain_do sched_hook IDLE ""
check "the hook is cleared" wait_for 10 sched_has "SCHED SchedulerTasks Test | QUEUED"
ain_do tab History
ain_do shot "$CASE_DIR/history.png"
ain_do tab Tasks
ain_do shot "$CASE_DIR/tasks.png"

section "delete"
ain_do answer Yes
ain_do sched_select Test
ain_do sched_click "Delete the task; a running task is aborted first"
check "the task is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Test"

section "a hook task"
ain_do sched_open "$CASE_DIR/hook.task"
ain_do tab "Task editor"
ain_do sched_page Scheduling
ain_do shot "$CASE_DIR/hook.png"
check "it is shown in the visual editor" sched_has "SCHED EditorMode visual"
ain_do sched_click Apply
check "the scheduler accepts it" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
check "the task list shows it as a hook" wait_for 10 sched_has "SCHED SchedulerTasks Park | HOOK | hook"
ain_do answer Yes
ain_do sched_select Park
ain_do sched_click "Delete the task; a running task is aborted first"
check "it is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Park"

section "the chart of the night"
prop_set "Mount Agent.GEOGRAPHIC_COORDINATES.LATITUDE=42.7;LONGITUDE=23.3"
check "it has the site of the Mount Agent" wait_for 10 sched_has "SCHED SchedulerChart site 42.7 23.3"
ain_do sched_open "$CASE_DIR/m31.task"
ain_do sched_click Apply
check "the task is stored" wait_for 10 sched_has "SCHED SchedulerTasks M31 | QUEUED"
ain_do tab Tasks
ain_do sched_select M31
check "it shows the target of the task, the selected task over the others" wait_for 10 sched_has_all "| targets M31 |" "| highlighted M31"
check "and its run in the night plan" wait_for 10 sched_has "| runs M31 |"
ain_do sched_open "$CASE_DIR/south.task"
ain_do sched_click Apply
check "a task not tonight is stored" wait_for 10 sched_has "| South | run 0/1 | normal 50 | NOT TONIGHT"
check "the chart does not show it" sched_has "| targets M31 |"
ain_do tab Tasks
ain_do sched_select M31
check "a queued task keeps the colors of the chart" sched_has "| colors M31:idle"
check "the night plan selects its run" sched_has "SCHED SchedulerPlanSelected M31"
ain_do tab "Night plan"
ain_do sched_plan_select M31
check "a run selected in the night plan selects its task, the chart shows it" sched_has_all "SCHED SchedulerTasksSelected M31" "| highlighted M31"
ain_do sched_deselect plan
check "Escape in the night plan clears the selection of both lists and the chart" sched_has_all "SCHED SchedulerPlanSelected none" "SCHED SchedulerTasksSelected none" "| highlighted  |"
ain_do sched_select M31
ain_do tab Tasks
ain_do shot "$CASE_DIR/chart.png"
ain_do answer Yes
ain_do sched_click "Delete the task; a running task is aborted first"
check "a deleted task is not shown" wait_for 10 sched_has "| targets  |"

section "a task made a hook by mistake"
ain_do sched_open "$CASE_DIR/flats.task"
ain_do tab "Task editor"
ain_do sched_page Scheduling
ain_do sched_combo normal hook
ain_do sched_page Text
ain_do sched_text
check "the text of the hook has no scheduling keys" bash -c "grep '^SCHED_TEXT' '$CASE_DIR/ain.out' | tail -1 | grep -v 'repeat' | grep -q 'priority hook'"
ain_do sched_page Scheduling
ain_do sched_combo hook normal
ain_do sched_page Text
ain_do sched_text
check "another class gets the keys and the level back" bash -c "grep '^SCHED_TEXT' '$CASE_DIR/ain.out' | tail -1 | grep -qF 'priority normal 40\\n// @twilight nautical\\n// @repeat   3'"

stop_ain
stop_server
finish_case
