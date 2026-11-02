# A meridian flip with "Use Solver" during a batch, on simulators. The simulator always renders the
# same star field, so the location is set to bring that field to transit in the middle of the batch.

TARGET_RA=6.5268
TARGET_DEC=5.0933
# the sequence coordinates are J2000, the mount agent works in JNow
JNOW_OFFSET=0.0439
TRANSIT_AFTER=115

flip_on() {	# 0 Astrometry, 1 Platesolver
	if [ "$1" = 0 ]; then need_astrometry; else need_platesolver; fi
	rm -rf "$HOME"
	start_rig "$1"
	section "$SOLVER"

	display="Mount Agent.AGENT_MOUNT_DISPLAY_COORDINATES_PROPERTY"
	longitude=$(python3 -I -c "
import sys
ra, transit, target = map(float, sys.argv[1:4])
lst = ra - transit
print(round((((target + $JNOW_OFFSET - $TRANSIT_AFTER / 3600 - lst + 12) % 24 - 12) * 15 + 180) % 360 - 180, 5))" "$(item "$display" RA_JNOW)" "$(item "$display" TIME_TO_TRANSIT)" $TARGET_RA)
	prop_set "Mount Agent.GEOGRAPHIC_COORDINATES.LATITUDE=42;LONGITUDE=$longitude"
	prop_set "$SIM_MOUNT.GEOGRAPHIC_COORDINATES.LATITUDE=42;LONGITUDE=$longitude"
	sleep 2

	cat > flip.js <<JS
var sequence = new Sequence("flip test");
sequence.select_imager_camera("$SIM_CAMERA");
sequence.select_filter_wheel("$SIM_WHEEL");
sequence.select_mount("$SIM_MOUNT");
sequence.unpark();
sequence.enable_tracking();
sequence.enable_meridian_flip(true, 0);
sequence.slew($TARGET_RA, $TARGET_DEC);
sequence.sync_center(1);
sequence.capture_batch(25, 4);
sequence.start();
JS
	ain_do tab Sequence
	ain_do loadseq "$CASE_DIR/flip.js"
	sleep 2
	ain_do button Run
	wait_for 15 equals_busy
	check "batch starts" wait_for 60 log_has "Imager Agent @ local: Batch started"
	pier_before=$(prop list "$SIM_MOUNT.MOUNT_SIDE_OF_PIER" -t 1 | grep "= ON" | head -1 | sed 's/.*PIER\.//; s/ = ON//')
	check "sequence finishes" wait_for 240 sequence_done
	pier_after=$(prop list "$SIM_MOUNT.MOUNT_SIDE_OF_PIER" -t 1 | grep "= ON" | head -1 | sed 's/.*PIER\.//; s/ = ON//')

	check "sequence is not in alert" equals "$(sequence_state)" OK
	check "flip runs in order, centring with $SOLVER" log_order "Batch started" "Batch paused, transit" "Meridian flip started" "$SOLVER @ local: Solving started" "$SOLVER @ local: Centered" "Meridian flip finished" "Batch finished" "Sequence finished"
	check "flip did not fail" log_lacks "Meridian flip failed"
	check "pier side changes from east to west" equals "$pier_before-$pier_after" "EAST-WEST"
	check "all frames are captured" equals "$(item "Imager Agent.AGENT_IMAGER_STATS" FRAME)" 25
	stop_rig
}

flip_on 0
flip_on 1

finish_case
