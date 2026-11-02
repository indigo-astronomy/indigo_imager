# The Sequencer.js embedded in AIN picks the loaded solver agent, and select_solver_agent() overrides it.

probe() {	# driver, expected agent
	section "$2 loaded"
	{
		cat "$ROOT_DIR/ain_imager_src/scripts/Sequencer.js"
		cat <<'JS'

indigo_log("TEST auto=" + sequencer_solver_agent("Imager Agent"));
var s = new Sequence("t");
s.select_solver_agent("Other Solver @ host2");
s.set_solver_filter("Filter #2");
indigo_log("TEST step0=" + s.sequence[0].execute);
indigo_log("TEST step1=" + s.sequence[1].execute);
indigo_sequencer.select_solver_agent("Other Solver @ host2");
indigo_log("TEST selected=" + indigo_sequencer.devices[SOLVER_AGENT]);
JS
	} > probe.js
	start_server indigo_agent_scripting "$1"
	wait_for 15 agent_present "Scripting Agent"
	python3 -I "$TESTS_DIR/fake/send_script.py" $PORT probe.js
	check "script picks $2" wait_for 5 grep -q "TEST auto=$2\$" server.log
	check "select_solver_agent() is a step" grep -q 'TEST step0=select_solver_agent("Other Solver @ host2")' server.log
	check "set_solver_filter() is a step" grep -q 'TEST step1=set_solver_filter("Filter #2")' server.log
	check "select_solver_agent() overrides it" grep -q "TEST selected=Other Solver @ host2\$" server.log
	stop_server
}

probe indigo_agent_astrometry "Astrometry Agent"
probe indigo_agent_platesolver "Platesolver Agent"

finish_case
