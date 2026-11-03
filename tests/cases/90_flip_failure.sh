# A failed meridian flip sets SEQUENCE_STATE to Alert only if a sequence is running. Unloading the
# solver agent makes the flip fail after its slew. A flip keeps running after the sequence which
# enabled it, for example during a batch started by hand.

need_astrometry

solver_gone() {
	! agent_present "$SOLVER"
}

imager_stopped() {
	[ "$(state_of "Imager Agent.AGENT_START_PROCESS")" != BUSY ]
}

step_states() {
	prop list "Scripting Agent.SEQUENCE_STEP_STATE" -t 1
}

section "outside a sequence"
rm -rf "$HOME"
start_rig 0
check "sequence which enables the flip finishes" point_at_target
check "sequence is OK" equals "$(sequence_state)" OK
steps_before=$(step_states | sort -u)
check "transit is close" set_transit_in 30
prop_set "Server.DRIVERS.indigo_agent_astrometry=OFF"
check "solver agent is unloaded" wait_for 15 solver_gone
prop_set "Imager Agent.AGENT_IMAGER_BATCH.COUNT=30;EXPOSURE=4"
prop_set "Imager Agent.AGENT_START_PROCESS.EXPOSURE=ON"
check "flip fails" wait_for 90 log_has "Meridian flip failed (no solver agent is loaded)"
sleep 2
check "flipper reports the failure" equals "$(state_of "Scripting Agent.FLIPPER_STATE")" ALERT
check "sequence state stays OK" equals "$(sequence_state)" OK
check "sequence step states do not change" equals "$(step_states | sort -u)" "$steps_before"
check "imager is stopped" wait_for 15 imager_stopped
stop_rig

section "during a sequence"
rm -rf "$HOME"
start_rig 0
check "mount points at the target" point_at_target
check "transit is close" set_transit_in 40
cat > flip.js <<JS
var sequence = new Sequence("flip failure test");
sequence.enable_meridian_flip(true, 0);
sequence.capture_batch(25, 4);
sequence.start();
JS
ain_do tab Sequence
ain_do loadseq "$CASE_DIR/flip.js"
sleep 2
ain_do button Run
wait_for 15 equals_busy
check "batch starts" wait_for 30 log_has "Imager Agent @ local: Batch started"
prop_set "Server.DRIVERS.indigo_agent_astrometry=OFF"
check "flip fails" wait_for 90 log_has "Meridian flip failed (no solver agent is loaded)"
check "sequence stops" wait_for 30 sequence_done
check "sequence state is Alert" equals "$(sequence_state)" ALERT
check "imager is stopped" wait_for 15 imager_stopped
stop_rig

finish_case
