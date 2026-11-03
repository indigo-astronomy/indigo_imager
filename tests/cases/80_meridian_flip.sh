# A meridian flip with "Use Solver" during a batch, on simulators.

flip_on() {	# 0 Astrometry, 1 Platesolver
	if [ "$1" = 0 ]; then need_astrometry; else need_platesolver; fi
	rm -rf "$HOME"
	start_rig "$1"
	section "$SOLVER"
	check "mount points at the target" point_at_target
	check "transit is close" set_transit_in 40

	cat > flip.js <<JS
var sequence = new Sequence("flip test");
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
	pier_before=$(prop list "$SIM_MOUNT.MOUNT_SIDE_OF_PIER" -t 1 | grep "= ON" | head -1 | sed 's/.*PIER\.//; s/ = ON//')
	check "sequence finishes" wait_for 240 sequence_done
	pier_after=$(prop list "$SIM_MOUNT.MOUNT_SIDE_OF_PIER" -t 1 | grep "= ON" | head -1 | sed 's/.*PIER\.//; s/ = ON//')

	echo "  pier $pier_before -> $pier_after"
	check "sequence is not in alert" equals "$(sequence_state)" OK
	check "flip runs in order, centring with $SOLVER" log_order "Batch started" "Batch paused, transit" "Meridian flip started" "$SOLVER @ local: Solving started" "$SOLVER @ local: Centered" "Meridian flip finished" "Batch finished" "Sequence finished"
	check "flip did not fail" log_lacks "Meridian flip failed"
	# a target in the eastern sky is reached from the west side of the pier, and from the east after transit
	check "pier side changes from west to east" equals "$pier_before-$pier_after" "WEST-EAST"
	check "all frames are captured" equals "$(item "Imager Agent.AGENT_IMAGER_STATS" FRAME)" 25
	stop_rig
}

flip_on 0
flip_on 1

finish_case
