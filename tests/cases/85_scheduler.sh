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
ain_do sched_page Properties
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
check "the history shows the run" wait_for 30 sched_has "| Test | run 1 | task | SUCCEEDED"
check "the task is done" wait_for 10 sched_has "SCHED SchedulerTasks Test | DONE"
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
ain_do tab Properties
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

section "settings"
ain_do tab Settings
ain_do sched_hook IDLE Test
check "the hook is set" wait_for 10 hook_is IDLE '"Test"'
check "the settings keep their values" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_SETTINGS' IDLE_CHECK_INTERVAL)" 30
check "the night limit is kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_NIGHT' NAUTICAL)" ON
check "the data switches are kept" equals "$(item 'Scheduler Agent.AGENT_SCHEDULER_DATA' OBJECT)$(item 'Scheduler Agent.AGENT_SCHEDULER_DATA' FOLDER)" ONOFF
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
ain_do sched_page Properties
ain_do shot "$CASE_DIR/hook.png"
check "it is shown in the visual editor" sched_has "SCHED EditorMode visual"
ain_do sched_click Apply
check "the scheduler accepts it" wait_for 10 sched_has "SCHED SchedulerEditorStatus Accepted"
check "the task list shows it as a hook" wait_for 10 sched_has "SCHED SchedulerTasks Park | HOOK | hook"
ain_do answer Yes
ain_do sched_select Park
ain_do sched_click "Delete the task; a running task is aborted first"
check "it is deleted" wait_for 10 sched_lacks "SCHED SchedulerTasks Park"

section "a task made a hook by mistake"
ain_do sched_open "$CASE_DIR/flats.task"
ain_do sched_page Properties
ain_do sched_combo normal hook
ain_do sched_page Text
ain_do sched_text
check "the text of the hook has no scheduling keys" bash -c "grep '^SCHED_TEXT' '$CASE_DIR/ain.out' | tail -1 | grep -v 'repeat' | grep -q 'priority hook'"
ain_do sched_page Properties
ain_do sched_combo hook normal
ain_do sched_page Text
ain_do sched_text
check "another class gets the keys and the level back" bash -c "grep '^SCHED_TEXT' '$CASE_DIR/ain.out' | tail -1 | grep -qF 'priority normal 40\\n// @twilight nautical\\n// @repeat   3'"

stop_ain
stop_server
finish_case
