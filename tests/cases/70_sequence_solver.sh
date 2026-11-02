# A sequence run with AIN's Run button sends its solver steps to the loaded agent, on simulators.

run_on() {	# 0 Astrometry, 1 Platesolver
	if [ "$1" = 0 ]; then need_astrometry; else need_platesolver; fi
	rm -rf "$HOME"
	start_rig "$1"
	section "$SOLVER"
	check "filter combo lists the wheel" wait_for 15 combo_has FILTER "Filter #2"
	check "sequence filter options come from the agent" combo_has FILTER "Current filter"

	cat > sequence.js <<JS
var sequence = new Sequence("solver test");
sequence.select_imager_camera("$SIM_CAMERA");
sequence.select_filter_wheel("$SIM_WHEEL");
sequence.select_mount("$SIM_MOUNT");
sequence.unpark();
sequence.set_solver_filter("Filter #2");
sequence.sync_center(1);
sequence.start();
JS
	check "sequence finishes" run_sequence sequence.js 120
	check "sequence is not in alert" equals "$(sequence_state)" OK
	check "solver filter is set" equals "$(item "$SOLVER.AGENT_PLATESOLVER_FILTER" 2)" ON
	check "solver exposure is set" equals "$(item "$SOLVER.AGENT_PLATESOLVER_EXPOSURE" EXPOSURE)" 1
	check "solving, syncing and centring are in order" log_order "$SOLVER @ local: Solving started" "$SOLVER @ local: Solved" "$SOLVER @ local: Synced" "$SOLVER @ local: Centered" "Sequence finished"
	stop_rig
}

run_on 0
run_on 1

section "switching refills the Solver tab"
need_platesolver
need_astrometry
rm -rf "$HOME"
start_rig 0
check "filter combo lists the wheel" wait_for 15 combo_has FILTER "Filter #2"
ain_do click Platesolver Agent
check "Platesolver is loaded" wait_for 15 driver_is platesolver ON
check "Solver tab lists Platesolver" wait_for 15 combo_has SOLVER_AGENT "Platesolver Agent @ local"
check "source is kept" combo_has SOURCE "current='Imager Agent'"
check "filter combo refills without selecting the source again" wait_for 15 combo_has FILTER "Filter #2"
stop_rig

finish_case
