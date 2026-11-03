# A loaded solver agent gets its relations back, so that a flip or a solver step of a running
# batch or sequence does not fail after the agent is switched, on simulators.

related() {	# agent, related agent - ON or OFF
	equals "$(item "$1.FILTER_RELATED_AGENT_LIST" "$2")" ON
}

not_related() {
	! related "$@"
}

check_relations() {	# solver agent, image source
	check "$1 is related to the mount" wait_for 15 related "$1" "Mount Agent"
	check "mount is related to $1" wait_for 15 related "Mount Agent" "$1"
	check "$1 is related to $2" wait_for 15 related "$1" "$2"
}

need_astrometry
need_platesolver
rm -rf "$HOME"
start_rig 0

section "startup"
check_relations "Astrometry Agent" "Imager Agent"

section "menu switch"
ain_do click Platesolver Agent
check "Platesolver is loaded" wait_for 15 driver_is platesolver ON
check_relations "Platesolver Agent" "Imager Agent"

section "switch by another client"
prop_set "Server.DRIVERS.indigo_agent_platesolver=OFF"
wait_for 15 driver_is platesolver OFF
prop set -t 1 "Server.LOAD.DRIVER=indigo_agent_astrometry" >/dev/null
check "Astrometry is loaded" wait_for 15 driver_is astrometry ON
check_relations "Astrometry Agent" "Imager Agent"
ain_do click Astrometry Agent
prop_set "Astrometry Agent.AGENT_PLATESOLVER_USE_INDEX.$ASTROMETRY_INDEXES"

section "switch during a sequence"
ain_do source None
check "Solver tab source is unrelated" wait_for 15 not_related "Astrometry Agent" "Imager Agent"
cat > sequence.js <<JS
var sequence = new Sequence("switch test");
sequence.select_imager_camera("$SIM_CAMERA");
sequence.select_mount("$SIM_MOUNT");
sequence.unpark();
sequence.capture_batch(8, 2);
sequence.sync_center(1);
sequence.start();
JS
ain_do tab Sequence
ain_do loadseq "$CASE_DIR/sequence.js"
sleep 2
ain_do button Run
wait_for 15 equals_busy
check "batch starts" wait_for 30 log_has "Imager Agent @ local: Batch started"
ain_do click Platesolver Agent
check "Platesolver is loaded" wait_for 15 driver_is platesolver ON
prop_set "Platesolver Agent.AGENT_PLATESOLVER_USE_INDEX.$PLATESOLVER_INDEXES"
check_relations "Platesolver Agent" "Imager Agent"
check "sequence finishes" wait_for 120 sequence_done
check "sequence is not in alert" equals "$(sequence_state)" OK
check "centring is done by Platesolver" log_order "Platesolver Agent @ local: Solving started" "Platesolver Agent @ local: Centered" "Sequence finished"
stop_rig

finish_case
